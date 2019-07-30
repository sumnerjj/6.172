#include "util.h"

#include "gtest/gtest.h"

namespace {

// gtest is smart enough to compare doubles that are "almost" equal, but it
// doesn't work for arrays of doubles, so we provide this helper.  You'd think
// this should be a function, but gtest macros use "return", so that doesn't
// work.
#define EXPECT_MAT_EQ(expected, actual) \
   do { \
      for (int i = 0; i < 4; i++) { \
         for (int j = 0; j < 4; j++) { \
            EXPECT_EQ(expected[i][j], actual[i][j]); \
         } \
      } \
   } while (0)

TEST(UtilTest, matrixMulIdentity) {
   Matrix4x4 A;
   Matrix4x4 B;
   Matrix4x4 C;
   initMatrix(A);
   initMatrix(B);
   // C = A * B
   mulMatrix(C, A, B);
   Matrix4x4 expected = {
      {1, 0, 0, 0},
      {0, 1, 0, 0},
      {0, 0, 1, 0},
      {0, 0, 0, 1},
   };
   EXPECT_MAT_EQ(expected, C);
}

// Tests that Foo does Xyz.
TEST(UtilTest, matrixMulSimple) {
   Matrix4x4 A = {
      {1, 2, 3, 4},
      {1, 2, 3, 4},
      {1, 2, 3, 4},
      {1, 2, 3, 4},
   };
   Matrix4x4 B;
   memcpy(B, A, sizeof(A));
   Matrix4x4 C;
   // C = A * B
   mulMatrix(C, A, B);
   Matrix4x4 expected = {
      {10, 20, 30, 40},
      {10, 20, 30, 40},
      {10, 20, 30, 40},
      {10, 20, 30, 40},
   };
   EXPECT_MAT_EQ(expected, C);
}

}  // namespace
