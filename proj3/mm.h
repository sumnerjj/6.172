#ifndef MM_MM_H
#define MM_MM_H

#include <unistd.h>

int mm_check(void);
int mm_init(void);
void *mm_malloc(size_t size);
void mm_free(void *ptr);
void *mm_realloc(void *ptr, size_t size);

#endif /* MM_MM_H */
