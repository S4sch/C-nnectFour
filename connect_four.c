#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <string.h>

#include "connect_four.h"
#include "rl_agent.h"

// ---------------- Core win-check helpers ----------------

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

// -------- Ask user if they want to play again --------
static int askPlayAgain(void) {
    char buf[16];

    while (1) {
        printf("\nPlay again? (y/n): ");
        if (!fgets(buf, sizeof(buf), stdin)) {
            // Input error / EOF: treat as "no"
            return 0;
        }

        // Skip leading whitespace
        char *p = buf;
        while (*p && isspace((unsigned char)*p)) p++;

        if (*p == 'y' || *p == 'Y') {
            return 1;
        } else if (*p == 'n' || *p == 'N') {
            return 0;
        } else {
            printf("Please enter 'y' or 'n'.\n");
        }
    }
}

// ---------------- Self-learning agent ----------------

static RLAgent gAgent;
#define MODEL_PATH "c4_model.bin"

// ---------------- main ----------------

int main(void) {
    // Seed RNG for CPU move tie-breaking + RL exploration
    srand((unsigned int)time(NULL));

    // Init + load self-learning agent
    rl_init(&gAgent);
    if (rl_load(&gAgent, MODEL_PATH)) {
        printf("Loaded self-learning model from %s\n", MODEL_PATH);
    } else {
        printf("No self-learning model found at %s. Starting fresh.\n", MODEL_PATH);
    }

    // Outer loop: repeat whole games
    do {
        int mode = selectGameMode();

        // If minimax CPU mode, let user choose difficulty (depth)
        if (mode == 2) {
            selectCPUDifficulty();
        }

        // Select whether or not to play with color
        selectColorMode();

        // Training mode (self-play)
        if (mode == 4) {
            int games = promptTrainingGames();
            printf("\nTraining self-learning AI for %d games...\n", games);
            rl_train_selfplay(&gAgent, games);
            if (rl_save(&gAgent, MODEL_PATH)) {
                printf("Training complete. Saved model to %s\n", MODEL_PATH);
            } else {
                printf("Training complete, but failed to save model to %s\n", MODEL_PATH);
            }
            printf("\nNow that it has saved, you will play against it.\n");
            // After training, immediately let the user play against it
            mode = 3;
        }

        char board[ROWS][COLS];
        initializeBoard(board);

        char currentPlayer = PLAYER1;

        // Single-game loop
        while (1) {
            displayBoard(board);

            int col = 0;

            if (currentPlayer == PLAYER2) {
                if (mode == 2) {
                    // Minimax CPU
                    col = getCPUMove(board, currentPlayer);
                    printf("CPU chooses column %d\n", col + 1);
                } else if (mode == 3) {
                    // Self-learning AI
					col = rl_choose_move(&gAgent, board, currentPlayer, 0.0, 2);
                    printf("SelfLearn AI chooses column %d\n", col + 1);
                } else {
                    // HvH: PLAYER2 is a human
                    col = getHumanMove(board, currentPlayer);
                }
            } else {
                // PLAYER1 is always human in these modes
                col = getHumanMove(board, currentPlayer);
            }

            int row = dropPiece(board, col, currentPlayer);

            if (checkWin(board, currentPlayer, row, col)) {
                displayBoardWin(board, currentPlayer, row, col);
                if ((mode == 2 || mode == 3) && currentPlayer == PLAYER2) {
                    printf("%s (%c) wins!\n", (mode == 2) ? "CPU" : "SelfLearn AI", currentPlayer);
                } else {
                    printf("Player %c wins!\n", currentPlayer);
                }
                break;
            }

            if (isBoardFull(board)) {
                displayBoard(board);
                printf("It's a draw!\n");
                break;
            }

            currentPlayer = (currentPlayer == PLAYER1) ? PLAYER2 : PLAYER1;
        }

        // Save model (harmless even if unchanged)
        rl_save(&gAgent, MODEL_PATH);

    } while (askPlayAgain());

    printf("Thanks for playing!\n");
    return 0;
}