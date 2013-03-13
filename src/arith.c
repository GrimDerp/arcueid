/* 
  Copyright (C) 2013 Rafael R. Sevilla

  This file is part of Arcueid

  Arcueid is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published
  by the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <complex.h>
#include "arcueid.h"
#include "arith.h"
#include "../config.h"
#include "utf.h"

#ifdef HAVE_GMP_H
#include <gmp.h>
#endif

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#elif defined __GNUC__
#ifndef alloca
# define alloca __builtin_alloca
#endif
#elif defined _AIX
# define alloca __alloca
#elif defined _MSC_VER
# include <malloc.h>
# define alloca _alloca
#else
# include <stddef.h>
void *alloca (size_t);
#endif

#define ABS(x) (((x)>=0)?(x):(-(x)))
#define SGN(x) (((x)>=0)?(1):(-(1)))

#ifdef HAVE_GMP_H
#define REPBNUM(n) *((mpz_t *)REP(n))
#define REPRAT(q) *((mpq_t *)REP(q))
#endif

/*================================= Flonums */

AFFDEF(flonum_pprint, f)
{
  AFBEGIN;
  double val = *((double *)REP(AV(f)));
  int len;
  char *outstr;

  len = snprintf(NULL, 0, "%g", val);
  outstr = (char *)alloca(sizeof(char)*(len+2));
  snprintf(outstr, len+1, "%g", val);
  ARETURN(arc_mkstringc(c, outstr));
  AFEND;
}
AFFEND

static value flonum_hash(arc *c, value f, arc_hs *s)
{
  char *ptr = (char *)REP(f);
  int i;

  for (i=0; i<sizeof(double)/sizeof(char); i++)
    arc_hash_update(s, *ptr++);
  return(1);
}

static value flonum_iscmp(arc *c, value v1, value v2)
{
  return((*((double *)REP(v1)) == *((double *)REP(v2))) ? CTRUE : CNIL);
}

static value flonum_coerce(arc *c, value v1, enum arc_types t)
{
  return(CNIL);
}

static value add_flonum(arc *c, value v1, value v2)
{
  return(arc_mkflonum(c, REPFLO(v1) + REPFLO(v2)));
}

value arc_mkflonum(arc *c, double val)
{
  value cv;

  cv = arc_mkobject(c, sizeof(double), T_FLONUM);
  *((double *)REP(cv)) = val;
  return(cv);
}

/*================================= End Flonums */

/*================================= Complex */

AFFDEF(complex_pprint, z)
{
  AFBEGIN;
  double complex val = *((double *)REP(AV(z)));
  int len;
  char *outstr;

  len = snprintf(NULL, 0, "%g%+gi", creal(val), cimag(val));
  outstr = (char *)alloca(sizeof(char)*(len+2));
  snprintf(outstr, len+1, "%g%+gi", creal(val), cimag(val));
  ARETURN(arc_mkstringc(c, outstr));
  AFEND;
}
AFFEND

static value complex_hash(arc *c, value f, arc_hs *s)
{
  char *ptr = (char *)REP(f);
  int i;

  for (i=0; i<sizeof(double complex)/sizeof(char); i++)
    arc_hash_update(s, *ptr++);
  return(1);
}

static value complex_iscmp(arc *c, value v1, value v2)
{
  return((*((double complex *)REP(v1)) == *((double complex *)REP(v2))) ? CTRUE : CNIL);
}

static value complex_coerce(arc *c, value v1, enum arc_types t)
{
  return(CNIL);
}

static value add_complex(arc *c, value v1, value v2)
{
  return(arc_mkcomplex(c, REPCPX(v1) + REPCPX(v2)));
}

value arc_mkcomplex(arc *c, double complex z)
{
  value cv;

  cv = arc_mkobject(c, sizeof(double complex), T_COMPLEX);
  *((double complex *)REP(cv)) = z;
  return(cv);
}

/*================================= End Complex */

#ifdef HAVE_GMP_H

