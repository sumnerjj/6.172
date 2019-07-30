/*
 * In this broken malloc implementation, every block is allocated to be
 * BAD_SIZE bytes, and no alignment is performed.  This should trip two checks
 * in a good validator: the alignment check, and the overlapping regions check
 * when the allocation size is above BAD_SIZE.
 */

#include "bad_malloc.h"
#include "memlib.h"

#define BAD_SIZE 4101

/*
 * bad_init - Does nothing.
 */
int bad_init(void)
{
  return 0;
}

/*
 * bad_check - No checker.
 */
int bad_check(void)
{
  return 1;
}

/*
 * bad_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is not a multiple of the alignment,
 *     and may not fit the requested allocation size.
 */
void *bad_malloc(size_t size)
{
  size = BAD_SIZE;  /* Overrides input, and has bad aligmnent. */

  void *p = mem_sbrk(size);

  if (p == (void *)-1) {
    /* The heap is probably full, so return NULL. */
    return NULL;
  } else {
    return p;
  }
}

/*
 * bad_free - Freeing a block does nothing.
 */
void bad_free(void *ptr)
{
}

/*
 * bad_realloc - Implemented simply in terms of bad_malloc and bad_free, but lacks
 *      copy step.
 */
void *bad_realloc(void *ptr, size_t size)
{
  void *newptr;

  /* Allocate a new chunk of memory, and fail if that allocation fails. */
  newptr = bad_malloc(size);
  if (NULL == newptr)
    return NULL;

  /* Release the old block. */
  bad_free(ptr);

  /* Return a pointer to the new block. */
  return newptr;
}
