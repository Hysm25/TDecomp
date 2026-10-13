#ifndef TDECOMP_GAME_ENDING_H
#define TDECOMP_GAME_ENDING_H

#include "common.h"
#include "nes.h"
#include "constants/gameplay.h"
#include "constants/hardware.h"
#include "constants/memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Authoritative Ending Animation ROM Data Tables
 */

/* Dome liftoff delay counter match table (4 bytes at $A735) */
extern const uint8_t domeLiftoffDelayTable[4];

/* Dome X coordinate offsets (4 bytes at $A739) */
extern const uint8_t domeNumberToXOffsetTable[4];

/* Dome starting Y heights (4 bytes at $A73D) */
extern const uint8_t domeStartHeightsTable[4];

/* Dome sprite IDs (4 bytes at $A741) */
extern const uint8_t domeNumberToSpriteTable[4];

/* Dome thruster flame X offset table (4 bytes at $A745) */
extern const uint8_t domeFlameXOffsetTable[4];

/* Type B Cathedral sprite animation speeds per level 0..9 (10 bytes at $A749) */
extern const uint8_t ending_typeBCathedralAnimSpeed[10];

/* Type B Cathedral frame delay before movement per level 0..9 (10 bytes at $A753) */
extern const uint8_t ending_typeBCathedralFrameDelayTable[10];

/* Type B Cathedral initial X delta per level 0..9 (10 bytes at $A75D) */
extern const uint8_t cathedralStartDeltaTable[10];

/* Type B Cathedral ending X target per level 0..9 (10 bytes at $A767) */
extern const uint8_t cathedralEndTargetTable[10];

/* Type B Cathedral movement vector step per level 0..9 (10 bytes at $A771) */
extern const uint8_t ending_typeBCathedralVectorTable[10];

/* Type B Cathedral sprite trigger X coordinates (60 bytes at $A77B: 10 levels * 6 sprites) */
extern const uint8_t ending_typeBCathedralXTable[60];

/* Type B Cathedral sprite Y positions (60 bytes at $A7B7: 10 levels * 6 sprites) */
extern const uint8_t ending_typeBCathedralYTable[60];

/* Type B Cathedral base sprite IDs per level 0..9 (10 bytes at $A7F3) */
extern const uint8_t ending_typeBCathedralSpriteTable[10];

/* Mario frame to Y offset table (4 bytes at $A80A) */
extern const uint8_t marioFrameToYOffsetTable[4];

/* Luigi frame to Y offset table (10 bytes at $A80E) */
extern const uint8_t luigiFrameToYOffsetTable[10];

/* Luigi frame to sprite table (10 bytes at $A818) */
extern const uint8_t luigiFrameToSpriteTable[10];

/* PPU nametable patches for Type B Concert character masking */
extern const uint8_t ending_patchToPpu_typeBConcertHeightUnused[18];
extern const uint8_t ending_patchToPpu_typeBConcertHeight0[22];
extern const uint8_t ending_patchToPpu_typeBConcertHeight1[24];
extern const uint8_t ending_patchToPpu_typeBConcertHeight2[24];
extern const uint8_t ending_patchToPpu_typeBConcertHeight3[28];
extern const uint8_t ending_patchToPpu_typeBConcertHeight4[18];

/* PPU nametable patch for Type A 120,000+ score cathedral launch (89 bytes at $A8CC) */
extern const uint8_t ending_patchToPpu_typeAOver120k[89];

/* Rocket body sprite ID table per ending tier 0..4 (5 bytes at $AA11) */
extern const uint8_t rocketToSpriteTable[5];

/* Rocket thruster flame sprite table per ending tier 0..4 and frame 0..1 (10 bytes at $AA16) */
extern const uint8_t rocketToJetSpriteTable[10];

/* Rocket thruster flame X offset table per ending tier 0..4 (5 bytes at $AA20) */
extern const uint8_t rocketToJetXOffsetTable[5];

/* Rocket body X coordinate offset table per ending tier 0..4 (5 bytes at $AA25) */
extern const uint8_t rocketToXOffsetTable[5];

/* Rocket initial Y position table per ending tier 0..4 (5 bytes at $AA2A) */
extern const uint8_t rocketStartYTable[5];

/*
 * Decompiled Ending Routines
 */

/*
 * Reads patch data from patchToPpuAddr ($14) and streams byte sequences to PPU.
 * Format: PPUADDR hi, lo, bytes..., $FE (next address), $FD (terminate).
 * Source address: $A507 (main.asm lines 4713-4732).
 */
void patchToPpu(NES_State *nes);

/*
 * Dispatches PPU nametable masking patches for Type B Concert based on ending ($C4),
 * falling through across unearned height tiers to erase inactive characters.
 * Source address: $A4B1 (main.asm lines 4664-4712).
 */
