#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "connect_four.h"

// =======================================================
// Input + Display + Smart CPU (Minimax)
// Matches the flowcharts and adds CPU difficulty selection.
// =======================================================


// ---------- SAFE INTEGER INPUT ----------

static int readInt(const char *prompt, int *out) {
    char buf[128];

    printf("%s", prompt);
    if (!fgets(buf, sizeof(buf), stdin)) return 0;

    char *p = buf;
    while (isspace((unsigned char)*p)) p++;

    char *endptr;
    long val = strtol(p, &endptr, 10);

    if (p == endptr) return 0;

    while (isspace((unsigned char)*endptr)) endptr++;
    if (*endptr != '\0') return 0;

    *out = (int)val;
    return 1;
}


// ---------- MODE SELECTION ----------

int selectGameMode(void) {
    int mode = 0;
    while (mode != 1 && mode != 2) {
        printf("\nSelect mode:\n");
        printf("1) Human vs Human\n");
        printf("2) Human vs CPU (smart AI)\n");
        if (!readInt("Choice: ", &mode)) {
            printf("Invalid input. Please enter 1 or 2.\n");
        }
    }
    return mode;
}


// ---------- CPU DIFFICULTY SELECTION ----------

// Global (file-local) CPU depth used by minimax.
// Default is "Normal".
static int cpuDepth = 4;

int selectCPUDifficulty(void) {
    int choice = 0;
    while (choice < 1 || choice > 3) {
        printf("\nChoose CPU difficulty:\n");
        printf("1) Easy   (looks 3 moves ahead)\n");
        printf("2) Normal (looks 4 moves ahead)\n");
        printf("3) Hard   (looks 5 moves ahead)\n");

        if (!readInt("Difficulty: ", &choice)) {
            printf("Invalid input. Please enter 1, 2 or 3.\n");
            continue;
        }
    }

    if (choice == 1) cpuDepth = 3;
    if (choice == 2) cpuDepth = 4;
    if (choice == 3) cpuDepth = 5;

    printf("CPU difficulty set to depth %d.\n", cpuDepth);
    return cpuDepth;
}


// ---------- BOARD DISPLAY ----------

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


// ---------- MOVE VALIDATION ----------

int isMoveValid(char board[ROWS][COLS], int col) {
    if (col < 0 || col >= COLS) return 0;
    return board[0][col] == EMPTY;
}


// ---------- HUMAN MOVE ----------

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

        int col = col_input - 1;

        if (!isMoveValid(board, col)) {
            printf("Error: column %d is not valid or is full.\n", col_input);
            continue;
        }

        return col;
    }
}


// =======================================================
// SMART CPU (Minimax + Alpha-Beta)
// =======================================================

#define INF 1000000

// Center-first move order for better alpha-beta pruning
static const int columnOrder[COLS] = {3, 2, 4, 1, 5, 0, 6};

static int getLandingRow(char board[ROWS][COLS], int col) {
    if (col < 0 || col >= COLS) return -1;
    for (int r = ROWS - 1; r >= 0; r--) {
        if (board[r][col] == EMPTY) return r;
    }
    return -1;
}

static int hasWon(char board[ROWS][COLS], char piece) {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c <= COLS - 4; c++) {
            if (board[r][c] == piece &&
                board[r][c+1] == piece &&
                board[r][c+2] == piece &&
                board[r][c+3] == piece) return 1;
        }
    }
    for (int c = 0; c < COLS; c++) {
        for (int r = 0; r <= ROWS - 4; r++) {
            if (board[r][c] == piece &&
                board[r+1][c] == piece &&
                board[r+2][c] == piece &&
                board[r+3][c] == piece) return 1;
        }
    }
    for (int r = 0; r <= ROWS - 4; r++) {
        for (int c = 0; c <= COLS - 4; c++) {
            if (board[r][c] == piece &&
                board[r+1][c+1] == piece &&
                board[r+2][c+2] == piece &&
                board[r+3][c+3] == piece) return 1;
        }
    }
    for (int r = 3; r < ROWS; r++) {
        for (int c = 0; c <= COLS - 4; c++) {
            if (board[r][c] == piece &&
                board[r-1][c+1] == piece &&
                board[r-2][c+2] == piece &&
                board[r-3][c+3] == piece) return 1;
        }
    }
    return 0;
}

static int scoreWindow(char w[4], char cpu, char human) {
    int cpuCount = 0, humanCount = 0, emptyCount = 0;
    for (int i = 0; i < 4; i++) {
        if (w[i] == cpu) cpuCount++;
        else if (w[i] == human) humanCount++;
        else emptyCount++;
    }

    int score = 0;

    if (cpuCount == 4) score += 100000;
    else if (cpuCount == 3 && emptyCount == 1) score += 120;
    else if (cpuCount == 2 && emptyCount == 2) score += 10;

    if (humanCount == 3 && emptyCount == 1) score -= 140;
    else if (humanCount == 2 && emptyCount == 2) score -= 10;

    return score;
}

