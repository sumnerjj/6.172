#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* Typedefs */

typedef uint32_t pixel_t;

/* Function declarations */

int getIndex(int x, int y, int d);

/* Function definitions */

// This is the function that does the actual rotation.
// Only time spent in this function is reported by the testbed.
void rotate_main(pixel_t *dest, const pixel_t *src, int n) 
{
  int i, j;
  for (i = 0; i < n; i++) {
    for (j = 0; j < n; j++) {
      dest[getIndex(j, i, n)] = src[getIndex(i, n - 1 - j, n)];
    }
  }
}

/* Helper function definitions

   These functions must be modified if you make any changes
   to the data layout of the matrices being rotated. They are
   helper functions used during allocation, initialization,
   printing, and correctness checking. */

// This function returns the number of bytes needed to store
// an nxn matrix.
uint64_t getAllocationSize(int n)
{
  return n * n * sizeof(pixel_t);
}

// This function translates a 2D index pair (x, y) into a linear array index.
// Assumes matrix is stored in row-major order and the row stride is d.
int getIndex(int x, int y, int d)
{
  return x * d + y;
}

