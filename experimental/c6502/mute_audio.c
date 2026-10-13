/*
 * mute_audio.c - Target 6502 C implementation of muteAudio ($E271)
 *
 * Silences audio channels and clears DMC raw output.
 * Direct memory-mapped I/O replacement for original assembly routine at $E271.
 */

#include "nes_target.h"

#pragma code-name ("C_MUTE")

void muteAudio(void) {
    REG_DMC_RAW = 0x00;
    REG_SQ1_VOL = 0x10;
    REG_SQ2_VOL = 0x10;
    REG_NOISE_VOL = 0x10;
    REG_TRI_LINEAR = 0x00;
}
