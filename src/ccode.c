/* 
  Copyright (C) 2013 Rafael R. Sevilla

  This file is part of Arcueid

  Arcueid is free software: you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published
  by the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library. If not, see <http://www.gnu.org/licenses/>
*/
#include <stdarg.h>
#include "arcueid.h"
#include "vmengine.h"

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

struct cfunc_t {
  value name;
  union {
    value (*sff)(arc *, ...);
    int (*aff)(arc *, value);
  } cfunc;
  int argc;
};

#if 0

static value cfunc_pprint(arc *c, value sexpr, value *ppstr, value visithash)
{
  __arc_append_cstring(c, "#<cprocedure: ", ppstr);
  arc_prettyprint(c, ((struct cfunc_t *)REP(sexpr))->name, ppstr, visithash);
  __arc_append_cstring(c, ">", ppstr);
  return(*ppstr);
}

#endif

static void cfunc_marker(arc *c, value v, int depth,
			  void (*mark)(struct arc *, value, int))
{
  mark(c, ((struct cfunc_t *)REP(v))->name, depth);
}

static unsigned long cfunc_hash(arc *c, value v, arc_hs *s)
{
  return(arc_hash(c, ((struct cfunc_t *)REP(v))->name));
}

value arc_mkccode(arc *c, int argc, value (*cfunc)(arc *, ...), value name)
{
  value cfn;
  struct cfunc_t *rcfn;

  cfn = arc_mkobject(c, sizeof(struct cfunc_t), T_CCODE);
  rcfn = (struct cfunc_t *)REP(cfn);
  rcfn->name = name;
  rcfn->cfunc.sff = cfunc;
  rcfn->argc = argc;
  return(cfn);
}

value arc_mkaff(arc *c, int (*xaff)(arc *, value), value name)
{
  value aff = arc_mkccode(c, -2, NULL, name);
  struct cfunc_t *rcfn;

  rcfn = (struct cfunc_t *)REP(aff);
  rcfn->cfunc.aff = xaff;
  return(aff);
}

/* Initialize the environment of an Arcueid foreign function.  This
   essentially creates the AFF's local environment if it is not yet
   there (i.e. the the TENVR of the thread is nil.  It will also take
   care of copying the parameters to the function from the thread's
   stack into any parameters that were defined by the AFFDEF macro. */
void __arc_affenv(arc *c, value thr, int prevsize, int extrasize)
{
  /* Not the first call--assume env was already set up before */
  if (!NIL_P(TENVR(thr)))
    return;

  __arc_mkenv(c, thr, prevsize, extrasize);
}

int __arc_affip(arc *c, value thr)
{
  return(TIP(thr).aff_line);
}

/* Call a function.  This sets up the thread so that when the AFF returns,
   the dispatcher will invoke the function that has been set up.  DO NOT
   USE THIS FUNCTION DIRECTLY.  It should only be used from the AFCALL
   macro.

   If a null continuation is passed, the function application is processed
   as a tail call.
 */
int __arc_affapply(arc *c, value thr, value cont, value func, ...)
{
  va_list ap;
  value arg;
  int argc=0;

  /* Add the continuation to the continuation register if a continuation
     was passed.  If it is null, then the current continuation is used,
     as it is a tail call. */
  if (!NIL_P(cont))
    TCONR(thr) = cont;
  va_start(ap, func);
  /* Push the arguments onto the stack. Look for the CLASTARG sentinel
     value. */
  while ((arg = va_arg(ap, value)) != CLASTARG) {
    argc++;
    CPUSH(thr, arg);
  }
  va_end(ap);
  /* set the argument count */
  TARGC(thr) = argc;
  /* set the value register to the function to be called */
  TVALR(thr) = func;
  /* If this is a tail call, overwrite the current environment */
  if (NIL_P(cont))
    __arc_menv(c, thr, argc);
  return(TR_FNAPP);
}

int __arc_affyield(arc *c, value thr, int line)
{
  TIP(thr).aff_line = line;
  return(TR_SUSPEND);
}

int __arc_affiowait(arc *c, value thr, int line, int fd)
{
  TWAITFD(thr) = INT2FIX(fd);
  TSTATE(thr) = Tiowait;
  return(__arc_affyield(c, thr, line));
}

int __arc_resume_aff(arc *c, value thr)
{
  struct cfunc_t *rcfn;

  rcfn = (struct cfunc_t *)REP(TFUNR(thr));
  return(rcfn->cfunc.aff(c, thr));
}

static int cfunc_apply(arc *c, value thr, value cfn)
{
  int argc, i;
  struct cfunc_t *rcfn;
  value *argv;

  rcfn = (struct cfunc_t *)REP(cfn);
  argc = TARGC(thr);
  if (rcfn->argc >= 0 && rcfn->argc != argc) {
    /* XXX - error handling */
    arc_err_cstrfmt(c, "wrong number of arguments (%d for %d)", argc,
		    rcfn->argc);
    return(TR_RC);
  }

  if (rcfn->argc == -2) {
    /* Set up the thread with the initial information for AFFs */
    TIP(thr).aff_line = 0;	/* start at line 0 (start of function body) */
    TENVR(thr) = CNIL;
    TFUNR(thr) = cfn;
    /* return to the trampoline and make it resume from the beginning
       of the function now that everything is ready */
    return(TR_RESUME);
  }

  /* Simple Foreign Functions */
  argv = alloca(sizeof(value)*argc);
  for (i=argc-1; i>=0; i--)
    argv[i] = CPOP(thr);
  switch (rcfn->argc) {
  case -1:
    TVALR(thr) = rcfn->cfunc.sff(c, argc, argv);
    break;
  case 0:
    TVALR(thr) = rcfn->cfunc.sff(c);
    break;
  case 1:
    TVALR(thr) = rcfn->cfunc.sff(c, argv[0]);
    break;
  case 2:
    TVALR(thr) = rcfn->cfunc.sff(c, argv[0], argv[1]);
    break;
  case 3:
    TVALR(thr) = rcfn->cfunc.sff(c, argv[0], argv[1], argv[2]);
    break;
  case 4:
    TVALR(thr) = rcfn->cfunc.sff(c, argv[0], argv[1], argv[2], argv[3]);
    break;
  case 5:
    TVALR(thr) = rcfn->cfunc.sff(c, argv[0], argv[1], argv[2], argv[3],
				 argv[4]);
    break;
  case 6:
    TVALR(thr) = rcfn->cfunc.sff(c, argv[0], argv[1], argv[2], argv[3],
				 argv[4], argv[5]);
    break;
  case 7:
    TVALR(thr) = rcfn->cfunc.sff(c, argv[0], argv[1], argv[2], argv[3],
				 argv[4], argv[5], argv[6]);
    break;
  case 8:
    TVALR(thr) = rcfn->cfunc.sff(c, argv[0], argv[1], argv[2], argv[3],
				 argv[4], argv[5], argv[6], argv[7]);
    break;
    /* XXX - extend this to perhaps 17 arguments just like Ruby */
  default:
    arc_err_cstrfmt(c, "too many arguments");
    break;
  }
  /* Restore continuation for non-AFF.  This will just return to
     wherever we were called from. */
  return(TR_RC);
}

typefn_t __arc_cfunc_typefn__ = {
  cfunc_marker,
  __arc_null_sweeper,
  NULL,
  cfunc_hash,
  NULL,
  NULL,
  cfunc_apply,
  NULL
};
