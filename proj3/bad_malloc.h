#ifndef MM_BAD_MALLOC_H
#define MM_BAD_MALLOC_H

#include <unistd.h>

int bad_init(void);
void *bad_malloc(size_t size);
void bad_free(void *ptr);
void *bad_realloc(void *ptr, size_t size);
int bad_check(void);

#endif /* MM_BAD_MALLOC_H */
