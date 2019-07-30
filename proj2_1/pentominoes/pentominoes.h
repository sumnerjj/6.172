#ifndef PENTOMINOES_H
#define PENTOMINOES_H

/**
 * Copyright (c) 2010 6.172 Staff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 **/

#include <stdint.h>


#include <stdbool.h>


/* Board datatype.
 */
typedef struct stboard board_t;

/* Pieces:
 *
 * Pieces are represented by type pentomino_t. A capital letter represents the
 * name of the piece, and an array of 5 points defines an absolute placement of
 * the piece starting at (0,0).
 *
 * For example, for the X piece, if we are placing that piece at (3,3), then
 * location of the pieces are:
 *  (3,3), (3+1, 3+7), (3+1, 3+0), (3+1, 3+1), and (3+2, 3+0)
 * ... all mod 8.
 * */
typedef struct pentomino_t pentomino_t;

board_t *board_new(void);

board_t *board_new_frompoints(uint8_t ex1, uint8_t ey1,
                              uint8_t ex2, uint8_t ey2,
                              uint8_t ex3, uint8_t ey3,
                              uint8_t ex4, uint8_t ey4);

void board_free(board_t *board);

/* This function is used by the test code to read your board.
 */
char board_get_square(board_t *board, int x, int y);

/* Defined in main.c */
void print_board(board_t *board);
typedef bool (*solution_handler_t)(board_t *);

/* Return true if search was stopped before entire solution space was
 * searched. */
bool solve(board_t *board, solution_handler_t cb);

#endif /* PENTOMINOES_H */
