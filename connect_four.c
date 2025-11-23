#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "connect_four.h"

// Count consecutive pieces in a direction (dr, dc),
// starting from (start_r, start_c), inclusive.
static int countDirection(char board[ROWS][COLS], char piece,
                          int start_r, int start_c, int dr, int dc) {
    int count = 0;
    int r = start_r;
    int c = start_c;

    while (r >= 0 && r < ROWS && c >= 0 && c < COLS && board[r][c] == piece) {
        count++;
        r += dr;
        c += dc;
    }
    return count;
}

void initializeBoard(char board[ROWS][COLS]) {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            board[r][c] = EMPTY;
        }
    }
}

// Drops a piece into column col.
// Returns the row where the piece landed, or -1 if column is full.
int dropPiece(char board[ROWS][COLS], int col, char piece) {
    for (int r = ROWS - 1; r >= 0; r--) {
        if (board[r][col] == EMPTY) {
            board[r][col] = piece;
            return r;
        }
    }
    return -1;
}

int isBoardFull(char board[ROWS][COLS]) {
    // If top row has no EMPTY, no moves left.
    for (int c = 0; c < COLS; c++) {
        if (board[0][c] == EMPTY) return 0;
    }
    return 1;
}

// Check win only around the last placed piece.
// Much simpler and faster than scanning everything.
int checkWin(char board[ROWS][COLS], char piece, int last_row, int last_col) {
    // Horizontal
    int horiz = countDirection(board, piece, last_row, last_col, 0, 1) +
                countDirection(board, piece, last_row, last_col, 0, -1) - 1;
    if (horiz >= 4) return 1;

    // Vertical
    int vert = countDirection(board, piece, last_row, last_col, 1, 0) +
               countDirection(board, piece, last_row, last_col, -1, 0) - 1;
    if (vert >= 4) return 1;

    // Diagonal down-right / up-left
    int diag1 = countDirection(board, piece, last_row, last_col, 1, 1) +
                countDirection(board, piece, last_row, last_col, -1, -1) - 1;
    if (diag1 >= 4) return 1;

    // Diagonal down-left / up-right
    int diag2 = countDirection(board, piece, last_row, last_col, 1, -1) +
                countDirection(board, piece, last_row, last_col, -1, 1) - 1;
    if (diag2 >= 4) return 1;

    return 0;
}

int main(void) {
    char board[ROWS][COLS];
    initializeBoard(board);

    // Seed RNG for CPU random moves
    srand((unsigned int)time(NULL));

    int mode = selectGameMode(); // 1 = HvH, 2 = HvCPU
    char currentPlayer = PLAYER1;

    while (1) {
        displayBoard(board);

        int col;
        if (mode == 2 && currentPlayer == PLAYER2) {
            // CPU turn
            col = getCPUMove(board, currentPlayer);
            printf("CPU chooses column %d\n", col + 1);
        } else {
            // Human turn
            col = getHumanMove(board, currentPlayer);
        }

        int row = dropPiece(board, col, currentPlayer);

        // Win check after drop
        if (checkWin(board, currentPlayer, row, col)) {
            displayBoard(board);
            if (mode == 2 && currentPlayer == PLAYER2)
                printf("CPU (%c) wins!\n", currentPlayer);
            else
                printf("Player %c wins!\n", currentPlayer);
            break;
        }

        // Draw check
        if (isBoardFull(board)) {
            displayBoard(board);
            printf("It's a draw!\n");
            break;
        }

        // Switch player
        currentPlayer = (currentPlayer == PLAYER1) ? PLAYER2 : PLAYER1;
    }

    return 0;
}
