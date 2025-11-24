#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "connect_four.h"

// =======================================================
// Input + Display + Smart CPU (Minimax)
// Matches the flowcharts and adds CPU difficulty selection.
// =======================================================


// Enable/disable color threat highlighting.
static int gColorMode = 0;

// Simple ANSI color codes
#define CLR_RESET        "\x1b[0m"
#define CLR_P1           "\x1b[31m"  // red for PLAYER1
#define CLR_P2           "\x1b[34m"  // blue for PLAYER2
#define CLR_THREAT_EMPTY "\x1b[33m"  // yellow for empty threat cells
#define CLR_RESET "\x1b[0m"
#define CLR_WIN   "\x1b[32m" 


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

int selectColorMode(void) {
    int choice = 0;
    while (choice != 1 && choice != 2) {
        printf("\nColor mode:\n");
        printf("1) Off (plain board)\n");
        printf("2) On  (show threats in color)\n");
        if (!readInt("Choice: ", &choice)) {
            printf("Invalid input. Please enter 1 or 2.\n");
        }
    }
    gColorMode = (choice == 2);
    printf("Color mode %s.\n", gColorMode ? "ON" : "OFF");
    return gColorMode;
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
    while (choice < 1 || choice > 4) {
        printf("\nChoose CPU difficulty:\n");
        printf("1) Easy       (looks 3 moves ahead)\n");
        printf("2) Normal     (looks 4 moves ahead)\n");
        printf("3) Hard       (looks 5 moves ahead)\n");
        printf("4) Almost Perfect    (looks 8 moves ahead, may be slow)\n");

        if (!readInt("Difficulty: ", &choice)) {
            printf("Invalid input. Please enter 1, 2, 3 or 4.\n");
            continue;
        }
    }

    if (choice == 1)      cpuDepth = 3;
    else if (choice == 2) cpuDepth = 4;
    else if (choice == 3) cpuDepth = 5;
    else if (choice == 4) cpuDepth = 8;

    printf("CPU difficulty set to depth %d.\n", cpuDepth);
    return cpuDepth;
}

//Shows the winning combination in Green
static void computeWinMask(char board[ROWS][COLS],
                           char piece,
                           int last_row, int last_col,
                           int winMask[ROWS][COLS]) {
    // Clear mask
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            winMask[r][c] = 0;
        }
    }

    // Directions: horiz, vert, diag down-right, diag up-right
    const int dr[4] = { 0,  1,  1, -1};
    const int dc[4] = { 1,  0,  1,  1};

    // Try each direction; for each, slide a 4-length window that includes (last_row,last_col)
    for (int d = 0; d < 4; d++) {
        int dir_r = dr[d];
        int dir_c = dc[d];

        // Offsets so that the 4-cell window includes (last_row,last_col)
        for (int offset = -3; offset <= 0; offset++) {
            int r0 = last_row + offset * dir_r;
            int c0 = last_col + offset * dir_c;

            // Check if 4 cells starting at (r0,c0) are all on-board and match `piece`
            int count = 0;
            for (int i = 0; i < 4; i++) {
                int rr = r0 + i * dir_r;
                int cc = c0 + i * dir_c;

                if (rr < 0 || rr >= ROWS || cc < 0 || cc >= COLS) {
                    count = -1;
                    break;
                }
                if (board[rr][cc] != piece) {
                    count = -1;
                    break;
                }
                count++;
            }

            if (count == 4) {
                // Mark winning cells
                for (int i = 0; i < 4; i++) {
                    int rr = r0 + i * dir_r;
                    int cc = c0 + i * dir_c;
                    winMask[rr][cc] = 1;
                }
                return; // only one winning line needed
            }
        }
    }
}

void displayBoardWin(char board[ROWS][COLS], char winner,
                     int last_row, int last_col) {
    int winMask[ROWS][COLS];
    computeWinMask(board, winner, last_row, last_col, winMask);

    // Always ignore color mode and threat coloring here:
    // only show winning four in green, others plain.

    printf("\n  ");
    for (int c = 0; c < COLS; c++) {
        printf(" %d ", c + 1);
    }
    printf("\n");

    for (int r = 0; r < ROWS; r++) {
        printf(" |");
        for (int c = 0; c < COLS; c++) {
            char cell = board[r][c];
            if (winMask[r][c]) {
                // Winning four -> green
                printf(" %s%c%s ", CLR_WIN, cell, CLR_RESET);
            } else {
                // Everything else plain, no other colors
                printf(" %c ", cell);
            }
        }
        printf("|\n");
    }

    printf("  ");
    for (int c = 0; c < COLS; c++) {
        printf("---");
    }
    printf("-\n\n");
}