void ending_typeBConcertPatchToPpuForHeight(NES_State *nes);

/*
 * Selects Type A ending tier (0..4) based on Player 1 score high byte ($75):
 *   < 50k: tier 0 (small rocket)
 *   < 70k: tier 1 (medium rocket)
 *   < 100k: tier 2 (large rocket)
 *   < 120k: tier 3 (huge rocket)
 *   >= 120k: tier 4 (cathedral launch + PPU nametable patch).
 * Initializes rocket starting Y coordinate and delay counter.
 * Source address: $A96E (main.asm lines 5197-5225).
 */
void selectEndingScreen(NES_State *nes);

/*
 * Alternate entry point for selectEndingScreen after tier is determined ($A9A5).
 * Sets ending_customVars = rocketStartYTable[ending] and ending_customVars+1 = 0.
 * Source address: $A9A5 (main.asm lines 5226-5232).
 */
void ending_selected(NES_State *nes);

/*
 * Stages Type A rocket and thruster sprites into OAM staging buffer,
 * handles 240-frame launch delay, and accelerates upward ascent physics.
 * Source address: $A9B1 (main.asm lines 5234-5282).
 */
void ending_typeA(NES_State *nes);

/*
 * Initializes Type B ending variables: clears ending, ending_customVars,
 * and cathedral delay counter; initializes dome liftoff heights for level 9
 * or cathedral sprite positions for levels 0..8.
 * Source address: $A463 (main.asm lines 4623-4650).
 */
void ending_initTypeBVars(NES_State *nes);

/*
 * Calculates dome liftoff acceleration, plays sound effect, updates dome heights,
 * and stages dome and flame sprites for Type B Level 9 concert finale.
 * Source address: $A6BC (main.asm lines 4992-5050).
 */
void LA6BC(NES_State *nes);

/*
 * Stages guest Nintendo character sprites (Mario, Luigi, Peach, Bowser, Donkey Kong,
 * Samus, Link, Pit) according to player1_startHeight with cascading fallthrough,
 * and executes dome liftoff staging via LA6BC.
 * Source address: $A53B (main.asm lines 4750-4906).
 */
void ending_typeBConcert(NES_State *nes);

/*
 * Alternates cathedral character animation sprite index based on speed table.
 * Source address: $A690 (main.asm lines 4961-4978).
 */
void ending_typeBCathedralSetSprite(NES_State *nes);

/*
 * Computes table index: levelNumber * 6 + ending_currentSprite.
 * Also sets generalCounter ($A8) = levelNumber * 2.
 * Source address: $A6AE (main.asm lines 4979-4991).
 */
uint8_t ending_computeTypeBCathedralYTableIndex(NES_State *nes);

/*
 * Animates Type B cathedral characters for levels 0..8 across startHeight sprites.
 * Source address: $A625 (main.asm lines 4907-4960).
 */
void ending_typeBCathedral(NES_State *nes);

/*
 * Master Type B cutscene sprite staging routine: branches to ending_typeBConcert
 * for level 9, or ending_typeBCathedral for levels 0..8.
 * Source address: $A52E (main.asm lines 4739-4748).
 */
void ending_typeB(NES_State *nes);

/*
 * Master ending sprite staging dispatcher: calls ending_typeB if Game Type B,
 * or ending_typeA if Game Type A.
 * Source address: $A527 (main.asm lines 4734-4738).
 */
void render_ending(NES_State *nes);

/*
 * Executes unskippable ending frame sleep loop, calling render_ending and
 * waiting for NMI frame sync while decrementing sleepCounter to 0.
 * Source address: $A7FD (main.asm lines 5098-5105).
 */
void render_endingUnskippable(NES_State *nes, uint8_t count);

/*
 * Runs full Type A ending cutscene sequence: sets CHR bank 2, loads nametable
 * and palette, selects ending tier, plays music, and waits for launch completion
 * and START button press.
 * Source address: $A926 (main.asm lines 5165-5195).
 */
void endingAnimationA(NES_State *nes);

/*
 * Runs full Type B ending cutscene sequence: sets CHR banks according to level,
 * initializes variables, plays music, counts down level and height bonus points
 * with software BCD decrements and sound effects, and waits for START button press.
 * Source address: $9E49 (main.asm lines 3802-3944).
 */
void endingAnimationB(NES_State *nes);

/*
 * Master ending cutscene entry point: sets sprite index 2, render mode 4,
 * and dispatches to endingAnimationA (Type A) or endingAnimationB (Type B).
 * Source address: $9E3A (main.asm lines 3790-3798).
 */
void endingAnimation(NES_State *nes);

#ifdef __cplusplus
}
#endif

#endif /* TDECOMP_GAME_ENDING_H */
