/*
 * proof_of_concept.c - Target-Ready 6502 C Implementations
 *
 * Demonstrates compiling decompiled C routines into executable 6502 machine code
 * for the NES using cc65.
 */

#include "nes_target.h"

#pragma static-locals(on)

/*
 * copyOamStagingToOam ($AB5E in reference ROM)
 * Resets OAM write address and initiates DMA transfer from page $0200.
 *
 * Compiles under cc65 to:
 *   lda #$00
 *   sta $2003
 *   lda #$02
 *   sta $4014
 *   rts
 * (11 bytes, 100% bit-exact to original 1989 Nintendo machine code)
 */
void copyOamStagingToOam(void) {
    REG_OAMADDR = 0x00;
    REG_OAMDMA = 0x02;
}

/*
 * muteAudio ($E271 in reference ROM)
 * Clears DMC raw output, silences square and noise channels via duty/envelope,
 * and halts the triangle wave linear counter.
 *
 * Compiles under cc65 to:
 *   lda #$00
 *   sta $4011
 *   lda #$10
 *   sta $4000
 *   sta $4004
 *   sta $400C
 *   lda #$00
 *   sta $4008
 *   rts
 * (22 bytes, 100% bit-exact to original 1989 Nintendo machine code)
 */
void muteAudio(void) {
    REG_DMC_RAW = 0x00;
    REG_SQ1_VOL = 0x10;
    REG_SQ2_VOL = 0x10;
    REG_NOISE_VOL = 0x10;
    REG_TRI_LINEAR = 0x00;
}

/*
 * unreferenced_soundRng ($E09E in reference ROM)
 * Audio subsystem 16-bit LFSR pseudo-random number generator.
 *
 * Demonstrates behavioral equivalence: computes the exact same state
 * as the original 6502 routine.
 */
void unreferenced_soundRng(void) {
    uint8_t b0 = ZP_SOUND_RNG_LO;
    uint8_t b1 = ZP_SOUND_RNG_HI;
    uint8_t c = (b0 & 2) ^ (b1 & 2);

    RAM_SOUND_RNG_TMP = (uint8_t)(b0 & 2);
    ZP_SOUND_RNG_LO = (uint8_t)((b0 >> 1) | (c ? 0x80 : 0x00));
    ZP_SOUND_RNG_HI = (uint8_t)((b1 >> 1) | ((b0 & 1) ? 0x80 : 0x00));
}
