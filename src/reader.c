/* 
  Copyright (C) 2010 Rafael R. Sevilla

  This file is part of Arcueid

  Arcueid is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation; either version 3 of the
  License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA
  02110-1301 USA.
*/
/* XXX -- We should probably rewrite this in Arc just as we wrote
   the compiler in Arc. */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include "arcueid.h"
#include "alloc.h"
#include "utf.h"
#include "arith.h"
#include "symbols.h"
#include "builtin.h"
#include "../config.h"

value arc_intern(arc *c, value name)
{
  value symid, symval;
  int symintid;

  if ((symid = arc_hash_lookup(c, c->symtable, name)) != CUNBOUND) {
    /* convert the fixnum ID into the symbol value */
    symval = ID2SYM(FIX2INT(symid));
    /* do not allow nil to have a symbol value */
    return((symval == ARC_BUILTIN(c, S_NIL)) ? CNIL : symval);
  }

  symintid = ++c->lastsym;
  symid = INT2FIX(symintid);
  symval = ID2SYM(symintid);
  arc_hash_insert(c, c->symtable, name, symid);
  arc_hash_insert(c, c->rsymtable, symid, name);
  return(symval);
}

value arc_intern_cstr(arc *c, const char *name)
{
  value symstr = arc_mkstringc(c, name);
  return(arc_intern(c, symstr));
}

value arc_sym2name(arc *c, value sym)
{
  value symid;

  symid = INT2FIX(SYM2ID(sym));
  return(arc_hash_lookup(c, c->rsymtable, symid));
}

static Rune scan(arc *c, value src);
static value read_list(arc *c, value src, value eof);
static value read_anonf(arc *c, value src, value eof);
static value read_quote(arc *c, value src, value sym, value eof);
static value read_comma(arc *c, value src, value eof);
static value read_string(arc *c, value src);
static value read_char(arc *c, value src);
static void read_comment(arc *c, value src);
static value read_symbol(arc *c, value src);
static value expand_ssyntax(arc *c, value sym);
static value expand_compose(arc *c, value sym);
static value expand_sexpr(arc *c, value sym);
static value expand_and(arc *c, value sym);

value arc_read(arc *c, value src, value eof)
{
  Rune ch;

  while ((ch = scan(c, src)) >= 0) {
    switch (ch) {
    case '(':
      return(read_list(c, src, eof));
    case ')':
      arc_err_cstrfmt(c, "misplaced right paren");
      return(CNIL);
    case '[':
      return(read_anonf(c, src, eof));
    case ']':
      arc_err_cstrfmt(c, "misplaced right bracket");
      return(CNIL);
    case '\'':
      return(read_quote(c, src, ARC_BUILTIN(c, S_QUOTE), eof));
    case '`':
      return(read_quote(c, src, ARC_BUILTIN(c, S_QQUOTE), eof));
    case ',':
      return(read_comma(c, src, eof));
    case '"':
      return(read_string(c, src));
    case '#':
      return(read_char(c, src));
    case ';':
      read_comment(c, src);
      break;
    case -1:
      return(eof);
    default:
      arc_ungetc_rune(c, ch, src);
      return(read_symbol(c, src));
    }
  }
  return(eof);
}

static value read_list(arc *c, value src, value eof)
{
  value top, val, last;
  Rune ch;
  int indot = 0;

  top = val = last = CNIL;
  while ((ch = scan(c, src)) >= 0) {
    switch (ch) {
    case ';':
      read_comment(c, src);
      break;
    case ')':
      return(top);
    default:
      if (indot) {
	arc_err_cstrfmt(c, "illegal use of .");
	return(CNIL);
      }
      arc_ungetc_rune(c, ch, src);
      val = arc_read(c, src, eof);
      if (val == ARC_BUILTIN(c, S_DOT)) {
	val = arc_read(c, src, eof);
	if (last) {
	  scdr(last, val);
	} else {
	  arc_err_cstrfmt(c, "illegal use of .");
	  return(CNIL);
	}
	indot = 1;
      } else {
	val = cons(c, val, CNIL);
	if (last)
	  scdr(last, val);
	else
	  top = val;
	last = val;
      }
      break;
    }
  }
  arc_err_cstrfmt(c, "unexpected end of source");
  return(CNIL);
}