//For color mode, checking combinations of 3 and shows them with color
static void computeThreatMasks(char board[ROWS][COLS],
                               int threatP1[ROWS][COLS],
                               int threatP2[ROWS][COLS]) {
    // Clear masks
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            threatP1[r][c] = 0;
            threatP2[r][c] = 0;
        }
    }

    // Helper lambda-ish macro for marking a window
    #define MARK_WINDOW(r0, c0, dr, dc, player, mask)           \
        do {                                                   \
            for (int i = 0; i < 4; i++) {                      \
                int rr = (r0) + i * (dr);                      \
                int cc = (c0) + i * (dc);                      \
                mask[rr][cc] = 1;                              \
            }                                                  \
        } while (0)

    // Scan all windows of length 4
    // Horizontal
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c <= COLS - 4; c++) {
            int p1 = 0, p2 = 0, empty = 0;
            for (int i = 0; i < 4; i++) {
                char cell = board[r][c + i];
                if (cell == PLAYER1) p1++;
                else if (cell == PLAYER2) p2++;
                else empty++;
            }
            // Threat = 3 in a row + 1 empty, with no opponent pieces
            if (p1 == 3 && p2 == 0 && empty == 1) {
                MARK_WINDOW(r, c, 0, 1, PLAYER1, threatP1);
            } else if (p2 == 3 && p1 == 0 && empty == 1) {
                MARK_WINDOW(r, c, 0, 1, PLAYER2, threatP2);
            }
        }
    }

    // Vertical
    for (int c = 0; c < COLS; c++) {
        for (int r = 0; r <= ROWS - 4; r++) {
            int p1 = 0, p2 = 0, empty = 0;
            for (int i = 0; i < 4; i++) {
                char cell = board[r + i][c];
                if (cell == PLAYER1) p1++;
                else if (cell == PLAYER2) p2++;
                else empty++;
            }
            if (p1 == 3 && p2 == 0 && empty == 1) {
                MARK_WINDOW(r, c, 1, 0, PLAYER1, threatP1);
            } else if (p2 == 3 && p1 == 0 && empty == 1) {
                MARK_WINDOW(r, c, 1, 0, PLAYER2, threatP2);
            }
        }
    }

    // Diagonal down-right
    for (int r = 0; r <= ROWS - 4; r++) {
        for (int c = 0; c <= COLS - 4; c++) {
            int p1 = 0, p2 = 0, empty = 0;
            for (int i = 0; i < 4; i++) {
                char cell = board[r + i][c + i];
                if (cell == PLAYER1) p1++;
                else if (cell == PLAYER2) p2++;
                else empty++;
            }
            if (p1 == 3 && p2 == 0 && empty == 1) {
                MARK_WINDOW(r, c, 1, 1, PLAYER1, threatP1);
            } else if (p2 == 3 && p1 == 0 && empty == 1) {
                MARK_WINDOW(r, c, 1, 1, PLAYER2, threatP2);
            }
        }
    }

    // Diagonal up-right
    for (int r = 3; r < ROWS; r++) {
        for (int c = 0; c <= COLS - 4; c++) {
            int p1 = 0, p2 = 0, empty = 0;
            for (int i = 0; i < 4; i++) {
                char cell = board[r - i][c + i];
                if (cell == PLAYER1) p1++;
                else if (cell == PLAYER2) p2++;
                else empty++;
            }
            if (p1 == 3 && p2 == 0 && empty == 1) {
                MARK_WINDOW(r, c, -1, 1, PLAYER1, threatP1);
            } else if (p2 == 3 && p1 == 0 && empty == 1) {
                MARK_WINDOW(r, c, -1, 1, PLAYER2, threatP2);
            }
        }
    }

    #undef MARK_WINDOW
}



// ---------- BOARD DISPLAY ----------

