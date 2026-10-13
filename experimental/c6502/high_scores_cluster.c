#include "nes_target.h"

/*
 * high_scores_cluster.c - Target-Ready C Implementation of High Score Subsystem
 *
 * Covers routines and data originally located at $A0EE - $A200 (275 bytes, PRG Chunk 1):
 *   - handleHighScoreIfNecessary ($A0EE, 70 bytes)
 *   - adjustHighScores           ($A134, 94 bytes)
 *   - copyHighScoreNameToNextIndex ($A192, 47 bytes)
 *   - copyHighScoreScoreToNextIndex ($A1C1, 31 bytes)
 *   - copyHighScoreLevelToNextIndex ($A1E0, 17 bytes)
 *   - highScoreIndexToHighScoreNamesOffset ($A1F1, 8 bytes)
 *   - highScoreIndexToHighScoreScoresOffset ($A1F9, 8 bytes)
 *
 * External interface contracts:
 *   - Single external caller: JSR handleHighScoreIfNecessary ($A0EE) from
 *     gameMode_play_handleGameOver (main.asm line 3573).
 *   - Exit contract 1 (non-qualifying): clean RTS to caller (@resetGameState).
 *   - Exit contract 2 (qualifying): tail-jump JMP highScoreEntryScreen ($A201).
 *   - Zero external references to internal subroutines or tables.
 */

#pragma static-locals(on)

extern uint8_t highScoreNames[48];
extern uint8_t highScoreScoresA[24];
extern uint8_t highScoreLevels[8];
extern void highScoreEntryScreen(void);

#pragma rodata-name ("RODATA")

const uint8_t highScoreIndexToHighScoreNamesOffset[8] = {
    0x00, 0x06, 0x0C, 0x12, 0x18, 0x1E, 0x24, 0x2A
};

const uint8_t highScoreIndexToHighScoreScoresOffset[8] = {
    0x00, 0x03, 0x06, 0x09, 0x0C, 0x0F, 0x12, 0x15
};

#pragma code-name ("C_HS")

static uint8_t shift_fromIdx;

/*
 * Shifts high score entries down by one slot starting from shift_fromIdx.
 * shift_fromIdx == 1: shifts entry 1 down to entry 2.
 * shift_fromIdx == 0: shifts entry 0 down to entry 1.
 * Supports both Type A (offset +0) and Type B (offset +24 name / +12 score / +4 level).
 */
static void shiftDown(void) {
    uint8_t nameBase = (ZP_GAME_TYPE != 0) ? 24 : 0;
    uint8_t scoreBase = (ZP_GAME_TYPE != 0) ? 12 : 0;
    uint8_t levelBase = (ZP_GAME_TYPE != 0) ? 4 : 0;
    int8_t i;
    uint8_t srcName, srcScore, srcLevel;

    /* Shift 6-character name */
    srcName = nameBase + (shift_fromIdx != 0 ? 6 : 0);
    for (i = 5; i >= 0; i--) {
        highScoreNames[srcName + 6 + i] = highScoreNames[srcName + i];
    }

    /* Shift 3-byte BCD score */
    srcScore = scoreBase + (shift_fromIdx != 0 ? 3 : 0);
    highScoreScoresA[srcScore + 3] = highScoreScoresA[srcScore];
    highScoreScoresA[srcScore + 4] = highScoreScoresA[srcScore + 1];
    highScoreScoresA[srcScore + 5] = highScoreScoresA[srcScore + 2];

    /* Shift 1-byte level number */
    srcLevel = levelBase + shift_fromIdx;
    highScoreLevels[srcLevel + 1] = highScoreLevels[srcLevel];
}

/*
 * handleHighScoreIfNecessary ($A0EE in reference ROM)
 *
 * Checks if Player 1's score qualifies for the 3-entry leaderboard for the active
 * game type (Type A: entries 0..2; Type B: entries 4..6).
 * Ties are broken in favor of the new score.
 *
 * If non-qualifying: returns cleanly via RTS to caller (@resetGameState).
 * If qualifying: shifts lower entries down, clears the 6-character name slot,
 * writes player's 3-byte score and 1-byte level into the table, and jumps to
 * highScoreEntryScreen ($A201).
 */
void handleHighScoreIfNecessary_c(void) {
    uint8_t rawPos = (ZP_GAME_TYPE == 0) ? 0 : 4;
    ZP_HIGH_SCORE_RAW_POS = rawPos;

    while (1) {
        uint8_t y;
        uint8_t s_hi, s_mid, s_lo;

        ZP_GENERAL_COUNTER2 = rawPos;
        y = (rawPos << 1) + rawPos;

        s_hi = highScoreScoresA[y];
        if (s_hi < ZP_PLAYER1_SCORE_HI) {
            goto qualify;
        } else if (s_hi == ZP_PLAYER1_SCORE_HI) {
            s_mid = highScoreScoresA[y + 1];
            if (s_mid < ZP_PLAYER1_SCORE_MID) {
                goto qualify;
            } else if (s_mid == ZP_PLAYER1_SCORE_MID) {
                s_lo = highScoreScoresA[y + 2];
                if (s_lo <= ZP_PLAYER1_SCORE_LO) {
                    goto qualify;
                }
            }
        }

        rawPos++;
        ZP_HIGH_SCORE_RAW_POS = rawPos;
        if (rawPos == 3 || rawPos == 7) {
            return;
        }
    }

qualify:
    {
        uint8_t raw = ZP_HIGH_SCORE_RAW_POS;
        uint8_t slot = raw & 0x03;
        uint8_t nameOff, scoreOff, y;

        if (slot < 2) {
            shift_fromIdx = 1;
            shiftDown();
            if (slot == 0) {
                shift_fromIdx = 0;
                shiftDown();
            }
        }

        nameOff = highScoreIndexToHighScoreNamesOffset[raw];
        for (y = 0; y < 6; y++) {
            highScoreNames[nameOff + y] = 0;
        }

        scoreOff = highScoreIndexToHighScoreScoresOffset[raw];
        highScoreScoresA[scoreOff]     = ZP_PLAYER1_SCORE_HI;
        highScoreScoresA[scoreOff + 1] = ZP_PLAYER1_SCORE_MID;
        highScoreScoresA[scoreOff + 2] = ZP_PLAYER1_SCORE_LO;

        highScoreLevels[raw] = ZP_PLAYER1_LEVEL;

        highScoreEntryScreen();
    }
}
