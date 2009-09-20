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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>
#include "carc.h"

/* These magic numbers are essentially the same as that used by
   Inferno in its memory allocator. */
#define MAGIC_A 0xa110c		      /* Allocated block */
#define MAGIC_F	0xbadc0c0a	      /* Free block */
#define MAGIC_E	0xdeadbabe	      /* End of arena */
#define MAGIC_I	0xabba		      /* Block is immutable (non-GC) */

typedef struct Bhdr_t {
  uint64_t magic;
  uint64_t size;
  uint64_t color;
  uint64_t pad;	    /* so that the block header is exactly 16 bytes */
  union {
    char data[1];
    struct Bhdr_t *next;
  } u;
} Bhdr;

#define B2D(bp) ((void *)bp->u.data)
#define D2B(b, dp) ((b) = ((Bhdr *)(((char *)dp) - ((Bhdr *)0)->u.data))
#define B2NB(b) ((Bhdr *)((char *)(b) + (b)->size))
#define FBNEXT(b) ((b)->u.next)
#define BHDRSIZE ((long)(((Bhdr *)0)->u.data))
/* round the heap size */
#define ROUNDSIZE(ns, s) { (ns) = ((s) & 0x0f); (ns) = ((ns) < (s)) ? ((ns) + 0x10) : ns; }
  

static Bhdr *fl_head = NULL;
static Bhdr *fl_prev = NULL;
static uint64_t epoch = 3;
static int mutator = 0;
static int marker = 1;
static int sweeper = 2;
#define propagator 3		/* propagator color */

/* This function takes an eligible block and carves out of it a portion
   of at least [size].  There are three cases:

   1. The free block is exactly the size requested.  Detach it from
      the free list and return it. 
   2. The additional space after carving out the free block is less than
      or equal to the size of the header.  The remaining space becomes
      slack (internal fragmentation), and the block is returned as is.
   3. The block is big enough.  Split it in two, and return the
      right-side block.
   In all cases the new allocated block is right-justified in the free
   block, in the higher address portions.  This way, the linking of the
   free list does not change in case 3.
 */
static void *fl_get_block(size_t size, Bhdr *prev, Bhdr *cur)
{
  Bhdr *h;

  if (cur->size <= (uint64_t)(size + BHDRSIZE)) {
    /* Cases 1 and 2, unlink the block and use it as is. */
    h = cur;
    FBNEXT(prev) = FBNEXT(cur);
  } else {
    /* Case 3, the block is bigger than what we need.  Shrink the
       current block by the size plus the size of the block header.
       Since we are carving away the right-hand side, the linkage
       of the free list does not change. */
    cur->size -= size + BHDRSIZE;
    h = B2NB(cur);
  }
  fl_prev = prev;
  h->size = size;
  h->magic = MAGIC_A;
  h->color = mutator;
  return(B2D(h));
}

static void fl_add_block(void *blk)
{
}

static void *fl_alloc(size_t size)
{
  Bhdr *prev, *cur;

  /* Search from [fl_prev] to the end of the list */
  prev = fl_prev;
  cur = FBNEXT(prev);
  while (cur != NULL) {
    if (cur->size >= size) {
      return(fl_get_block(size, prev, cur));
    }
    prev = cur;
    cur = FBNEXT(prev);
  }
  /* Search from the start of the list to fl_prev */
  prev = fl_head;
  cur = FBNEXT(prev);
  while (prev != fl_prev) {
    if (cur->size >= size) {
      return(fl_get_block(size, prev, cur));
    }
    prev = cur;
    cur = FBNEXT(prev);
  }
  /* No block found */
  return(NULL);
}

/* Allocate more memory using malloc/mmap */
static void *expand_heap(size_t request)
{
  /* XXX fill this in */
  return(NULL);
}

static void *alloc(carc *c, size_t osize)
{
  void *blk, *nblk;
  size_t size;

  /* Adjust the size of the allocated block so that we maintain
     alignment of at least 16 bytes. */
  ROUNDSIZE(size, osize);
  blk = fl_alloc(size);
  if (blk == NULL) {
    nblk = expand_heap(size);
    if (nblk == NULL) {
      c->signal_error(c, "Fatal error: out of memory!");
      return(NULL);
    }
    fl_add_block(nblk);
    blk = fl_alloc(size);
  }
  return(blk);
}

static value get_cell(carc *c)
{
  void *cellptr;

  cellptr = alloc(c, sizeof(struct cell));
  if (cellptr == NULL)
    return(CNIL);
  return((value)cellptr);
}

void carc_set_allocator(carc *c)
{
  c->get_cell = get_cell;
  c->mem_alloc = alloc;
  epoch = 3;
  mutator = 0;
  marker = 1;
  sweeper = 2;
}
