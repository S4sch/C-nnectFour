#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "connect_four.h"

// Safe integer input via fgets + strtol.
// Returns 1 on success, 0 on invalid input.
static int readInt(const char *prompt, int *out) {
    char buf[128];

    printf("%s", prompt);
    if (!fgets(buf, sizeof(buf), stdin)) return 0;

    // Skip leading whitespace
    char *p = buf;
    while (isspace((unsigned char)*p)) p++;

    char *endptr;
    long val = strtol(p, &endptr, 10);

    // No digits read
    if (p == endptr) return 0;

    // Only allow trailing whitespace/newline
    while (isspace((unsigned char)*endptr)) endptr++;
    if (*endptr != '\0') return 0;

    *out = (int)val;
    return 1;
}

int selectGameMode(void) {
    int mode = 0;
    while (mode != 1 && mode != 2) {
        printf("\nSelect mode:\n");
        printf("1) Human vs Human\n");
        printf("2) Human vs CPU (random moves)\n");
        if (!readInt("Choice: ", &mode)) {
            printf("Invalid input. Please enter 1 or 2.\n");
        }
    }
    return mode;
}

void displayBoard(char board[ROWS][COLS]) {
    printf("\n  ");
    for (int c = 0; c < COLS; c++) {
        printf(" %d ", c + 1);
    }
    printf("\n");

    for (int r = 0; r < ROWS; r++) {
        printf(" |");
        for (int c = 0; c < COLS; c++) {
            printf(" %c ", board[r][c]);
        }
        printf("|\n");
    }

    printf("  ");
    for (int c = 0; c < COLS; c++) {
        printf("---");
    }
    printf("-\n\n");
}

int isMoveValid(char board[ROWS][COLS], int col) {
    if (col < 0 || col >= COLS) return 0;     // out of range
    return board[0][col] == EMPTY;           // top cell empty => column not full
}

int getHumanMove(char board[ROWS][COLS], char piece) {
    int col_input;

    while (1) {
        char prompt[64];
        snprintf(prompt, sizeof(prompt),
                 "Player %c, choose a column (1-%d): ", piece, COLS);

        if (!readInt(prompt, &col_input)) {
            printf("Error: please enter a number.\n");
            continue;
        }

        int col = col_input - 1; // convert to 0-based

        if (!isMoveValid(board, col)) {
            printf("Error: column %d is not valid or is full.\n", col_input);
            continue;
        }

        return col;
    }
}

// Very simple CPU: pick any random valid column.
int getCPUMove(char board[ROWS][COLS], char piece) {
    (void)piece; // piece not needed for random AI

    int col;
    do {
        col = rand() % COLS;
    } while (!isMoveValid(board, col));

    return col;
}
