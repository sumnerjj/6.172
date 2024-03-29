/*
 * validator.c - 6.172 Malloc Validator
 *
 * Validates a malloc/free/realloc implementation defined in mm.c.
 *
 * Copyright (c) 2010, R. Bryant and D. O'Hallaron, All rights reserved.
 * May not be used, modified, or copied without permission.
 */
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "mdriver.h"
#include "memlib.h"
#include "mm.h"
#include "validator.h"

/* Returns true if p is ALIGNMENT-byte aligned */
#if (__WORDSIZE == 64 )
#define IS_ALIGNED(p)  ((((uint64_t)(p)) % ALIGNMENT) == 0)
#else
#define IS_ALIGNED(p)  ((((uint32_t)(p)) % ALIGNMENT) == 0)
#endif

/***************************
 * Range list data structure
 **************************/

/* Records the extent of each block's payload */
typedef struct range_t {
  char *lo;              /* low payload address */
  char *hi;              /* high payload address */
  struct range_t *next;  /* next list element */
} range_t;

/*****************************************************************
 * The following routines manipulate the range list, which keeps
 * track of the extent of every allocated block payload. We use the
 * range list to detect any overlapping allocated blocks.
 ****************************************************************/

/*
 * add_range - As directed by request opnum in trace tracenum,
 *     we've just called the student's malloc to allocate a block of
 *     size bytes at addr lo. After checking the block for correctness,
 *     we create a range struct for this block and add it to the range list.
 */
static int add_range(malloc_impl_t *impl, range_t **ranges, char *lo, int size,
                     int tracenum, int opnum)
{
  char *hi = lo + size - 1;
  range_t *p;

  /* You can use this as a buffer for writing messages with sprintf. */
  char msg[MAXLINE];

  assert(size > 0);

  /* Payload addresses must be ALIGNMENT-byte aligned */
  if (!IS_ALIGNED(lo)) {
    sprintf(msg, "Payload address (%p) not aligned to %d bytes",
            lo, ALIGNMENT);
    malloc_error(tracenum, opnum, msg);
    return 0;
  }

  /* The payload must lie within the extent of the heap */
  if ((lo < (char *)impl->heap_lo()) || (lo > (char *)impl->heap_hi()) ||
      (hi < (char *)impl->heap_lo()) || (hi > (char *)impl->heap_hi())) {
    sprintf(msg, "Payload (%p:%p) lies outside heap (%p:%p)",
            lo, hi, impl->heap_lo(), impl->heap_hi());
    malloc_error(tracenum, opnum, msg);
    return 0;
  }

  /* The payload must not overlap any other payloads */
  for (p = *ranges; p != NULL; p = p->next) {
    if (lo <= p->hi && hi >= p->lo) {
      sprintf(msg, "Payload (%p:%p) overlaps another payload (%p:%p)\n",
              lo, hi, p->lo, p->hi);
      malloc_error(tracenum, opnum, msg);
      return 0;
    }
  }

  /* Everything looks OK, so remember the extent of this block by creating a
   * range struct and adding it the range list.
   */
  if ((p = (range_t *)malloc(sizeof(range_t))) == NULL)
    unix_error("malloc error in add_range");
  p->next = *ranges;
  p->lo = lo;
  p->hi = hi;
  *ranges = p;
  return 1;
}

/*
 * remove_range - Free the range record of block whose payload starts at lo
 */
static void remove_range(range_t **ranges, char *lo)
{
  range_t *p;
  range_t **prevpp = ranges;
  int size;

  for (p = *ranges;  p != NULL; p = p->next) {
    if (p->lo == lo) {
      *prevpp = p->next;
      size = p->hi - p->lo + 1;
      free(p);
      break;
    }
    prevpp = &(p->next);
  }
}

/*
 * clear_ranges - free all of the range records for a trace
 */
static void clear_ranges(range_t **ranges)
{
  range_t *p;
  range_t *pnext;

  for (p = *ranges; p != NULL; p = pnext) {
    pnext = p->next;
    free(p);
  }
  *ranges = NULL;
}

/*
 * eval_mm_valid - Check the mm malloc package for correctness
 */
int eval_mm_valid(malloc_impl_t *impl, trace_t *trace, int tracenum)
{
  int i, j;
  int index;
  int size;
  int oldsize;
  char *newp;
  char *oldp;
  char *p;
  range_t *ranges = NULL;

  /* Reset the heap. */
  impl->reset_brk();

  /* Call the mm package's init function */
  if (impl->init() < 0) {
    malloc_error(tracenum, 0, "impl init failed.");
    return 0;
  }

  /* Interpret each operation in the trace in order */
  for (i = 0; i < trace->num_ops; i++) {
    index = trace->ops[i].index;
    size = trace->ops[i].size;

    switch (trace->ops[i].type) {

      case ALLOC: /* malloc */

        /* Call the student's malloc */
        if ((p = impl->malloc(size)) == NULL) {
          malloc_error(tracenum, i, "impl malloc failed.");
          return 0;
        }

        /*
         * Test the range of the new block for correctness and add it
         * to the range list if OK. The block must be  be aligned properly,
         * and must not overlap any currently allocated block.
         */
        if (add_range(impl, &ranges, p, size, tracenum, i) == 0)
          return 0;

        /* Fill the allocated region with some unique data that you can check
         * for if the region is copied via realloc.
         */
        memset(p, index & 0xFF, size);

        /* Remember region */
        trace->blocks[index] = p;
        trace->block_sizes[index] = size;
        break;

      case REALLOC: /* realloc */

        /* Call the student's realloc */
        oldp = trace->blocks[index];
        if ((newp = impl->realloc(oldp, size)) == NULL) {
          malloc_error(tracenum, i, "impl realloc failed.");
          return 0;
        }

        /* Remove the old region from the range list */
        remove_range(&ranges, oldp);

        /* Check new block for correctness and add it to range list */
        if (add_range(impl, &ranges, newp, size, tracenum, i) == 0)
          return 0;

        /* Make sure that the new block contains the data from the old block,
         * and then fill in the new block with new data that you can use to
         * verify the block was copied if it is resized again.
         */
        oldsize = trace->block_sizes[index];
        if (size < oldsize) oldsize = size;
        for (j = 0; j < oldsize; j++) {
          if (newp[j] != (index & 0xFF)) {
            malloc_error(tracenum, i, "impl realloc did not preserve the "
                         "data from old block");
            return 0;
          }
        }
        memset(newp, index & 0xFF, size);

        /* Remember region */
        trace->blocks[index] = newp;
        trace->block_sizes[index] = size;
        break;

      case FREE: /* free */

        /* Remove region from list and call student's free function */
        p = trace->blocks[index];
        remove_range(&ranges, p);
        impl->free(p);
        break;

      default:
        app_error("Nonexistent request type in eval_mm_valid");
    }

  }

  /* Free ranges allocated and reset the heap. */
  impl->reset_brk();
  clear_ranges(&ranges);

  /* As far as we know, this is a valid malloc package */
  return 1;
}