static void read_comment(arc *c, value src)
{
  Rune ch;

  while ((ch = arc_readc_rune(c, src)) != -1 && !ucisnl(ch))
    ;
  if (ch != -1)
    arc_ungetc_rune(c, ch, src);
}

static int issym(Rune ch)
{
  char *p;

  if (ucisspace(ch)) 
    return(0);
  for (p = "()';[]"; *p != '\0';) {
    if ((Rune)*p++ == ch)
      return(0);
  }
  return(1);
}

#define STRMAX 256

static value getsymbol(arc *c, value src)
{
  Rune buf[STRMAX];
  Rune ch;
  int i;
  value sym, nstr;

  sym = CNIL;
  i=0;
  while ((ch = arc_readc_rune(c, src)) != -1 && issym(ch)) {
    if (i >= STRMAX) {
      nstr = arc_mkstring(c, buf, i);
      sym = (sym == CNIL) ? nstr : arc_strcat(c, sym, nstr);
      i = 0;
    }
    buf[i++] = ch;
  }
  if (i==0 && sym == CNIL)
    return(CNIL);
  nstr = arc_mkstring(c, buf, i);
  sym = (sym == CNIL) ? nstr : arc_strcat(c, sym, nstr);

  arc_ungetc_rune(c, ch, src);
  return(sym);
}

/* parse a symbol name or number */
static value read_symbol(arc *c, value str)
{
  value sym, num;

  if ((sym = getsymbol(c, str)) == CNIL)
    arc_err_cstrfmt(c, "expecting symbol name");
  if (arc_strcmp(c, sym, arc_mkstringc(c, ".")) == INT2FIX(0))
    return(ARC_BUILTIN(c, S_DOT));
  num = arc_string2num(c, sym);
  if (num == CNIL) {
    return(arc_intern(c, sym));
  } else {
    return(num);
  }
}

/* scan for first non-blank character */
static Rune scan(arc *c, value src)
{
  Rune ch;

  while ((ch = arc_readc_rune(c, src)) >= 0 && ucisspace(ch))
    ;
  return(ch);
}

/* Read an Arc square bracketed anonymous function.  This expands to
   (fn (_) ...) */
static value read_anonf(arc *c, value src, value eof)
{
  value top, val, last, ret;
  Rune ch;

  top = val = last = CNIL;
  while ((ch = scan(c, src)) != -1) {
    switch (ch) {
    case ';':
      read_comment(c, src);
      break;
    case ']':
      ret = cons(c, ARC_BUILTIN(c, S_FN),
		 cons(c, cons(c, ARC_BUILTIN(c, S_US), CNIL),
		      cons(c, top, CNIL)));
      return(ret);
    default:
      arc_ungetc_rune(c, ch, src);
      val = cons(c, arc_read(c, src, CNIL), CNIL);
      if (last)
	scdr(last, val);
      else
	top = val;
      last = val;
      break;
    }
  }
  arc_err_cstrfmt(c, "unexpected end of source");
  return(CNIL);

}

static value read_quote(arc *c, value src, value sym, value eof)
{
  value val;

  val = arc_read(c, src, eof);
  return(cons(c, sym, cons(c, val, CNIL)));
}

static value read_comma(arc *c, value src, value eof)
{
  Rune ch;

  if ((ch = arc_readc_rune(c, src)) == '@')
    return(read_quote(c, src, ARC_BUILTIN(c, S_UNQUOTESP), eof));
  arc_ungetc_rune(c, ch, src);
  return(read_quote(c, src, ARC_BUILTIN(c, S_UNQUOTE), eof));
}

/* First unescaped @ in s, if any.  Escape by doubling. */
static int atpos(arc *c, value str, int i)
{
  int len;

  len = arc_strlen(c, str);
  while (i < len) {
    if (arc_strindex(c, str, i) == '@') {
      if (i+1 < len && arc_strindex(c, str, i+1) != '@')
	return(i);
      i++;
    }
    i++;
  }
  return(-1);
}

