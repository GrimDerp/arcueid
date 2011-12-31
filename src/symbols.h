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

#ifndef _SYMBOLS_H_

#define _SYMBOLS_H_

enum builtin_syms {
  S_FN,				/* fn syntax */
  S_US,				/* underscore */
  S_QUOTE,			/* quote */
  S_QQUOTE,			/* quasiquote */
  S_UNQUOTE,			/* unquote */
  S_UNQUOTESP,			/* unquote-splicing */
  S_COMPOSE,			/* compose */
  S_COMPLEMENT,			/* complement */
  S_T,				/* true */
  S_NIL,			/* nil */
  S_NO,				/* no */
  S_ANDF,			/* andf */
  S_GET,			/* get */
  S_SYM,			/* sym */
  S_FIXNUM,			/* fixnum */
  S_BIGNUM,			/* bignum */
  S_FLONUM,			/* flonum */
  S_RATIONAL,			/* rational */
  S_COMPLEX,			/* complex */
  S_CHAR,			/* char */
  S_STRING,			/* string */
  S_CONS,			/* cons */
  S_TABLE,			/* table */
  S_INPUT,			/* input */
  S_OUTPUT,			/* output */
  S_EXCEPTION,			/* exception */
  S_PORT,			/* port */
  S_THREAD,			/* thread */
  S_VECTOR,			/* vector */
  S_CONTINUATION,		/* continuation */
  S_CLOSURE,			/* closure */
  S_CODE,			/* code */
  S_ENVIRONMENT,		/* environment */
  S_VMCODE,			/* vmcode */
  S_CCODE,			/* ccode */
  S_CUSTOM,			/* custom */
  S_INT,			/* int */
  S_UNKNOWN,			/* unknown */
  S_RE,				/* re */
  S_IM,				/* im */
  S_NUM,			/* num */
  S_SIG,			/* sig */
  S_STDIN_FD,			/* stdin-fd */
  S_STDOUT_FD,			/* stdout-fd */
  S_STDERR_FD,			/* stderr-fd */
  S_MAC,			/* mac */
  S_IF,				/* if */
  S_ASSIGN,			/* assign */
  S_O,				/* o */
  S_DOT,			/* . */
  S_CAR,			/* car */
  S_CDR,			/* cdr */
  S_SCAR,			/* scar */
  S_SCDR,			/* scdr */
  S_IS,				/* is */
  S_PLUS,			/* + */
  S_MINUS,			/* - */
  S_TIMES,			/* * */
  S_DIV,			/* / */

  S_THE_END			/* end of the line */
};

#define ARC_BUILTIN(c, sym) (VINDEX((c)->builtin, sym))

#endif
