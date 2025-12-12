#include "rl_agent.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define RL_INF 1e100

static char otherPlayer(char p) { return (p == PLAYER1) ? PLAYER2 : PLAYER1; }

static void copyBoard(char dst[ROWS][COLS], char src[ROWS][COLS]) {
    memcpy(dst, src, sizeof(char) * ROWS * COLS);
}

static int isMoveValidRL(char board[ROWS][COLS], int col) {
    return (col >= 0 && col < COLS && board[0][col] == EMPTY);
}

static int getLandingRow(char board[ROWS][COLS], int col) {
    for (int r = ROWS - 1; r >= 0; r--) {
        if (board[r][col] == EMPTY) return r;
    }
    return -1;
}

static int isPlayableCell(char board[ROWS][COLS], int r, int c) {
    if (board[r][c] != EMPTY) return 0;
    return (r == ROWS - 1 || board[r + 1][c] != EMPTY);
}

static double dot(const double *w, const double *x) {
    double s = 0.0;
    for (int i = 0; i < RL_FEATURES; i++) s += w[i] * x[i];
    return s;
}

static void clampWeights(RLAgent *a) {
    for (int i = 0; i < RL_FEATURES; i++) {
        if (a->w[i] > 50.0) a->w[i] = 50.0;
        if (a->w[i] < -50.0) a->w[i] = -50.0;
    }
}

// Count immediate winning moves available to `piece` on this board.
static int countImmediateWins(char board[ROWS][COLS], char piece) {
    int count = 0;
    for (int c = 0; c < COLS; c++) {
        int r = getLandingRow(board, c);
        if (r < 0) continue;
        board[r][c] = piece;
        {
            int win = checkWin(board, piece, r, c);
            board[r][c] = EMPTY;
            if (win) count++;
        }
    }
    return count;
}

/*
Features (RL_FEATURES = 14):
0  bias
1  centerDiff (me - opp)
2  my 3+1 playable
3  my 3+1 not playable
4  my 2+2 (>=1 empty playable)
5  my 2+2 (no empty playable)
6  opp 3+1 playable
7  opp 3+1 not playable
8  opp 2+2 (>=1 empty playable)
9  opp 2+2 (no empty playable)
10 my immediate winning moves next turn
11 opp immediate winning moves next turn
12 my 1+3 potential
13 opp 1+3 potential
*/

static void score_window(char board[ROWS][COLS], char me, char opp,
                         double f[RL_FEATURES],
                         int r0, int c0, int dr, int dc) {
    int meCount = 0, oppCount = 0, emptyCount = 0;
    int playableEmpty = 0;

    for (int i = 0; i < 4; i++) {
        int r = r0 + i * dr;
        int c = c0 + i * dc;
        char cell = board[r][c];

        if (cell == me) meCount++;
        else if (cell == opp) oppCount++;
        else {
            emptyCount++;
            if (isPlayableCell(board, r, c)) playableEmpty++;
        }
    }

    // Only count "clean" windows (no mixed pieces)
    if (oppCount == 0) {
        if (meCount == 3 && emptyCount == 1) {
            if (playableEmpty) f[2] += 1.0;
            else               f[3] += 1.0;
        } else if (meCount == 2 && emptyCount == 2) {
            if (playableEmpty) f[4] += 1.0;
            else               f[5] += 1.0;
        } else if (meCount == 1 && emptyCount == 3) {
            f[12] += 1.0;
        }
    }

    if (meCount == 0) {
        if (oppCount == 3 && emptyCount == 1) {
            if (playableEmpty) f[6] += 1.0;
            else               f[7] += 1.0;
        } else if (oppCount == 2 && emptyCount == 2) {
            if (playableEmpty) f[8] += 1.0;
            else               f[9] += 1.0;
        } else if (oppCount == 1 && emptyCount == 3) {
            f[13] += 1.0;
        }
    }
}

static void extractFeatures(char board[ROWS][COLS], char me, double f[RL_FEATURES]) {
    char opp = otherPlayer(me);
    memset(f, 0, sizeof(double) * RL_FEATURES);
    f[0] = 1.0;

    // Center column difference
    {
        int center = COLS / 2;
        int myCenter = 0, oppCenter = 0;
        for (int r = 0; r < ROWS; r++) {
            if (board[r][center] == me) myCenter++;
            else if (board[r][center] == opp) oppCenter++;
        }
        f[1] = (double)(myCenter - oppCenter);
    }

    // Scan windows
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c <= COLS - 4; c++)
            score_window(board, me, opp, f, r, c, 0, 1);

    for (int c = 0; c < COLS; c++)
        for (int r = 0; r <= ROWS - 4; r++)
            score_window(board, me, opp, f, r, c, 1, 0);

    for (int r = 0; r <= ROWS - 4; r++)
        for (int c = 0; c <= COLS - 4; c++)
            score_window(board, me, opp, f, r, c, 1, 1);

    for (int r = 3; r < ROWS; r++)
        for (int c = 0; c <= COLS - 4; c++)
            score_window(board, me, opp, f, r, c, -1, 1);

    // Immediate win counts (very important tactical signal)
    f[10] = (double)countImmediateWins(board, me);
    f[11] = (double)countImmediateWins(board, opp);
}

