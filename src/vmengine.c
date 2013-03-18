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
#include "arcueid.h"
#include "vmengine.h"
#include "arith.h"

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

static value apply_return(arc *c, value thr)
{
  value cont;

  if (NIL_P(TCONR(thr)))
    return(CNIL);
  cont = car(TCONR(thr));
  TCONR(thr) = cdr(TCONR(thr));
  arc_restorecont(c, thr, cont);
  return(cont);
}

static int apply_vr(arc *c, value thr)
{
  typefn_t *tfn;

  TFUNR(thr) = TVALR(thr);
  tfn = __arc_typefn(c, TVALR(thr));
  if (tfn == NULL) {
    arc_err_cstrfmt(c, "cannot apply value");
    return(0);
  }
  return(tfn->apply(c, thr, TVALR(thr)));
}

/* This is the main trampoline.  Its job is basically to keep running
   the thread until some condition occurs where it is no longer able to.

   The trampoline states are as follows:

   TR_RESUME
   Resume execution of the current state of the function being executed
   after it has been suspended for whatever reason.

   TR_SUSPEND
   Return to the thread dispatcher if the thread has been suspended for
   whatever reason.

   TR_FNAPP
   Apply the function that has been set up in the value register.  This
   will set up the thread state so that APP_RESUME will begin executing
   the function in question.

   TR_RC
   Restore the last continuation on the continuation register.
*/
void __arc_thr_trampoline(arc *c, value thr, enum tr_states_t state)
{
  value cont;

  for (;;) {
    switch (state) {
    case TR_RESUME:
      /* Resume execution of the current virtual machine state. */
      state = (TYPE(TFUNR(thr)) == T_CCODE) ? __arc_resume_aff(c, thr) : __arc_vmengine(c, thr);
      break;
    case TR_SUSPEND:
      /* just return to the dispatcher */
      return;
    case TR_FNAPP:
      /* If the state of the trampoline becomes TR_FNAPP, we will attempt
	 to apply the function in the value register, with arguments on
	 the stack, whatever type of function it happens to be. */
      state = apply_vr(c, thr);
      break;
    case TR_RC:
    default:
      /* Restore the last continuation on the continuation register.  Just
	 return to the virtual machine if the last continuation was a normal
	 continuation. */
      cont = apply_return(c, thr);
      if (NIL_P(cont)) {
	/* There was no available continuation on the continuation
	   register.  If this happens, the current thread should
	   terminate. */
	TQUANTA(thr) = 0;
	TSTATE(thr) = Trelease;
	return;
      }
      state = TR_RESUME;
      break;
    }
  }
}

/* instruction decoding macros */
#ifdef HAVE_THREADED_INTERPRETER
/* threaded interpreter */
#define INST(name) lbl_##name
#define JTBASE ((void *)&&lbl_inop)
#ifdef HAVE_TRACING
#define NEXT {							\
    if (--TQUANTA(thr) <= 0)					\
      goto endquantum;						\
    if (vmtrace)						\
      trace(c, thr);						\
    goto *(JTBASE + jumptbl[*TIPP(thr)++]); }
#else
#define NEXT {							\
    if (--TQUANTA(thr) <= 0)					\
      goto endquantum;						\
    goto *(JTBASE + jumptbl[*TIPP(thr)++]); }
#endif

#else
/* switch interpreter */
#define INST(name) case name
#define NEXT break
#endif

/* The actual virtual machine engine.  Fits into the trampoline just
   like a normal function. */
