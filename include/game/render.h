#ifndef TDECOMP_GAME_RENDER_H
#define TDECOMP_GAME_RENDER_H

#include "common.h"
#include "nes.h"
#include "constants/gameplay.h"
#include "constants/hardware.h"
#include "constants/memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Authoritative color palette table per level modulo 10 (40 bytes at $984C) */
extern const uint8_t colorTable[40];

/* Authoritative OAM content lookup and sprite data table ($8C6C to $9443, 2008 bytes) */
extern const uint8_t oamContentLookupAndSprites[2008];

/*
 * Copies OAM staging buffer ($0200..$02FF) to PPU OAM via DMA ($4014).
 * Writes 0 to OAMADDR ($2003) and 2 to OAMDMA ($4014).
 * Source address: $AB5E (main.asm lines 5486-5492).
 */
void copyOamStagingToOam(NES_State *nes);

/*
 * Resets PPU scroll registers to (0, 0) and restores PPUCTRL from currentPpuCtrl.
 * Source address: $AA8A (main.asm lines 5356-5363).
 */
void copyCurrentScrollAndCtrlToPPU(NES_State *nes);

/*
 * Loads sprite data from ROM into OAM staging buffer ($0200) based on
 * spriteIndexInOamContentLookup ($A2), spriteXOffset ($A0), and spriteYOffset ($A1).
 * Source address: $8C27 (main.asm lines 1861-1903).
 */
void loadSpriteIntoOamStaging(NES_State *nes);

/*
 * Updates background palette 2 ($3F08) and sprite palette 2 ($3F18)
 * according to player 1's level number modulo 10.
 * Source address: $9808 (main.asm lines 2833-2870).
 */
void updatePaletteForLevel(NES_State *nes);

/*
 * Render Mode 0: Legal screen and title screen scroll and control register update.
 * Masks currentPpuCtrl (& 0xFC) and resets scroll to (0, 0).
 * Source address: $82B1 (main.asm lines 636-646).
 */
void render_mode_legal_and_title_screens(NES_State *nes);

/*
 * Render Mode 1: Menu screens scroll and control register update.
 * Masks currentPpuCtrl (& 0xFC), writes to PPUCTRL, and resets scroll to (0, 0).
 * Source address: $85DA (main.asm lines 1078-1091).
 */
void render_mode_menu_screens(NES_State *nes);

/*
 * Render Mode 4: Ending animation score rendering to PPU nametable.
 * Renders player 1 score ($208E) and Type B bonuses/total score ($20B0, $20D0, $212E).
 * Resets scroll to (0, 0).
 * Source address: $9F95 (main.asm lines 3975-4014).
 */
void render_mode_ending_animation(NES_State *nes);

/*
 * Render Mode 3: Play and demo screen render handler.
 * Source address: $94EE (main.asm lines 2457-2671).
 */
void render_mode_play_and_demo(NES_State *nes);

/*
 * Master render dispatcher: jumps to corresponding render routine based on renderMode (0..4).
 * Source address: $804B (main.asm lines 290-296).
 */
void render(NES_State *nes);

#ifdef __cplusplus
}
#endif

#endif /* TDECOMP_GAME_RENDER_H */