/*================================= Bignum */

static void bignum_sweep(arc *c, value v)
{
  mpz_clear(REPBNUM(v));
}

AFFDEF(bignum_pprint, n)
{
  AFBEGIN;
  char *outstr;
  int len;
  value psv;

  len = mpz_sizeinbase(REPBNUM(AV(n)), 10) + 1;
  /* XXX should we be using a real malloc for this? */
  outstr = (char *)malloc(sizeof(char)*len);
  mpz_get_str(outstr, 10, REPBNUM(AV(n)));
  psv = arc_mkstringc(c, outstr);
  free(outstr);
  ARETURN(psv);
  AFEND;
}
AFFEND

static value bignum_coerce(arc *c, value v1, enum arc_types t)
{
  return(CNIL);
}

static value bignum_hash(arc *c, value n, arc_hs *s)
{
  unsigned long *rop;
  size_t numb = sizeof(unsigned long);
  size_t countp, calc_size;
  int i;
 
  calc_size = (mpz_sizeinbase(REPBNUM(n),  2) + numb-1) / numb;
  rop = (unsigned long *)malloc(calc_size * numb);
  mpz_export(rop, &countp, 1, numb, 0, 0, REPBNUM(n));
  for (i=0; i<countp; i++)
    arc_hash_update(s, rop[i]);
  free(rop);
  return((unsigned long)countp);
}

static value bignum_iscmp(arc *c, value v1, value v2)
{
  return((mpz_cmp(REPBNUM(v1), REPBNUM(v2)) == 0) ?
	 CTRUE : CNIL);
}

static value add_bignum(arc *c, value v1, value v2)
{
  return(CNIL);
}

value arc_mkbignuml(arc *c, long val)
{
  value cv;

  cv = arc_mkobject(c, sizeof(mpz_t), T_BIGNUM);
  mpz_init(REPBNUM(cv));
  mpz_set_si(REPBNUM(cv), val);
  return(cv);
}

/*================================= End Bignum */

/*================================= Rational */

static value add_rational(arc *c, value v1, value v2)
{
  return(CNIL);
}

/*================================= End Rational */

#endif

/*================================= Basic arithmetic functions */

/* All arithmetic functions take two arguments, and return their
   result, not modifying their arguments.  A new cell is allocated
   from the heap for the result if this is needed.

   The following implicit type conversions occur for arithmetic
   operations:

              Fixnum   Bignum   Rational   Flonum   Complex
   Fixnum     Fixnum*  Bignum   Rational   Flonum   Complex
   Bignum     Bignum   Bignum   Rational   Flonum   Complex
   Rational   Rational Rational Rational   Flonum   Complex
   Flonum     Flonum   Flonum   Flonum     Flonum   Complex
   Complex    Complex  Complex  Complex    Complex  Complex

   * If a bignum result is smaller than ±FIXNUM_MAX, it is
   implicitly converted to a fixnum.  If arithmetic on fixnums
   would give a result greater than ±FIXNUM_MAX, it will
   automatically extend to a bignum unless bignum support is not
   compiled in (in which case an overflow error is signaled).

   If a rational has a denominator of 1, the result is implicitly
   converted to a fixnum, if the range allows, or a bignum if not.

   Division of fixnums and/or bignums will result in a fixnum or a
   bignum only if the two numbers divide each other exactly.  If the
   division is inexact, the result will be a rational number if
   support for bignum/rational arithmetic has been compiled in.  If
   not, the quotient alone is returned.
 */

#ifdef HAVE_GMP_H