int __arc_vmengine(arc *c, value thr)
{
#ifdef HAVE_THREADED_INTERPRETER
  static const int jumptbl[] = {
#include "jumptbl.h"
  };
#else
  value curr_instr;
#endif

#ifdef HAVE_THREADED_INTERPRETER
#ifdef HAVE_TRACING
  if (vmtrace)
    trace(c, thr);
#endif
  goto *(void *)(JTBASE + jumptbl[*TIPP(thr)++]);
#else
  for (;;) {
    curr_instr = *TIPP(thr)++;
    switch (FIX2INT(curr_instr)) {
#endif
    INST(inop):
      NEXT;
    INST(ipush):
      CPUSH(thr, TVALR(thr));
      NEXT;
    INST(ipop):
      TVALR(thr) = CPOP(thr);
      NEXT;
    INST(ildi):
      TVALR(thr) = *TIPP(thr)++;
      NEXT;
    INST(ildl): {
	value lidx = *TIPP(thr)++;
	TVALR(thr) = CODE_LITERAL(CLOS_CODE(TFUNR(thr)), FIX2INT(lidx));
      }
      NEXT;
    INST(ildg):
      /* XXX - unimplemented */
      NEXT;
    INST(istg):
      /* XXX - unimplemented */
      NEXT;
    INST(ilde):
      /* XXX - unimplemented */
      NEXT;
    INST(iste):
      /* XXX - unimplemented */
      NEXT;
    INST(imvarg):
      /* XXX - unimplemented */
      NEXT;
    INST(imvoarg):
      /* XXX - unimplemented */
      NEXT;
    INST(imvrarg):
      /* XXX - unimplemented */
      NEXT;
    INST(icont):
      /* XXX - unimplemented */
      NEXT;
    INST(ienv):
      /* XXX - unimplemented */
      NEXT;
    INST(iapply):
      {
	/* Set up the argc based on the call.  Everything else required
	   for function application has already been set up beforehand */
	TARGC(thr) = FIX2INT(*TIPP(thr)++);
	return(TR_FNAPP);
      }
      NEXT;
    INST(iret):
      /* Return to the trampoline, and make it restore the current
	 continuation in the continuation register */
      return(TR_RC);
      NEXT;
    INST(ijmp):
    INST(ijt):
    INST(ijf):
    INST(itrue):
      TVALR(thr) = CTRUE;
      NEXT;
    INST(inil):
      TVALR(thr) = CNIL;
      NEXT;
    INST(ihlt):
      TSTATE(thr) = Trelease;
      goto endquantum;
      NEXT;
    INST(iadd):
      TVALR(thr) = __arc_add2(c, CPOP(thr), TVALR(thr));
      NEXT;
    INST(isub):
      TVALR(thr) = __arc_sub2(c, CPOP(thr), TVALR(thr));
      NEXT;
    INST(imul):
      TVALR(thr) = __arc_mul2(c, CPOP(thr), TVALR(thr));
      NEXT;
    INST(idiv):
      TVALR(thr) = __arc_div2(c, CPOP(thr), TVALR(thr));
      NEXT;
    INST(icons):
      TVALR(thr) = cons(c, TVALR(thr), CPOP(thr));
      NEXT;
    INST(icar):
      if (NIL_P(TVALR(thr)))
	TVALR(thr) = CNIL;
      else if (TYPE(TVALR(thr)) != T_CONS)
	arc_err_cstrfmt(c, "can't take car of value");
      else
	TVALR(thr) = car(TVALR(thr));
      NEXT;
    INST(icdr):
      if (NIL_P(TVALR(thr)))
	TVALR(thr) = CNIL;
      else if (TYPE(TVALR(thr)) != T_CONS)
	arc_err_cstrfmt(c, "can't take cdr of value");
      else
	TVALR(thr) = cdr(TVALR(thr));
      NEXT;
    INST(iscar):
      scar(CPOP(thr), TVALR(thr));
      NEXT;
    INST(iscdr):
      scdr(CPOP(thr), TVALR(thr));
      NEXT;
    INST(ispl):
      {
	value list = TVALR(thr), nlist = CPOP(thr);
	/* Find the first cons in list whose cdr is not itself a cons.
	   Join the list from the stack to it. */
	if (list == CNIL) {
	  TVALR(thr) = nlist;
	} else {
	  for (;;) {
	    if (!CONS_P(cdr(list))) {
	      if (cdr(list) == CNIL)
		scdr(list, nlist);
	      else
		arc_err_cstrfmt(c, "splicing improper list");
	      break;
	    }
	    list = cdr(list);
	  }
	}
      }
      NEXT;
    INST(iis):
      TVALR(thr) = arc_is2(c, TVALR(thr), CPOP(thr));
      NEXT;
    INST(igt):
    INST(ilt):
    INST(idup):
      TVALR(thr) = *(TSP(thr)+1);
      NEXT;
    INST(icls):
    INST(iconsr):
      TVALR(thr) = cons(c, CPOP(thr), TVALR(thr));
      NEXT;
#ifndef HAVE_THREADED_INTERPRETER
    default:
#else
    INST(invalid):
#endif
      arc_err_cstrfmt(c, "invalid opcode %02x", *TIPP(thr));
#ifdef HAVE_THREADED_INTERPRETER
#else
    }
    if (--TQUANTA(thr) <= 0)
      goto endquantum;
  }
#endif

 endquantum:
  return(TR_SUSPEND);
}
