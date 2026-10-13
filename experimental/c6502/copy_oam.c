/*
 * copy_oam.c - Target 6502 C implementation of copyOamStagingToOam ($AB5E)
 *
 * Resets OAM write address ($2003) to $00 and initiates OAM DMA transfer
 * ($4014) from staging page $0200. Bit-exact replacement for original assembly routine.
 */

#include "nes_target.h"

#pragma code-name ("C_OAM")

void copyOamStagingToOam(void) {
    REG_OAMADDR = 0x00;
    REG_OAMDMA = 0x02;
}