void rl_init(RLAgent *a) {
    for (int i = 0; i < RL_FEATURES; i++) a->w[i] = 0.0;

    // Helpful initial biases (not required, but speeds up learning)
    a->w[1]  = 0.3;   // center
    a->w[2]  = 2.0;   // my 3+1 playable
    a->w[6]  = -2.5;  // opp 3+1 playable (block!)
    a->w[10] = 3.0;   // my immediate wins
    a->w[11] = -3.5;  // opp immediate wins (danger)

    // Learning params (good defaults)
    a->alpha   = 0.004;
    a->gamma   = 0.99;
    a->lambda  = 0.85;
    a->epsilon = 0.25;
}

/*
Model save format (versioned):
magic "C4RL"
u32 version (2)
u32 features (RL_FEATURES)
then weights[RL_FEATURES] (double)
*/
int rl_save(const RLAgent *a, const char *path) {
    FILE *fp = fopen(path, "wb");
    if (!fp) return 0;

    {
        const char magic[4] = {'C','4','R','L'};
        uint32_t ver = 2;
        uint32_t feat = (uint32_t)RL_FEATURES;

        if (fwrite(magic, 1, 4, fp) != 4) { fclose(fp); return 0; }
        if (fwrite(&ver, sizeof(ver), 1, fp) != 1) { fclose(fp); return 0; }
        if (fwrite(&feat, sizeof(feat), 1, fp) != 1) { fclose(fp); return 0; }
    }

    if (fwrite(a->w, sizeof(double), RL_FEATURES, fp) != RL_FEATURES) {
        fclose(fp);
        return 0;
    }

    fclose(fp);
    return 1;
}

int rl_load(RLAgent *a, const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return 0;

    char magic[4];
    uint32_t ver = 0, feat = 0;

    if (fread(magic, 1, 4, fp) != 4) { fclose(fp); return 0; }
    if (memcmp(magic, "C4RL", 4) != 0) { fclose(fp); return 0; }
    if (fread(&ver, sizeof(ver), 1, fp) != 1) { fclose(fp); return 0; }
    if (fread(&feat, sizeof(feat), 1, fp) != 1) { fclose(fp); return 0; }

    if (ver != 2 || feat != (uint32_t)RL_FEATURES) {
        fclose(fp);
        return 0; // incompatible model file
    }

    if (fread(a->w, sizeof(double), RL_FEATURES, fp) != RL_FEATURES) {
        fclose(fp);
        return 0;
    }

    fclose(fp);
    return 1;
}

double rl_value(const RLAgent *a, char board[ROWS][COLS], char player) {
    double f[RL_FEATURES];
    extractFeatures(board, player, f);
    return dot(a->w, f);
}

// Tactical: immediate win else immediate block
static int immediateTactics(char board[ROWS][COLS], char me) {
    char opp = otherPlayer(me);
    char tmp[ROWS][COLS];

    // win now
    for (int c = 0; c < COLS; c++) {
        if (!isMoveValidRL(board, c)) continue;
        copyBoard(tmp, board);
        {
            int r = dropPiece(tmp, c, me);
            if (r >= 0 && checkWin(tmp, me, r, c)) return c;
        }
    }

    // block opp win now
    for (int c = 0; c < COLS; c++) {
        if (!isMoveValidRL(board, c)) continue;
        copyBoard(tmp, board);
        {
            int r = dropPiece(tmp, c, opp);
            if (r >= 0 && checkWin(tmp, opp, r, c)) return c;
        }
    }

    return -1;
}