#define TYPE_CASES(func, arg1, arg2) do {				\
    typefn_t *tfn = __arc_typefn(c, arg1),				\
      *tfn2 = __arc_typefn(c, arg2);					\
    if (tfn == NULL || tfn->coerce == NULL ||				\
	tfn2 == NULL || tfn2->coerce == NULL) {				\
      arc_err_cstrfmt(c, "cannot coerce");				\
      return(CNIL);							\
    }									\
    if (TYPE(arg1) == T_COMPLEX || TYPE(arg2) == T_COMPLEX) {		\
      arg1 = tfn->coerce(c, arg1, T_COMPLEX);				\
      arg2 = tfn2->coerce(c, arg2, T_COMPLEX);				\
      return(func##_complex(c, arg1, arg2));				\
    } else if (TYPE(arg1) == T_FLONUM || TYPE(arg2) == T_FLONUM) {	\
      arg1 = tfn->coerce(c, arg1, T_FLONUM);				\
      arg2 = tfn2->coerce(c, arg2, T_FLONUM);				\
      return(func##_flonum(c, arg1, arg2));				\
    } else if (TYPE(arg1) == T_RATIONAL || TYPE(arg2) == T_RATIONAL) {	\
      arg1 = tfn->coerce(c, arg1, T_RATIONAL);				\
      arg2 = tfn2->coerce(c, arg2, T_RATIONAL);				\
      return(func##_rational(c, arg1, arg2));				\
    } else if (TYPE(arg1) == T_BIGNUM || TYPE(arg2) == T_BIGNUM) {	\
      arg1 = tfn->coerce(c, arg1, T_BIGNUM);				\
      arg2 = tfn2->coerce(c, arg2, T_BIGNUM);				\
      return(func##_bignum(c, arg1, arg2));				\
    }									\
  } while (0)

#else

#define TYPE_CASES(func, arg1, arg2) do {				\
    typefn_t *tfn = __arc_typefn(c, arg1),				\
      *tfn2 = __arc_typefn(c, arg2);					\
    if (tfn == NULL || tfn->coerce == NULL ||				\
	tfn2 == NULL || tfn2->coerce == NULL) {				\
      arc_err_cstrfmt(c, "cannot coerce");				\
      return(CNIL);							\
    }									\
    if (TYPE(arg1) == T_COMPLEX || TYPE(arg2) == T_COMPLEX) {		\
      arg1 = tfn->coerce(c, arg1, T_COMPLEX);				\
      arg2 = tfn2->coerce(c, arg2, T_COMPLEX);				\
      return(func##_complex(c, arg1, arg2));				\
    } else if (TYPE(arg1) == T_FLONUM || TYPE(arg2) == T_FLONUM) {	\
      arg1 = tfn->coerce(c, arg1, T_FLONUM);				\
      arg2 = tfn2->coerce(c, arg2, T_FLONUM);				\
      return(func##_flonum(c, arg1, arg2));				\
    }									\
  } while (0)

#endif

value __arc_add2(arc *c, value arg1, value arg2)
{
  long fixnum_sum;

  if (TYPE(arg1) == T_FIXNUM && TYPE(arg2) == T_FIXNUM) {
    fixnum_sum = FIX2INT(arg1) + FIX2INT(arg2);
#ifdef HAVE_GMP_H
    if (ABS(fixnum_sum) > FIXNUM_MAX)
      return(arc_mkbignuml(c, fixnum_sum));
    /* without bignum support, overflow can do strange things, sorry! */
#endif
    return(INT2FIX(fixnum_sum));
  } 

  /* Frankly, I think overloading + in this way is a mistake,
     but it is not my design decision. */
  if ((NIL_P(arg1) || TYPE(arg1) == T_CONS)
      && (NIL_P(arg2) || TYPE(arg2) == T_CONS)) {
    if (arg1 == CNIL)
      return(arg2);
    if (arg2 == CNIL)
      return(arg1);
    return(arc_list_append(arg2, arg1));
  }

  if ((NIL_P(arg1) || TYPE(arg1) == T_STRING)
      && (NIL_P(arg2) || TYPE(arg2) == T_STRING)) {
    if (arg1 == CNIL)
      return(arg2);
    if (arg2 == CNIL)
      return(arg1);
    return(arc_strcat(c, arg2, arg1));
  }

  if (NIL_P(arg1) && TYPE(arg2) == T_CHAR) {
    Rune data[1];

    data[0] = *((Rune *)REP(arg2));
    return(arc_mkstring(c, data, 1));
  }

  if (NIL_P(arg2) && TYPE(arg1) == T_CHAR) {
    Rune data[1];

    data[0] = *((Rune *)REP(arg1));
    return(arc_mkstring(c, data, 1));
  }

  if (TYPE(arg1) == T_STRING && TYPE(arg2) == T_CHAR) {
    Rune data[1];

    data[0] = *((Rune *)REP(arg2));
    return(arc_strcat(c, arc_mkstring(c, data, 1), arg1));
  }

  if (TYPE(arg1) == T_CHAR && TYPE(arg2) == T_STRING) {
    return(arc_strcatc(c, arg2, *((Rune *)REP(arg1))));
  }

  if (TYPE(arg2) == T_STRING) {
    value carg1;
    typefn_t *tfn;

    tfn = __arc_typefn(c, arg1);
    if (tfn == NULL || tfn->coerce == NULL) {
      arc_err_cstrfmt(c, "cannot coerce to string");
      return(CNIL);
    }
    carg1 = tfn->coerce(c, arg1, T_STRING);
    return(arc_strcat(c, arg2, carg1));
  }

  TYPE_CASES(add, arg1, arg2);

  arc_err_cstrfmt(c, "Invalid types for addition");
  return(CNIL);
}

#if 0

/* utility functions */
static Rune strgetc(arc *c, value str, int *index)
{
  if (*index < arc_strlen(c, str))
    return(arc_strindex(c, str, (*index)++));
  return(Runeerror);
}

static void strungetc(arc *c, int *index)
{
  if (*index <= 0)
    return;
  (*index)--;
}

static value rune2dig(Rune r, int radix)
{
  Rune rl;
  value v;

  if (!ucisalnum(r))
    return(CNIL);
  rl = tolower(r);
  if (rl >= 0x30 && rl <= 0x39)
    v = rl - 0x30;
  else if (rl >= 0x61 && rl <= 0x7a)
    v = (rl - 0x61) + 10;
  if (v > radix)
    return(CNIL);
  return(INT2FIX(v));
}

static double str2flonum(arc *c, value str, int index, int imagflag)
{
  int state = 1, expn = 0, expnsign = 1;
  double sign = 1.0, mantissa=0.0, mult=0.1, fnum;
  value digitval, imag;
  Rune ch;

  while ((ch = strgetc(c, str, &index)) != Runeerror) {
    switch (state) {
    case 1:
      /* sign */
      switch (ch) {
      case '-':
	sign = -1;
	state = 2;
	break;
      case '+':
	sign = 1;
	state = 2;
	break;
      default:
	if (!(isdigit(ch) || ch == '.'))
	  return(CNIL);
	strungetc(c, &index);
	state = 2;
	break;
      }
      break;
    case 2:
      /* non-fractional part of mantissa */
      switch (ch) {
      case '.':
	state = 3;
	break;
      case 'e':
      case 'E':
	state = 4;
	break;
      case '+':
      case '-':
	/* Complex */
	if (imagflag) {
	  return(CNIL);
	} else {
	  /* read the imaginary part */
	  imag = str2flonum(c, str, index-1, 1);
	  if (TYPE(imag) != T_COMPLEX)
	    return(CNIL);
	  REPCPX(imag) += sign * (mantissa * pow(10, expnsign * expn));
	  return(imag);
	}
	break;
      case 'i':
      case 'I':
      case 'j':
      case 'J':
	/* imaginary */
	return(arc_mkcomplex(c, I * sign * (mantissa
					    * pow(10, expnsign * expn))));
      default:
	if (!isdigit(ch))
	  return(CNIL);
	mantissa = (mantissa * 10) + FIX2INT(rune2dig(ch, 10));
	break;
      }
      break;
    case 3:
      /* fractional part of mantissa */
      switch (ch) {
      case 'e':
      case 'E':
	state = 4;
	break;
      case '+':
      case '-':
	/* Complex */
	if (imagflag) {
	  return(CNIL);
	} else {
	  /* read the imaginary part */
	  imag = str2flonum(c, str, index-1, 1);
	  if (TYPE(imag) != T_COMPLEX)
	    return(CNIL);
	  REPCPX(imag) += sign * (mantissa * pow(10, expnsign * expn));
	  return(imag);
	}
	break;
      case 'i':
      case 'I':
      case 'j':
      case 'J':
	/* imaginary */
	return(arc_mkcomplex(c, 0.0 +
			      I *sign * (mantissa
					 * pow(10, expnsign * expn))));
      default:
	if (!isdigit(ch))
	  return(CNIL);
	mantissa += FIX2INT(rune2dig(ch, 10)) * mult;
	mult *= 0.1;
	break;
      }
      break;
    case 4:
      /* exponent sign */
      switch (ch) {
      case '-':
	expnsign = -1;
	state = 5;
	break;
      case '+':
	expnsign = 1;
	state = 5;
	break;
      default:
	if (!isdigit(ch))
	  return(CNIL);
	strungetc(c, &index);
	state = 5;
	break;
      }
      break;
    case 5:
      switch (ch) {
      case '+':
      case '-':
	/* Complex */
	if (imagflag) {
	  return(CNIL);
	} else {
	  /* read the imaginary part */
	  imag = str2flonum(c, str, index-1, 1);
	  if (TYPE(imag) != T_COMPLEX)
	    return(CNIL);
	  REPCPX(imag) += sign * (mantissa * pow(10, expnsign * expn));
	  return(imag);
	}
	break;
      case 'i':
      case 'I':
      case 'j':
      case 'J':
	/* imaginary */
	return(arc_mkcomplex(c, 0.0 + 
			     I * sign * (mantissa
					 * pow(10, expnsign * expn))));
      default:
	/* exponent magnitude */
	digitval = rune2dig(ch, 10);
	if (digitval == CNIL)
	  return(CNIL);
	expn = expn * 10 + FIX2INT(digitval);
	break;
      }
      break;
    }
  }
  /* Combine */
  fnum = sign * (mantissa * pow(10, expnsign * expn));
  return(arc_mkflonum(c, fnum));
}

value arc_string2num(arc *c, value str, int index, int rational)
{
  int state = 1, sign=1, radsel = 0, i;
  Rune ch;
  value nval = INT2FIX(0), digitval, radix = INT2FIX(10), denom;
  Rune chs[4];

  while ((ch = strgetc(c, str, &index)) != Runeerror) {
    switch (state) {
    case 1:
      /* sign */
      switch (ch) {
      case '-':
	sign = -1;
	state = 2;
	break;
      case '+':
	sign = 1;
	state = 2;
	break;
      case '.':
	return(str2flonum(c, str, 0, 0));
	break;
      default:
	if (!isdigit(ch))
	  return(CNIL);
	strungetc(c, &index);
	state = 2;
	break;
      }
      break;
    case 2:
      /* digits, or possible radix */
      switch (ch) {
      case '0':
	radix = INT2FIX(8);
	state = 3;
	break;
      case '.':
	return(str2flonum(c, str, 0, 0));
	break;
      default:
	if (tolower(ch) == 'i') {
	  for (i=0; i<4; i++) {
	    chs[i] = strgetc(c, str, &index);
	    if (chs[i] == Runeerror)
	      return(CNIL);
	  }
	  if (chs[0] == 'n' && chs[1] == 'f' && chs[2] == '.' && chs[3] == '0')
	    return(arc_mkflonum(c, sign*INFINITY));
	  return(CNIL);
	}
	if (tolower(ch) == 'n') {
	  for (i=0; i<4; i++) {
	    chs[i] = strgetc(c, str, &index);
	    if (chs[i] == Runeerror)
	      return(CNIL);
	  }
	  if (chs[0] == 'a' && chs[1] == 'n' && chs[2] == '.' && chs[3] == '0')
	    return(arc_mkflonum(c, NAN));
	  return(CNIL);
	}
	if (!isdigit(ch))
	  return(CNIL);
	/* more digits */
	strungetc(c, &index);
	state = 4;
	break;
      }
      break;
    case 3:
      /* digits, or possible radix */
      switch (ch) {
      case 'x':
	radix = INT2FIX(16);
	state = 4;
	break;
      case 'b':
	radix = INT2FIX(2);
	state = 4;
	break;
      case '.':
	return(str2flonum(c, str, 0, 0));
	break;
      case '+':
	/* complex */
	return(str2flonum(c, str, 0, 0));
	break;
      case '-':
	/* complex */
	return(str2flonum(c, str, 0, 0));
	break;
      default:
	if (!isdigit(ch))
	  return(CNIL);
	/* more digits */
	strungetc(c, &index);
	state = 4;
	break;
      }
      break;
    case 4:
      /* digits */
      switch (ch) {
      case '.':
      case 'e':
      case 'E':
	return(str2flonum(c, str, 0, 0));
	break;
      case 'r':
	/* Limbo-style radix selector: the base radix should
	   still be 10, and the value should be between 2 and 36 */
	if (radix == INT2FIX(10) && TYPE(nval) == T_FIXNUM
	    && (FIX2INT(nval) >= 2 && FIX2INT(nval) <= 36)) {
	  radix = nval;
	  nval = INT2FIX(0);
	  state = 5;
	} else if (!radsel) {
	  return(CNIL);		/* invalid radix selector */
	}
	break;
      case '/':
	if (rational)
	  return(CNIL);
	denom = arc_string2num(c, str, index, 1);
	if (TYPE(denom) == T_FIXNUM || TYPE(denom) == T_BIGNUM)
	  return(__arc_div2(c, __arc_mul2(c, INT2FIX(sign), nval), denom));
	else
	  return(CNIL);
	break;
      case 'i':
      case 'I':
      case 'j':
      case 'J':
	return(str2flonum(c, str, 0, 0));
      case '+':
      case '-':
	return(str2flonum(c, str, 0, 0));
      default:
	/* Digits */
	digitval = rune2dig(ch, radix);
	if (digitval == CNIL)
	  return(CNIL);
	nval = __arc_add2(c, __arc_mul2(c, nval, radix), digitval);
	break;
      }
      break;
    case 5:
      /* Digits */
      digitval = rune2dig(ch, radix);
      if (digitval == CNIL)
	return(CNIL);
      nval = __arc_add2(c, __arc_mul2(c, nval, radix), digitval);
      break;
    case 6:
      break;
    }
  }
  /* For nval to be a valid number, we must have entered at least state 3.
     If we have not, the number is not valid. */
  if (state >= 3) {
    nval = __arc_mul2(c, nval, INT2FIX(sign));
    return(nval);
  }
  return(CNIL);
}

#else

value arc_string2num(arc *c, value str, int index, int rational)
{
  return(CNIL);
}

#endif

typefn_t __arc_complex_typefn__ = {
  __arc_null_marker,
  __arc_null_sweeper,
  complex_pprint,
  complex_hash,
  complex_iscmp,
  NULL,
  NULL,
  complex_coerce,
};

typefn_t __arc_flonum_typefn__ = {
  __arc_null_marker,
  __arc_null_sweeper,
  flonum_pprint,
  flonum_hash,
  flonum_iscmp,
  NULL,
  NULL,
  flonum_coerce,
};

#ifdef HAVE_GMP_H

typefn_t __arc_bignum_typefn__ = {
  __arc_null_marker,
  bignum_sweep,
  bignum_pprint,
  bignum_hash,
  bignum_iscmp,
  NULL,
  NULL,
  bignum_coerce
};

#endif