static value unescape_ats(arc *c, value s)
{
  value ns;
  int i, len;
  Rune ch;

  ns = arc_mkstringlen(c, 0);
  len = arc_strlen(c, s);
  for (i=0; i<len;) {
    ch = arc_strindex(c, s, i);
    if (i+1 < len && ch == '@' && arc_strindex(c, s, i+1) == '@')
      i++;
    ns = arc_strcatc(c, ns, ch);
    i++;
  }
  return(ns);
}

/* XXX - try to make this tail recursive somehow, destructive if need be */
static value codestring(arc *c, value s)
{
  int i, len, rlen;
  value ss, rest, in, expr, i2;

  i = atpos(c, s, 0);
  len = arc_strlen(c, s);
  if (i < 0)
    return(cons(c, s, CNIL));
  ss = arc_substr(c, s, 0, i);
  rest = arc_substr(c, s, i+1, len);
  in = arc_instring(c, rest, CNIL);
  expr = arc_read(c, in, CNIL);
  i2 = FIX2INT(arc_tell(c, in));
  rlen = arc_strlen(c, rest);
  return(cons(c, ss, cons(c, expr, codestring(c, arc_substr(c, rest, i2, rlen)))));
}

static value read_atstring(arc *c, value s)
{
  value cs, p;

  if (atpos(c, s, 0) >= 0) {
    cs = codestring(c, s);
    for (p=cs; p; p = cdr(p)) {
      if (TYPE(car(p)) == T_STRING)
	scar(p, unescape_ats(c, car(p)));
    }
    return(cons(c, ARC_BUILTIN(c, S_STRING), cs));
  }
  return(unescape_ats(c, s));
}

/* XXX - we need to add support for octal and hexadecimal escapes as well */
static value read_string(arc *c, value src)
{
  Rune buf[STRMAX], ch, escrune;
  int i=0, state=1, digval, digcount;
  value nstr, str = CNIL;

  while ((ch = arc_readc_rune(c, src)) != -1) {
    switch (state) {
    case 1:
      switch (ch) {
      case '\"':
	/* end of string */
	nstr = arc_mkstring(c, buf, i);
	str = (str == CNIL) ? nstr : arc_strcat(c, str, nstr);
	return((c->atstrings) ? read_atstring(c, str) : str);
	break;
      case '\\':
	/* escape character */
	state = 2;
	break;
      default:
	if (i >= STRMAX) {
	  nstr = arc_mkstring(c, buf, i);
	  str = (str == CNIL) ? nstr : arc_strcat(c, str, nstr);
	  i = 0;
	}
	buf[i++] = ch;
	break;
      }
      break;
    case 2:
      /* escape code */
      switch (ch) {
      case '\'':
      case '\"':
      case '\\':
	/* ch is as is */
	break;
      case '0':
	ch = 0x0000;
	break;
      case 'a':
	ch = 0x0007;
	break;
      case 'b':
	ch = 0x0008;
	break;
      case 't':
	ch = 0x0009;
	break;
      case 'n':
	ch = 0x000a;
	break;
      case 'v':
	ch = 0x000b;
	break;
      case 'f':
	ch = 0x000c;
	break;
      case 'r':
	ch = 0x000d;
	break;
      case 'U':
      case 'u':
	escrune = 0;
        digcount = 0;
	state = 3;
        continue;
      default:
	arc_err_cstrfmt(c, "unknown escape code");
	break;
      }
      if (i >= STRMAX) {
	nstr = arc_mkstring(c, buf, i);
	str = (str == CNIL) ? nstr : arc_strcat(c, str, nstr);
	i = 0;
      }
      buf[i++] = ch;
      state = 1;
      break;
    case 3:
      /* Unicode escape */
      if (digcount >= 5) {
	arc_ungetc_rune(c, ch, src);
	if (i >= STRMAX) {
	  nstr = arc_mkstring(c, buf, i);
	  str = (str == CNIL) ? nstr : arc_strcat(c, str, nstr);
	  i = 0;
	}
	buf[i++] = escrune;
	state = 1;
      } else {
	if (ch >= '0' && ch <= '9')
	  digval = ch - '0';
	else if (ch >= 'A' && ch <= 'F')
	  digval = ch - 'A' + 10;
	else if (ch >= 'a' && ch <= 'f')
	  digval = ch - 'a' + 10;
	else
	  arc_err_cstrfmt(c, "invalid character in Unicode escape");
	escrune = escrune * 16 + digval;
	digcount++;
      }
      break;
    }
  }
  arc_err_cstrfmt(c, "unterminated string reaches end of input");
  return(CNIL);			/* to pacify -Wall */
}