// Evaluate a move using learned value + (optional) 2-ply reply
static double evalMove(const RLAgent *a,
                       char board[ROWS][COLS],
                       char player,
                       int col,
                       int searchDepth) {
    char opp = otherPlayer(player);

    char b1[ROWS][COLS];
    copyBoard(b1, board);
    int r1 = dropPiece(b1, col, player);
    if (r1 < 0) return -RL_INF;

    // If we win immediately, it's best
    if (checkWin(b1, player, r1, col)) return RL_INF;

    if (searchDepth <= 1) {
        // 1-ply: prefer states that are bad for opponent-to-move
        return -rl_value(a, b1, opp);
    }

    // 2-ply: opponent picks reply that minimizes our outcome
    double worst = RL_INF;
    int any = 0;

    for (int oc = 0; oc < COLS; oc++) {
        if (!isMoveValidRL(b1, oc)) continue;
        any = 1;

        char b2[ROWS][COLS];
        copyBoard(b2, b1);
        int r2 = dropPiece(b2, oc, opp);
        if (r2 >= 0 && checkWin(b2, opp, r2, oc)) {
            // opponent has winning reply => terrible line
            worst = -RL_INF;
            break;
        }

        // after opponent move, it's our turn again
        {
            double v = rl_value(a, b2, player);
            if (v < worst) worst = v;
        }
    }

    if (!any) return 0.0;
    return worst;
}

int rl_choose_move(const RLAgent *a,
                   char board[ROWS][COLS],
                   char player,
                   double epsilon_override,
                   int searchDepth) {
    int t = immediateTactics(board, player);
    if (t != -1) return t;

    double eps = (epsilon_override < 0.0) ? a->epsilon : epsilon_override;

    int valid[COLS];
    int vc = 0;
    for (int c = 0; c < COLS; c++) {
        if (isMoveValidRL(board, c)) valid[vc++] = c;
    }
    if (vc == 0) return 0;

    if (((double)rand() / (double)RAND_MAX) < eps) {
        return valid[rand() % vc];
    }

    int bestC = valid[0];
    double bestScore = -RL_INF;

    // Center-first ordering helps (not required, but good)
    static const int order[COLS] = {3,2,4,1,5,0,6};

    for (int i = 0; i < COLS; i++) {
        int c = order[i];
        if (!isMoveValidRL(board, c)) continue;

        double s = evalMove(a, board, player, c, searchDepth);
        if (s > bestScore) {
            bestScore = s;
            bestC = c;
        }
    }

    return bestC;
}

// TD(lambda) weight update
static void td_lambda_update(RLAgent *a,
                             double e[RL_FEATURES],
                             const double f_s[RL_FEATURES],
                             double delta) {
    for (int i = 0; i < RL_FEATURES; i++) {
        e[i] = a->gamma * a->lambda * e[i] + f_s[i];
        a->w[i] += a->alpha * delta * e[i];
    }
    clampWeights(a);
}

void rl_train_selfplay(RLAgent *a, int games) {
    // Epsilon schedule (decays to low exploration)
    const double eps_start = a->epsilon;
    const double eps_end   = 0.02;

    for (int g = 0; g < games; g++) {
        char board[ROWS][COLS];
        initializeBoard(board);

        // Eligibility traces per episode
        double e[RL_FEATURES];
        for (int i = 0; i < RL_FEATURES; i++) e[i] = 0.0;

        // Linear decay of epsilon
        double frac = (games <= 1) ? 1.0 : (double)g / (double)(games - 1);
        double eps = eps_start + (eps_end - eps_start) * frac;

        char current = (rand() & 1) ? PLAYER1 : PLAYER2;

        while (1) {
            // State features/value (player-to-move = current)
            double f_s[RL_FEATURES];
            extractFeatures(board, current, f_s);
            double v_s = dot(a->w, f_s);

            // Choose move: depth 1 is fast enough for training
            int col = rl_choose_move(a, board, current, eps, 1);
            int row = dropPiece(board, col, current);

            // Terminal win
            if (row >= 0 && checkWin(board, current, row, col)) {
                double reward = 1.0;
                double delta = reward - v_s; // terminal: no bootstrap
                td_lambda_update(a, e, f_s, delta);
                break;
            }

            // Draw
            if (isBoardFull(board)) {
                double reward = 0.0;
                double delta = reward - v_s;
                td_lambda_update(a, e, f_s, delta);
                break;
            }

            // Non-terminal: reward shaping (teaches tactics strongly)
            {
                char opp = otherPlayer(current);

                int oppWinsNext = countImmediateWins(board, opp);
                int myWinsNext  = countImmediateWins(board, current);

                double reward = 0.0;

                // Big penalty if we allow opponent an immediate win next turn
                if (oppWinsNext > 0) reward -= 0.9;

                // Small bonus if we create an immediate win threat
                if (myWinsNext > 0) reward += 0.2;

                // Bootstrap from next state's value (opponent-to-move)
                double v_next = rl_value(a, board, opp);

                // From current's perspective, opponent value is negated
                double target = reward + a->gamma * (-v_next);

                double delta = target - v_s;
                td_lambda_update(a, e, f_s, delta);

                current = opp;
            }
        }
    }
}
