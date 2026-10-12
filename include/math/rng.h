#ifndef TDECOMP_MATH_RNG_H
#define TDECOMP_MATH_RNG_H

#include "common.h"
#include "nes.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Advances the NES Tetris Linear Feedback Shift Register (LFSR) PRNG.
 * Source address: $AB47 (main.asm lines 5468-5483).
 *
 * In 6502 assembly:
 *   X = zero page address of seed (e.g. ADDR_RNG_SEED = $0017)
 *   Y = size of seed in bytes (typically 2)
 */
void generateNextPseudorandomNumber(NES_State *nes, uint8_t seed_addr, uint8_t seed_size);

/*
 * Advances the unreferenced audio sound effect RNG seed ($00EB-$00EC).
 * Source address: $E09E (main.asm lines 5963-5976).
 */
void unreferenced_soundRng(NES_State *nes);

/*
 * Selects the next random tetromino using the iconic NES Tetris piece reroll algorithm.
 * Source address: $9903 (main.asm lines 2964-3004).
 * Returns the spawn orientation of the chosen tetromino.
 */
uint8_t pickRandomTetrimino(NES_State *nes);

/*
 * Selects the next tetromino depending on game mode (Demo vs normal gameplay).
 * Source address: $98EB (main.asm lines 2948-2963).
 * Returns the spawn orientation of the chosen tetromino.
 */
uint8_t chooseNextTetrimino(NES_State *nes);

/*
 * Increments the BCD statistics counter for the specified piece.
 * Source address: $9969 (main.asm lines 3030-3055).
 * Input: orientation of the piece.
 */
void incrementPieceStat(NES_State *nes, uint8_t orientation);

/* Reference tables */
extern const uint8_t spawnTable[7];
extern const uint8_t tetriminoTypeFromOrientation[19];
extern const uint8_t spawnOrientationFromOrientation[19];

#ifdef __cplusplus
}
#endif

#endif /* TDECOMP_MATH_RNG_H */
