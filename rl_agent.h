#ifndef RL_AGENT_H
#define RL_AGENT_H

#include "connect_four.h"

#define RL_FEATURES 14

typedef struct {
    double w[RL_FEATURES];

    // Learning hyperparameters
    double alpha;    // learning rate
    double gamma;    // discount factor
    double lambda;   // eligibility trace decay (TD(lambda))
    double epsilon;  // starting exploration (training)
} RLAgent;

void  rl_init(RLAgent *a);
int   rl_load(RLAgent *a, const char *path);
int   rl_save(const RLAgent *a, const char *path);

// Value from perspective of "player to move" (player = 'X' or 'O')
double rl_value(const RLAgent *a, char board[ROWS][COLS], char player);

// Choose move for `player`.
int rl_choose_move(const RLAgent *a,
                   char board[ROWS][COLS],
                   char player,
                   double epsilon_override,
                   int searchDepth);

// Train by self-play
void rl_train_selfplay(RLAgent *a, int games);
#endif