/* These character constants are inherited by Arc from MzScheme.
   Frankly, I think they're stupid, and one would be better off using
   the same character escape sequences as for strings.  But well,
   we have to live with these types of complications for the sake of
   compatibility--maybe later on we can add a variable that modifies
   this reader behavior to something more rational (such as sharp
   followed by the actual character, with slash for escapes).  Arc
   does not otherwise use the #-sign for anything else. */
static value read_char(arc *c, value src)
{
  value tok, symch;
  int alldigits, i;
  Rune val, ch, digit;

  if ((ch = arc_readc_rune(c, src)) != '\\') {
    arc_err_cstrfmt(c, "invalid character constant");
    return(CNIL);
  }

  /* Special case for any special characters that are not valid symbols. */
  ch = arc_readc_rune(c, src);
  if (!issym(ch)) {
    return(arc_mkchar(c, ch));
  }

  arc_ungetc_rune(c, ch, src);
  tok = getsymbol(c, src);
  if (arc_strlen(c, tok) == 1)	/* single character */
    return(arc_mkchar(c, arc_strindex(c, tok, 0)));
  if (arc_strlen(c, tok) == 3) {
    /* Possible octal escape */
    alldigits = 1;
    val = 0;
    for (i=0; i<3; i++) {
      digit = arc_strindex(c, tok, i);
      if (!isdigit(digit)) {
	alldigits = 0;
	break;
      }
      val = val * 8 + (digit - '0');
    }
    if (alldigits)
      return(arc_mkchar(c, val));

    /* Possible hexadecimal escape */
    if (arc_strindex(c, tok, 0) == 'x') {
      alldigits = 1;
      val = 0;
      for (i=1; i<3; i++) {
	digit = arc_strindex(c, tok, i);
	if (!isxdigit(digit)) {
	  alldigits = 0;
	  break;
	}
	digit = tolower(digit);
	digit = (digit >= '0' && digit <= '9') ? (digit - '0') : (digit - 'a' + 10);
	val = val * 16 + digit;
      }
      if (alldigits)
	return(arc_mkchar(c, val));
    }
    /* Not an octal or hexadecimal escape */
  }

  /* Possible Unicode escape? */
  if (tolower(arc_strindex(c, tok, 0)) == 'u') {
    alldigits = 1;
    val = 0;
    for (i=1; i<arc_strlen(c, tok); i++) {
      digit = arc_strindex(c, tok, i);
      if (!isxdigit(digit)) {
	alldigits = 0;
	break;
      }
      digit = tolower(digit);
      digit = (digit >= '0' && digit <= '9') ? (digit - '0') : (digit - 'a' + 10);
      val = val * 16 + digit;
    }
    if (alldigits)
      return(arc_mkchar(c, val));
    arc_err_cstrfmt(c, "invalid Unicode escape");
  }

  /* Symbolic character escape */
  symch = arc_hash_lookup(c, c->charesctbl, tok);
  if (symch == CUNBOUND)
    arc_err_cstrfmt(c, "invalid character constant");
  return(symch);
}

value arc_ssyntax(arc *c, value x)
{
  value name;
  int i;
  Rune ch;

  if (TYPE(x) != T_SYMBOL)
    return(CNIL);

  name = arc_sym2name(c, x);
  for (i=0; i<arc_strlen(c, name); i++) {
    ch = arc_strindex(c, name, i);
    if (ch == ':' || ch == '~' || ch == '&' || ch == '.' || ch == '!')
      return(CTRUE);
  }
  return(CNIL);
}

#define STRMAX 256

