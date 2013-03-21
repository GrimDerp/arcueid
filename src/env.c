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

/* Arcueid's environments come in two flavours:
   1. Stack-based environments
   2. Heap-based environments

   Function application in Arcueid basically boils down to doing the
   following:

   1. Create a continuation, saving the contents of the environment
      register, instruction pointer (setting it up so it points to
      just after the actual apply instruction), and the current value
      of the continuation register.  This goes on the stack.
   2. Push the arguments of the continuation onto the stack.
   3. Put the function itself (which can be either a closure or a
      foreign function) into the value register.
   4. Transfer control to the called function.

   It is to be noted that after the saved continuation on the stack,
   the passed parameters may actually be used as the environment
   itself once control is transferred to the applied function,
   with some additional information, most importantly the actual
   size of the environment, which is stored as a number just after
   the last stack entry and a pointer to the lexically superior
   environment, if any.

   If an environment needs to be saved on the heap it is copied into a
   vector, and all lexically superior environments need to be similarly
   copied.  Essentially boils down to crunching the stack so that any
   environments that were directly stored on it are copied to the heap.

   This may happen as well when the stack actually fills up.  All
   environments on the stack may be moved to the heap.

   This strategy is what Clinger et. al. [1] call the stack/heap
   strategy.

   Environments that have been saved on the heap are Arcueid vectors
   with a specific structure:

   [1] William D. Clinger, Anne H. Hartheimer, and Eric M. Ost.
       Implementation Strategies for continuations.  In Proceedings
       Record of the 1988 ACM Symposium on Lisp and Functional
       Programming, ACM, 1988.
 */
#include "arcueid.h"
#include "vmengine.h"

/* Make a new environment on the stack.  The prevsize parameter is the
   count of objects already on the stack that become part of the
   environment that is being created, and extrasize are the additional
   elements on the stack that are to be added.  These additional
   elements have the initial value CUNBOUND. */
void __arc_mkenv(arc *c, value thr, int prevsize, int extrasize)
{
  value *envstart;
  int i, esofs;

  /* Add the extra environment entries */
  for (i=0; i<extrasize; i++)
    CPUSH(thr, CUNBOUND);
  /* Add the count */
  CPUSH(thr, INT2FIX(prevsize+extrasize));
  envstart = TSP(thr);
  CPUSH(thr, TENVR(thr));
  esofs = envstart - TSBASE(thr);
  TENVR(thr) = ((value)((long)(esofs << 4) | ENV_FLAG));
}

#define SENV_PTR(stkbase, x) (((int)((env) >> 4)) + (stkbase))
#define SENV_COUNT(base) (FIX2INT(*(base + 1)))

#define VENV_NEXT(x) (VINDEX((x), 0))
#define VENV_INDEX(x, i) (VINDEX((x), (i)+1))

static value nextenv(arc *c, value thr, value env)
{
  value *envptr;

  if (env == CNIL)
    return(CNIL);

  if (TYPE(env) == T_VECTOR)
    return(VENV_NEXT(env));

  /* We have a stack-based environment.  Get the address of the
     environment pointer from the stack. */
  envptr = SENV_PTR(TSBASE(thr), env);
  return(*envptr);
}

/* Get a value from the environment pointer given. */
value *__arc_getenv(arc *c, value thr, int depth, int index)
{
  value *senv, *senvstart, env;
  int count;

  env = TENVR(thr);

  /* Find the environment frame being referenced */
  while (depth-- > 0 && !NIL_P(env))
    env = nextenv(c, thr, env);

  if (NIL_P(env))
    return(CNIL);

  if (TYPE(env) == T_VECTOR)
    return(&VENV_INDEX(env, index));

  /* For a stack-based environment, we have to do some gymnastics */
  senv = SENV_PTR(TSBASE(thr), env);
  count = SENV_COUNT(senv);
  senvstart = senv + count + 1;
  return(senvstart - index);
}