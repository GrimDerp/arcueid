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

/* Mark the car and cdr */
static void clos_marker(arc *c, value v, int depth,
			void (*markfn)(arc *, value, int))
{
  markfn(c, car(v), depth);
  markfn(c, cdr(v), depth);
}

value arc_mkclos(arc *c, value code, value env)
{
  value cl;

  cl = cons(c, code, env);
  ((struct cell *)cl)->_type = T_CLOS;
  return(cl);
}

static int clos_apply(arc *c, value thr, value clos)
{
  value code, env;

  code = car(clos);
  env = cdr(clos);
  /* Set up the registers to make this code execute */
  TIPP(thr) = &VINDEX(code, 0);
  TENVR(thr) = env;
  TFUNR(thr) = clos;
  /* Return to the trampoline to make it resume */
  return(TR_RESUME);
}

typefn_t __arc_clos_typefn__ = {
  clos_marker,
  __arc_null_sweeper,
  NULL,
  NULL,
  NULL,
  NULL,
  clos_apply,
  NULL
};