/* I imagine this can be done a lot more cleanly! */
static value expand_compose(arc *c, value sym)
{
  value top, last, nelt, elt, sh;
  Rune ch, buf[STRMAX];
  int negate = 0, i=0, run = 1;

  sh = arc_instring(c, sym, CNIL);
  top = elt = last = CNIL;
  while (run) {
    ch = arc_readc_rune(c, sh);
    switch (ch) {
    case ':':
    case -1:
      nelt = (i > 0) ? arc_mkstring(c, buf, i) : CNIL;
      elt = (elt == CNIL) ? nelt : arc_strcat(c, elt, nelt);
      if (elt != CNIL)
	elt = arc_intern(c, elt);
      i=0;
      if (negate) {
	elt = (elt == CNIL) ? ARC_BUILTIN(c, S_NO) 
	  : cons(c, ARC_BUILTIN(c, S_COMPLEMENT), cons(c, elt, CNIL));
	if (ch == -1 && top == CNIL)
	  return(elt);
	negate = 0;
      }
      if (elt == CNIL) {
	if (ch == -1)
	  run = 0;
	continue;
      }
      elt = cons(c, elt, CNIL);
      if (last)
	scdr(last, elt);
      else
	top = elt;
      last = elt;
      elt = CNIL;
      if (ch == -1)
	run = 0;
      break;
    case '~':
      negate = 1;
      break;
    default:
      if (i >= STRMAX) {
	nelt = arc_mkstring(c, buf, i);
	elt = (elt == CNIL) ? nelt : arc_strcat(c, elt, nelt);
	i=0;
      }
      buf[i++] = ch;
      break;
    }
  }
  if (cdr(top) == CNIL)
    return(car(top));
  return(cons(c, ARC_BUILTIN(c, S_COMPOSE), top));
}

static value expand_sexpr(arc *c, value sym)
{
  Rune ch, buf[STRMAX], prevchar;
  value last, cur, nelt, elt, sh;
  int i=0;

  sh = arc_instring(c, sym, CNIL);
  last = cur = elt = nelt = CNIL;
  while ((ch = arc_readc_rune(c, sh)) != -1) {
    switch (ch) {
    case '.':
    case '!':
      prevchar = ch;
      nelt = (i > 0) ? arc_mkstring(c, buf, i) : CNIL;
      elt = (elt == CNIL) ? nelt : arc_strcat(c, elt, nelt);
      i=0;
      if (elt == CNIL)
	continue;
      elt = arc_intern(c, elt);
      if (last == CNIL)
	last = elt;
      else if (prevchar == '!')
	last = cons(c, last, cons(c, cons(c, ARC_BUILTIN(c, S_QUOTE), cons(c, elt, CNIL)), CNIL));
      else
	last = cons(c, last, cons(c, elt, CNIL));
      elt = CNIL;
      break;
    default:
      if (i >= STRMAX) {
	nelt = arc_mkstring(c, buf, i);
	elt = (elt == CNIL) ? nelt : arc_strcat(c, elt, nelt);
	i=0;
      }
      buf[i++] = ch;
      break;
    }
  }

  nelt = (i > 0) ? arc_mkstring(c, buf, i) : CNIL;
  elt = (elt == CNIL) ? nelt : arc_strcat(c, elt, nelt);
  elt = arc_intern(c, elt);
  if (elt == CNIL) {
    arc_err_cstrfmt(c, "Bad ssyntax %s", sym);
    return(CNIL);
  }
  if (last == CNIL) {
    if (prevchar == '!')
      return(cons(c, ARC_BUILTIN(c, S_GET), cons(c, cons(c, ARC_BUILTIN(c, S_QUOTE), cons(c, elt, CNIL)), CNIL)));
    return(cons(c, ARC_BUILTIN(c, S_GET), cons(c, elt, CNIL)));
  }
  if (prevchar == '!')
    return(cons(c, last, cons(c, cons(c, ARC_BUILTIN(c, S_QUOTE), cons(c, elt, CNIL)), CNIL)));
  return(cons(c, last, cons(c, elt, CNIL)));
}

