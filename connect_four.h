#ifndef CONNECT_FOUR_H
#define CONNECT_FOUR_H

// Standard Connect Four board size
#define ROWS 6
#define COLS 7

// Cell states / tokens
#define EMPTY   '.'
#define PLAYER1 'X'
#define PLAYER2 'O'

// -------- Core game logic (connect_four.c) --------
void initializeBoard(char board[ROWS][COLS]);
int  dropPiece(char board[ROWS][COLS], int col, char piece);
int  checkWin(char board[ROWS][COLS], char piece, int last_row, int last_col);
int  isBoardFull(char board[ROWS][COLS]);

// -------- I/O + display + CPU (io_engine.c) --------
void displayBoard(char board[ROWS][COLS]);
void displayBoardWin(char board[ROWS][COLS], char winner, int last_row, int last_col);
int  getHumanMove(char board[ROWS][COLS], char piece);
int  getCPUMove(char board[ROWS][COLS], char piece);
int  isMoveValid(char board[ROWS][COLS], int col);
int  selectGameMode(void);
int  selectColorMode(void);
int promptTrainingGames(void);

// lets the user choose CPU difficulty (depth)
int  selectCPUDifficulty(void);

#endif