static int evaluateBoard(char board[ROWS][COLS], char cpu, char human) {
    int score = 0;
    char w[4];

    int center = COLS / 2;
    int centerCount = 0;
    for (int r = 0; r < ROWS; r++) {
        if (board[r][center] == cpu) centerCount++;
    }
    score += centerCount * 6;

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c <= COLS - 4; c++) {
            w[0]=board[r][c]; w[1]=board[r][c+1];
            w[2]=board[r][c+2]; w[3]=board[r][c+3];
            score += scoreWindow(w, cpu, human);
        }
    }

    for (int c = 0; c < COLS; c++) {
        for (int r = 0; r <= ROWS - 4; r++) {
            w[0]=board[r][c]; w[1]=board[r+1][c];
            w[2]=board[r+2][c]; w[3]=board[r+3][c];
            score += scoreWindow(w, cpu, human);
        }
    }

    for (int r = 0; r <= ROWS - 4; r++) {
        for (int c = 0; c <= COLS - 4; c++) {
            w[0]=board[r][c]; w[1]=board[r+1][c+1];
            w[2]=board[r+2][c+2]; w[3]=board[r+3][c+3];
            score += scoreWindow(w, cpu, human);
        }
    }

    for (int r = 3; r < ROWS; r++) {
        for (int c = 0; c <= COLS - 4; c++) {
            w[0]=board[r][c]; w[1]=board[r-1][c+1];
            w[2]=board[r-2][c+2]; w[3]=board[r-3][c+3];
            score += scoreWindow(w, cpu, human);
        }
    }

    return score;
}

static int hasAnyValidMove(char board[ROWS][COLS]) {
    for (int c = 0; c < COLS; c++) {
        if (isMoveValid(board, c)) return 1;
    }
    return 0;
}

static int minimax(char board[ROWS][COLS], int depth, int alpha, int beta,
                   int maximizingPlayer, char cpu, char human) {

    if (hasWon(board, cpu))   return  500000 + depth;
    if (hasWon(board, human)) return -500000 - depth;

    if (depth == 0 || !hasAnyValidMove(board)) {
        return evaluateBoard(board, cpu, human);
    }

    if (maximizingPlayer) {
        int bestVal = -INF;

        for (int i = 0; i < COLS; i++) {
            int c = columnOrder[i];
            int r = getLandingRow(board, c);
            if (r == -1) continue;

            board[r][c] = cpu;
            int val = minimax(board, depth - 1, alpha, beta, 0, cpu, human);
            board[r][c] = EMPTY;

            if (val > bestVal) bestVal = val;
            if (val > alpha) alpha = val;
            if (alpha >= beta) break;  // alpha-beta prune
        }
        return bestVal;
    } else {
        int bestVal = INF;

        for (int i = 0; i < COLS; i++) {
            int c = columnOrder[i];
            int r = getLandingRow(board, c);
            if (r == -1) continue;

            board[r][c] = human;
            int val = minimax(board, depth - 1, alpha, beta, 1, cpu, human);
            board[r][c] = EMPTY;

            if (val < bestVal) bestVal = val;
            if (val < beta) beta = val;
            if (alpha >= beta) break;  // alpha-beta prune
        }
        return bestVal;
    }
}


// ---------- CPU MOVE ----------

int getCPUMove(char board[ROWS][COLS], char cpuPiece) {
    char humanPiece = (cpuPiece == PLAYER1) ? PLAYER2 : PLAYER1;

    int bestScore = -INF;
    int bestCols[COLS];
    int bestCount = 0;

    // Use center-first order here as well
    for (int i = 0; i < COLS; i++) {
        int c = columnOrder[i];
        int r = getLandingRow(board, c);
        if (r == -1) continue;

        board[r][c] = cpuPiece;
        int score = minimax(board, cpuDepth - 1, -INF, INF, 0, cpuPiece, humanPiece);
        board[r][c] = EMPTY;

        if (score > bestScore) {
            bestScore = score;
            bestCount = 0;
            bestCols[bestCount++] = c;
        } else if (score == bestScore) {
            bestCols[bestCount++] = c;
        }
    }

    if (bestCount > 0) {
        return bestCols[rand() % bestCount];
    }

    // Fallback: just pick the first valid move in natural order
    for (int c = 0; c < COLS; c++) {
        if (isMoveValid(board, c)) return c;
    }
    return 0;
}