static value expand_and(arc *c, value sym)
{
  value top, last, nelt, elt, sh;
  Rune ch, buf[STRMAX];
  int i=0, run = 1;

  top = elt = last = CNIL;
  sh = arc_instring(c, sym, CNIL);
  while (run) {
    ch = arc_readc_rune(c, sh);
    switch (ch) {
    case '&':
    case -1:
      nelt = (i > 0) ? arc_mkstring(c, buf, i) : CNIL;
      elt = (elt == CNIL) ? nelt : arc_strcat(c, elt, nelt);
      if (elt != CNIL)
	elt = arc_intern(c, elt);
      i=0;
      if (elt == CNIL) {
	if (ch == -1)
	  run = 0;
	continue;
      }
      elt = cons(c, elt, CNIL);
      if (last)
	scdr(last, elt);
      else
	top = elt;
      last = elt;
      elt = CNIL;
      if (ch == -1)
	run = 0;
      break;
    default:
      if (i >= STRMAX) {
	nelt = arc_mkstring(c, buf, i);
	elt = (elt == CNIL) ? nelt : arc_strcat(c, elt, nelt);
	i=0;
      }
      buf[i++] = ch;
      break;
    }
  }
  if (cdr(top) == CNIL)
    return(car(top));
  return(cons(c, ARC_BUILTIN(c, S_ANDF), top));
}

static value expand_ssyntax(arc *c, value sym)
{
  if (arc_strchr(c, sym, ':') != CNIL || arc_strchr(c, sym, '~') != CNIL)
    return(expand_compose(c, sym));
  if (arc_strchr(c, sym, '.') != CNIL || arc_strchr(c, sym, '!') != CNIL)
    return(expand_sexpr(c, sym));
  if (arc_strchr(c, sym, '&') != CNIL)
    return(expand_and(c, sym));
  return(CNIL);
} 

value arc_ssexpand(arc *c, value sym)
{
  value x;

  if (TYPE(sym) != T_SYMBOL)
    return(sym);
  x = arc_sym2name(c, sym);
  return(expand_ssyntax(c, x));
}

static struct {
  char *str;
  Rune val;
} chartbl[] = {
  { "null", 0 }, { "nul", 0 }, { "backspace", 8 }, { "tab", 9 },
  { "newline", 10 }, { "vtab", 11 }, { "page", 12 }, { "return", 13 },
  { "space", 32 }, { "rubout", 127 }, { NULL, -1 }
};

/* We must synchronize this against symbols.h as necessary! */
static char *syms[] = { "fn", "_", "quote", "quasiquote", "unquote",
			"unquote-splicing", "compose", "complement",
			"t", "nil", "no", "andf", "get", "sym",
			"fixnum", "bignum", "flonum", "rational",
			"complex", "char", "string", "cons", "table",
			"input", "output", "exception", "port",
			"thread", "vector", "continuation", "closure",
			"code", "environment", "vmcode", "ccode",
			"custom", "int", "unknown", "re", "im", "num",
			"sig", "stdin-fd", "stdout-fd", "stderr-fd",
			"mac", "if", "assign", "o", ".", "car", "cdr",
			"scar", "scdr", "is", "+", "-", "*", "/",
			"and", "apply", "chan", "AF_UNIX", "AF_INET",
			"AF_INET6", "SOCK_STREAM", "SOCK_DGRAM",
			"SOCK_RAW" };

void arc_init_reader(arc *c)
{
  int i;

  /* So that we don't have to add them to the rootset, we mark the
     symbol table, the builtin table, and the character escape table
     and its entries as immutable and immune from garbage collection. */
  c->lastsym = 0;
  c->symtable = arc_mkhash(c, 10);
  BLOCK_IMM(c->symtable);
  c->rsymtable = arc_mkhash(c, 10);
  BLOCK_IMM(c->rsymtable);
  c->builtin = arc_mkvector(c, S_THE_END);
  for (i=0; i<S_THE_END; i++)
    ARC_BUILTIN(c, i) = arc_intern(c, arc_mkstringc(c, syms[i]));

  c->charesctbl = arc_mkhash(c, 5);
  BLOCK_IMM(c->charesctbl);
  for (i=0; chartbl[i].str; i++) {
    value str = arc_mkstringc(c, chartbl[i].str);
    value chr = arc_mkchar(c, chartbl[i].val);
    value cell;

    BLOCK_IMM(str);
    BLOCK_IMM(chr);
    arc_hash_insert(c, c->charesctbl, str, chr);
    cell = arc_hash_lookup2(c, c->charesctbl, str);
    BLOCK_IMM(cell);
    arc_hash_insert(c, c->charesctbl, chr, str);
    cell = arc_hash_lookup2(c, c->charesctbl, chr);
    BLOCK_IMM(cell);
  }
}