void displayBoard(char board[ROWS][COLS]) {
    // If color mode is off, use simple old-style display
    if (!gColorMode) {
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
        return;
    }

    // Color mode: compute threat masks
    int threatP1[ROWS][COLS];
    int threatP2[ROWS][COLS];
    computeThreatMasks(board, threatP1, threatP2);

    printf("\n  ");
    for (int c = 0; c < COLS; c++) {
        printf(" %d ", c + 1);
    }
    printf("\n");

    for (int r = 0; r < ROWS; r++) {
        printf(" |");
        for (int c = 0; c < COLS; c++) {
            char cell = board[r][c];
            const char *color = CLR_RESET;

            if (cell == PLAYER1 && threatP1[r][c]) {
                color = CLR_P1;                 // red X in a threat line
            } else if (cell == PLAYER2 && threatP2[r][c]) {
                color = CLR_P2;                 // blue O in a threat line
            } else if (cell == EMPTY && (threatP1[r][c] || threatP2[r][c])) {
                color = CLR_THREAT_EMPTY;       // yellow '.' that would complete 4
            } else {
                color = CLR_RESET;
            }

            printf(" %s%c%s ", color, cell, CLR_RESET);
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

// ---------- Gravity + double-threat aware evaluation ----------

// Is this empty cell actually playable *now* by dropping in its column?
static int isPlayableCell(char board[ROWS][COLS], int r, int c) {
    if (board[r][c] != EMPTY) return 0;
    return (r == ROWS - 1 || board[r + 1][c] != EMPTY);
}

// Count how many different moves for `piece` would win immediately next turn.
static int countImmediateWins(char board[ROWS][COLS], char piece) {
    int count = 0;

    for (int c = 0; c < COLS; c++) {
        int r = getLandingRow(board, c);
        if (r == -1) continue;

        board[r][c] = piece;
        if (hasWon(board, piece)) {
            count++;
        }
        board[r][c] = EMPTY;
    }

    return count;
}

// Score a specific 4-cell window starting at (r0,c0) in direction (dr,dc)
static int scoreWindowAt(char board[ROWS][COLS],
                         int r0, int c0, int dr, int dc,
                         char cpu, char human) {
    int cpuCount = 0, humanCount = 0, emptyCount = 0;
    int playableEmptyCount = 0;

    for (int i = 0; i < 4; i++) {
        int r = r0 + i * dr;
        int c = c0 + i * dc;
        char cell = board[r][c];

        if (cell == cpu) {
            cpuCount++;
        } else if (cell == human) {
            humanCount++;
        } else {
            emptyCount++;
            if (isPlayableCell(board, r, c)) {
                playableEmptyCount++;
            }
        }
    }

    int score = 0;

    // Good patterns for CPU
    if (cpuCount == 4) {
        score += 100000;   // already winning pattern
    } else if (cpuCount == 3 && emptyCount == 1) {
        // Stronger if the empty spot is actually playable
        if (playableEmptyCount > 0) score += 180;
        else                        score += 60;
    } else if (cpuCount == 2 && emptyCount == 2) {
        score += 10;
    }

    // Good patterns for human (bad for CPU)
    if (humanCount == 3 && emptyCount == 1) {
        if (playableEmptyCount > 0) score -= 220;  // urgent to block
        else                        score -= 80;
    } else if (humanCount == 2 && emptyCount == 2) {
        score -= 10;
    }

    return score;
}

static int evaluateBoard(char board[ROWS][COLS], char cpu, char human) {
    int score = 0;

    // Center column bonus
    int center = COLS / 2;
    int centerCount = 0;
    for (int r = 0; r < ROWS; r++) {
        if (board[r][center] == cpu) centerCount++;
    }
    score += centerCount * 6;

    // Horizontal windows
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c <= COLS - 4; c++) {
            score += scoreWindowAt(board, r, c, 0, 1, cpu, human);
        }
    }

    // Vertical windows
    for (int c = 0; c < COLS; c++) {
        for (int r = 0; r <= ROWS - 4; r++) {
            score += scoreWindowAt(board, r, c, 1, 0, cpu, human);
        }
    }

    // Diagonal down-right
    for (int r = 0; r <= ROWS - 4; r++) {
        for (int c = 0; c <= COLS - 4; c++) {
            score += scoreWindowAt(board, r, c, 1, 1, cpu, human);
        }
    }

    // Diagonal up-right
    for (int r = 3; r < ROWS; r++) {
        for (int c = 0; c <= COLS - 4; c++) {
            score += scoreWindowAt(board, r, c, -1, 1, cpu, human);
        }
    }

    // Double-threat / immediate-win counting
    int cpuWinNext = countImmediateWins(board, cpu);
    int humanWinNext = countImmediateWins(board, human);

    if (cpuWinNext >= 2) {
        // Multiple winning moves next turn is incredibly strong
        score += 20000 * cpuWinNext;
    } else if (cpuWinNext == 1) {
        score += 5000;
    }

    if (humanWinNext >= 2) {
        // Opponent double threat is catastrophic
        score -= 25000 * humanWinNext;
    } else if (humanWinNext == 1) {
        score -= 6000;
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

    // Terminal win/loss checks with depth-based bonuses
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
