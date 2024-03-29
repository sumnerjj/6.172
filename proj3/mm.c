/*
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload (that is, the entire block
 * of memory which is internally allocated is returned for the caller to use).
 * There are no headers or footers.  Blocks are never coalesced or even reused.
 * mm_realloc is implemented directly using mm_malloc and mm_free.
 *
 * Almost every line is commented in this reference implementation, and the
 * explanations are quite lengthy.  This is intended to help you figure out
 * what's going on without looking everything up.  You should assume that your
 * comments will be read by someone who is familiar with C and with the
 * standard library calls that this code makes.  To such a reader, many of the
 * comments below would simply be distracting.  Instead of explaining /how/
 * you're accomplishing a particular task, you should explain /what/ you're
 * accomplishing and /why/ that's useful.
 *
 * Note: Change this comment block to give a brief description of your allocator.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* All blocks must have a specified minimum alignment. */
#define ALIGNMENT 8

/* Rounds up to the nearest multiple of ALIGNMENT. */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

/* The smallest aligned size that will hold a size_t value. */
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*
 * mm_check - This checks our invariant that the size_t header before every
 * block points to either the beginning of the next block, or the end of the
 * heap.
 */
int mm_check(void)
{
  char *p;
  char *lo = (char*)mem_heap_lo();
  char *hi = (char*)mem_heap_hi() + 1;
  size_t size = 0;

  p = lo;
  while (lo <= p && p < hi) {
    size = ALIGN(*(size_t*)p + SIZE_T_SIZE);
    p += size;
  }

  if (p != hi) {
    printf("Bad headers did not end at heap_hi!\n");
    printf("heap_lo: %p, heap_hi: %p, size: %lu, p: %p\n", lo, hi, size, p);
    return -1;
  }

  return 0;
}

/*
 * mm_init - Initialize the malloc package.  Called once before any other
 * calls are made.  Since this is a very simple implementation, we just
 * return success.
 */
int mm_init(void)
{
  return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
  /* We allocate a little bit of extra memory so that we can store the
     size of the block we've allocated.  Take a look at mm_realloc to see
     one example of a place where this can come in handy. */
  int aligned_size = ALIGN(size + SIZE_T_SIZE);

  /* Expands the heap by the given number of bytes and returns a pointer to
     the newly-allocated area.  This is a slow call, so you will want to
     make sure you don't wind up calling it on every malloc. */
  void *p = mem_sbrk(aligned_size);

  if (p == (void *)-1) {
    /* Whoops, an error of some sort occurred.  We return NULL to let
       the client code know that we weren't able to allocate memory. */
    return NULL;
  } else {
    /* We store the size of the block we've allocated in the first
       SIZE_T_SIZE bytes. */
    *(size_t*)p = size;

    /* Then, we return a pointer to the rest of the block of memory,
       which is at least size bytes long.  We have to cast to uint8_t
       before we try any pointer arithmetic because voids have no size
       and so the compiler doesn't know how far to move the pointer.
       Since a uint8_t is always one byte, adding SIZE_T_SIZE after
       casting advances the pointer by SIZE_T_SIZE bytes. */
    return (void *)((char *)p + SIZE_T_SIZE);
  }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
  void *newptr;
  size_t copy_size;

  /* Allocate a new chunk of memory, and fail if that allocation fails. */
  newptr = mm_malloc(size);
  if (NULL == newptr)
    return NULL;

  /* Get the size of the old block of memory.  Take a peek at mm_malloc(),
     where we stashed this in the SIZE_T_SIZE bytes directly before the
     address we returned.  Now we can back up by that many bytes and read
     the size. */
  copy_size = *(size_t*)((uint8_t*)ptr - SIZE_T_SIZE);

  /* If the new block is smaller than the old one, we have to stop copying
     early so that we don't write off the end of the new block of memory. */
  if (size < copy_size)
    copy_size = size;

  /* This is a standard library call that performs a simple memory copy. */
  memcpy(newptr, ptr, copy_size);

  /* Release the old block. */
  mm_free(ptr);

  /* Return a pointer to the new block. */
  return newptr;
}
