#ifndef TDECOMP_GAME_PLAYFIELD_H
#define TDECOMP_GAME_PLAYFIELD_H

#include "common.h"
#include "nes.h"
#include "constants/gameplay.h"
#include "constants/memory.h"
#include "game/tetrimino.h"
#include "math/rng.h"
#include "math/bcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Locks the active tetrimino into the playfield grid at (tetriminoX, tetriminoY).
 * If the piece is out of bounds or collides, triggers game over curtain ($9A11).
 * Source address: $99A2 (main.asm lines 3062-3128).
 */
void playState_lockTetrimino(NES_State *nes);

/*
 * Scans the playfield for completed rows in the 4 rows spanned by the active piece.
 * Completed rows are collapsed downward, and empty top rows inserted.
 * Source address: $9A6B (main.asm lines 3188-3278).
 */
void playState_checkForCompletedRows(NES_State *nes);

/*
 * Animates completed line clearing by flashing center-out columns every 4 frames.
 * Transitions to playState 5 when the 5-step animation finishes.
 * Source address: $977F (main.asm lines 2757-2826).
 */
void updateLineClearingAnimation(NES_State *nes);

/*
 * Updates the game over curtain animation, filling playfield rows with TILE_CURTAIN.
 * Source address: $9A11 (main.asm lines 3130-3160).
 */
void playState_updateGameOverCurtain(NES_State *nes);

/*
 * Receives pending garbage lines in 2-player mode, shifting existing rows upward.
 * Source address: $9B03 (main.asm lines 3280-3326).
 */
void playState_receiveGarbage(NES_State *nes);

/*
 * Checks if gameType is Type B; if so, branches to initPlayfieldForTypeB.
 * Source address: $87DC (main.asm lines 1339-1343).
 */
void initPlayfieldIfTypeB(NES_State *nes);

/*
 * Initializes the playfield for Type B game mode: generates random garbage in rows
 * 8-19, guarantees a blank cell per row, and clears rows according to startHeight.
 * Source address: $87E3 (main.asm lines 1344-1435).
 */
void initPlayfieldForTypeB(NES_State *nes);

/* Authoritative Data Tables from PRG ROM */
extern const uint8_t multBy10Table[20];
extern const uint8_t garbageLinesTable[5];
extern const uint8_t leftColumnsTable[5];
extern const uint8_t rightColumnsTable[5];
extern const uint8_t typeBBlankInitCountByHeightTable[6];
extern const uint8_t rngTable[8];

#ifdef __cplusplus
}
#endif

#endif /* TDECOMP_GAME_PLAYFIELD_H */
