#include "nes_target.h"

#pragma static-locals(on)
#pragma code-name ("C_RNG")

/*
 * generateNextPseudorandomNumber ($AB47 in reference ROM)
 * 16-bit LFSR pseudorandom number generator for NES Tetris gameplay.
 *
 * Steps rng_seed ($0017-$0018) by 1 bit using feedback polynomial:
 *   c = (rng_seed[0] bit 1) ^ (rng_seed[1] bit 1)
 *   rng_seed[0] = (rng_seed[0] >> 1) | (c << 7)
 *   rng_seed[1] = (rng_seed[1] >> 1) | ((old_seed[0] bit 0) << 7)
 * Also sets ZP tmp1 ($0000) = (rng_seed[0] & 0x02).
 *
 * Exhaustively proven equivalent across all 65,536 input seeds.
 */
void generateNextPseudorandomNumber(void) {
    uint8_t b0 = ZP_RNG_SEED_LO;
    uint8_t b1 = ZP_RNG_SEED_HI;
    uint8_t c = (b0 ^ b1) & 0x02;

    ZP_TMP1 = (uint8_t)(b0 & 0x02);
    ZP_RNG_SEED_LO = (uint8_t)((b0 >> 1) | (c ? 0x80 : 0x00));
    ZP_RNG_SEED_HI = (uint8_t)((b1 >> 1) | ((b0 & 0x01) ? 0x80 : 0x00));
}
