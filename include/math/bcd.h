#ifndef TDECOMP_MATH_BCD_H
#define TDECOMP_MATH_BCD_H

#include "common.h"
#include "nes.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Low-level BCD byte addition: a + b + carry_in.
 * Returns BCD adjusted result; sets *carry_out to 1 if result exceeded 99, else 0.
 */
uint8_t bcd_add8(uint8_t a, uint8_t b, uint8_t *carry);

/*
 * Low-level BCD byte subtraction: a - b - borrow_in.
 * Returns BCD adjusted result; sets *borrow_out to 1 if borrow occurred, else 0.
 */
uint8_t bcd_sub8(uint8_t a, uint8_t b, uint8_t *borrow);

/*
 * Adds soft drop points based on holdDownPoints ($004F).
 * Source address: $9C0C (main.asm lines 3431-3459).
 */
void addHoldDownPoints(NES_State *nes);

/*
 * Adds line clear points multiplied by (level + 1) for the number of completed lines.
 * Points per line: 1: 40, 2: 100, 3: 300, 4: 1200.
 * Maxes out score at 999,999 ($99, $99, $99).
 * Source address: $9C2A (main.asm lines 3460-3528).
 */
void addLineClearPoints(NES_State *nes);

/*
 * Updates the line counters, Type A level progression, and Type B lines remaining.
 * Source address: $9B58 (main.asm lines 3329-3430).
 */
void updateLinesAndLevel(NES_State *nes);

/*
 * Complete playState 4 handler: updates music speed, lines, statistics, and scores.
 * Source address: $9B58 (main.asm lines 3329-3528).
 */
void playState_updateLinesAndStatistics(NES_State *nes);

/*
 * Checks row 5 of playfield to trigger allegro (danger speed) music.
 * Source address: $9D17 (main.asm lines 3596-3629).
 */
void updateMusicSpeed(NES_State *nes);

/*
 * Sets currently requested music track.
 * Source address: $9E07 (main.asm lines 3746-3754).
 */
void setMusicTrack(NES_State *nes, uint8_t track);

/* Point values table per completed line count (0 to 4 lines) */
extern const uint16_t pointsTable[5];

/* Music selection table: normal and allegro tracks */
extern const uint8_t musicSelectionTable[8];

#ifdef __cplusplus
}
#endif

#endif /* TDECOMP_MATH_BCD_H */
