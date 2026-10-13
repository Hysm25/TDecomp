#ifndef TDECOMP_NES_TARGET_H
#define TDECOMP_NES_TARGET_H

/*
 * nes_target.h - Bare-metal NES hardware register & memory map definitions
 * for 6502 target compilation (cc65 / target-ready C).
 */

#include <stdint.h>

/* PPU Memory-Mapped Registers ($2000 - $2007) */
#define REG_PPUCTRL     (*(volatile uint8_t*)0x2000)
#define REG_PPUMASK     (*(volatile uint8_t*)0x2001)
#define REG_PPUSTATUS   (*(volatile uint8_t*)0x2002)
#define REG_OAMADDR     (*(volatile uint8_t*)0x2003)
#define REG_OAMDATA     (*(volatile uint8_t*)0x2004)
#define REG_PPUSCROLL   (*(volatile uint8_t*)0x2005)
#define REG_PPUADDR     (*(volatile uint8_t*)0x2006)
#define REG_PPUDATA     (*(volatile uint8_t*)0x2007)

/* APU & DMA Registers ($4000 - $4017) */
#define REG_SQ1_VOL     (*(volatile uint8_t*)0x4000)
#define REG_SQ1_SWEEP   (*(volatile uint8_t*)0x4001)
#define REG_SQ1_LO      (*(volatile uint8_t*)0x4002)
#define REG_SQ1_HI      (*(volatile uint8_t*)0x4003)

#define REG_SQ2_VOL     (*(volatile uint8_t*)0x4004)
#define REG_SQ2_SWEEP   (*(volatile uint8_t*)0x4005)
#define REG_SQ2_LO      (*(volatile uint8_t*)0x4006)
#define REG_SQ2_HI      (*(volatile uint8_t*)0x4007)

#define REG_TRI_LINEAR  (*(volatile uint8_t*)0x4008)
#define REG_TRI_LO      (*(volatile uint8_t*)0x400A)
#define REG_TRI_HI      (*(volatile uint8_t*)0x400B)

#define REG_NOISE_VOL   (*(volatile uint8_t*)0x400C)
#define REG_NOISE_LO    (*(volatile uint8_t*)0x400E)
#define REG_NOISE_HI    (*(volatile uint8_t*)0x400F)

#define REG_DMC_FREQ    (*(volatile uint8_t*)0x4010)
#define REG_DMC_RAW     (*(volatile uint8_t*)0x4011)
#define REG_DMC_START   (*(volatile uint8_t*)0x4012)
#define REG_DMC_LEN     (*(volatile uint8_t*)0x4013)

#define REG_OAMDMA      (*(volatile uint8_t*)0x4014)
#define REG_SND_CHN     (*(volatile uint8_t*)0x4015)
#define REG_JOY1        (*(volatile uint8_t*)0x4016)
#define REG_JOY2_APUFC  (*(volatile uint8_t*)0x4017)

/* Zero-Page Game Variables */
#define ZP_TMP1         (*(volatile uint8_t*)0x0000)
#define ZP_TMP2         (*(volatile uint8_t*)0x0001)
#define ZP_TMP3         (*(volatile uint8_t*)0x0002)
#define ZP_RNG_SEED_LO  (*(volatile uint8_t*)0x0017)
#define ZP_RNG_SEED_HI  (*(volatile uint8_t*)0x0018)
#define ZP_SOUND_RNG_LO (*(volatile uint8_t*)0x00EB)
#define ZP_SOUND_RNG_HI (*(volatile uint8_t*)0x00EC)

/* High Score & Gameplay Zero-Page Variables */
#define ZP_PLAYER1_LEVEL        (*(volatile uint8_t*)0x0064)
#define ZP_PLAYER1_SCORE_LO     (*(volatile uint8_t*)0x0073)
#define ZP_PLAYER1_SCORE_MID    (*(volatile uint8_t*)0x0074)
#define ZP_PLAYER1_SCORE_HI     (*(volatile uint8_t*)0x0075)
#define ZP_GENERAL_COUNTER      (*(volatile uint8_t*)0x00A8)
#define ZP_GENERAL_COUNTER2     (*(volatile uint8_t*)0x00A9)
#define ZP_GENERAL_COUNTER3     (*(volatile uint8_t*)0x00AA)
#define ZP_GAME_TYPE            (*(volatile uint8_t*)0x00C1)
#define ZP_HIGH_SCORE_RAW_POS   (*(volatile uint8_t*)0x00D5)

/* Internal Audio RAM */
#define RAM_SOUND_RNG_TMP       (*(volatile uint8_t*)0x06FF)

/* High Score Table RAM ($0700 - $074F) */
#define RAM_HIGH_SCORE_NAMES    ((volatile uint8_t*)0x0700)
#define RAM_HIGH_SCORE_SCORES   ((volatile uint8_t*)0x0730)
#define RAM_HIGH_SCORE_LEVELS   ((volatile uint8_t*)0x0748)

#endif /* TDECOMP_NES_TARGET_H */
