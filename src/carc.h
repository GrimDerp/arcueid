/* 
  Copyright (C) 2009 Rafael R. Sevilla

  This file is part of CArc

  CArc is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA
  02110-1301 USA.
*/

#ifndef _CARC_H_

#define _CARC_H_

#include <inttypes.h>

typedef struct carc carc;
typedef unsigned long value;

#define VALUE_SIZE (sizeof(value))
#define FIXNUM_MAX ((1 << ((VALUE_SIZE << 3) - 2)) - 1)
#define FIXNUM_MIN (-FIXNUM_MAX - 1)
#define FIXNUM_FLAG 0x01
#define INT2FIX(i) ((value)(((long)(i))<< 1 | FIXNUM_FLAG))
/* FIXME: portability to systems that don't preserve sign bit on
   right shifts. */
#define FIX2INT(x) ((long)(x) >> 1)

typedef void * (*func_alloc)(size_t);
typedef void (*func_dealloc)(void *);

extern carc *carc_init_new();
extern carc *carc_init_new_custom_alloc(func_alloc malloc, func_dealloc free);
extern int carc_init(carc *c);
extern int carc_init_custom_alloc(carc *c, func_alloc, func_dealloc);
extern void carc_deinit(carc *c);
extern void carc_load_file(carc *c, FILE *fp);
extern void carc_load_string(carc *c, const char *cmd);
extern value carc_apply1(carc *c, const char *procname, value arg);

#endif
