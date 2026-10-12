#ifndef TDECOMP_GAME_HIGHSCORE_H
#define TDECOMP_GAME_HIGHSCORE_H

#include "common.h"
#include "nes.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Authoritative High Score Data Tables
 */

/*
 * PPU Base Addresses for High Score Table Rows ($A086, 6 bytes: 3 rows * 2 bytes big-endian)
 * Row 0: $2289, Row 1: $22C9, Row 2: $2309
 */
extern const uint8_t highScorePpuAddrTable[6];

/*
 * High Score Character Code to Nametable Tile ID Lookup ($A08C, 44 bytes)
 */
extern const uint8_t highScoreCharToTile[44];

/*
 * Byte (0..49) to BCD representation lookup table ($A0BC, 50 bytes)
 */
extern const uint8_t byteToBcdTable[50];

/*
 * High Score Entry Index to highScoreNames RAM Offset Lookup ($A1F1, 8 bytes)
 * Offsets: $00, $06, $0C, $12, $18, $1E, $24, $2A
 */
extern const uint8_t highScoreIndexToHighScoreNamesOffset[8];

/*
 * High Score Entry Index to highScoreScores RAM Offset Lookup ($A1F9, 8 bytes)
 * Offsets: $00, $03, $06, $09, $0C, $0F, $12, $15
 */
extern const uint8_t highScoreIndexToHighScoreScoresOffset[8];

/*
 * High Score Cursor Row to Sprite Y Coordinate Lookup ($A33B, 3 bytes)
 * Rows 0..2: $9F, $AF, $BF
 */
extern const uint8_t highScorePosToY[3];

/*
 * High Score Cursor Letter to Sprite X Coordinate Lookup ($A33E, 6 bytes)
 * Letters 0..5: $48, $50, $58, $60, $68, $70
 */
extern const uint8_t highScoreNamePosToX[6];

/*
 * Default High Score Table initialized on cold boot ($AD67, 81 bytes)
 * Contains 48 bytes of default names, 24 bytes of default BCD scores,
 * 8 bytes of default starting levels, terminated by $FF.
 */
extern const uint8_t defaultHighScoresTable[81];

/*
 * Authoritative High Score Routines
 */

/*
 * Check if the NES RAM holds valid warm boot magic numbers ($0750 == $12, $34, $56, $78, $9A).
 */
bool isWarmBoot(const NES_State *nes);

/*
 * Cold Boot High Score Table Initialization:
 * Copies default names, scores, and levels from defaultHighScoresTable into RAM $0700-$074F
 * until encountering $FF, then sets initMagic at $0750.
 * Source address: $8095 (main.asm lines 326-346).
 */
void initHighScoreTable(NES_State *nes);

/*
 * Outputs two BCD digits (high nibble then low nibble) to PPUDATA ($2007).
 * Sets generalCounter ($00A8) to val.
 * Source address: $9712 (main.asm lines 2691-2703).
 */
void twoDigsToPPU(NES_State *nes, uint8_t val);

/*
 * Copies a 6-byte player name from entry at startByte to the next entry index.
 * Adjusts for gameType (startByte + 0x18 if Type B).
 * Source address: $A192 (main.asm lines 4208-4237).
 */
void copyHighScoreNameToNextIndex(NES_State *nes, uint8_t startByte);

/*
 * Copies a 3-byte BCD score from entry at startByte to the next entry index.
 * Adjusts for gameType (startByte + 0x0C if Type B).
 * Source address: $A1C1 (main.asm lines 4239-4257).
 */
void copyHighScoreScoreToNextIndex(NES_State *nes, uint8_t startByte);

/*
 * Copies a 1-byte level number from entry at startByte to the next entry index.
 * Adjusts for gameType (startByte + 0x04 if Type B).
 * Source address: $A1E0 (main.asm lines 4259-4271).
 */
void copyHighScoreLevelToNextIndex(NES_State *nes, uint8_t startByte);

/*
 * Adjusts high score table positions by shifting lower entries down,
 * clears the 6-character name slot for the new high score, writes the player's
 * score and level into the table, and initiates the high score entry screen.
 * Source address: $A134 (main.asm lines 4160-4206).
 */
void adjustHighScores(NES_State *nes);

/*
 * Checks if Player 1's score qualifies for the high score table for the current
 * gameType (Type A: entries 0..2, Type B: entries 4..6). Breaks ties in favor
 * of the new score. If qualified, sets highScoreEntryRawPos and invokes adjustHighScores.
 * Source address: $A0EE (main.asm lines 4116-4159).
 */
void handleHighScoreIfNecessary(NES_State *nes);

/*
 * Return stub for high score display routine.
 * Source address: $A085 (main.asm lines 4093-4094).
 */
void showHighScores_ret(NES_State *nes);

/*
 * Real high score display routine rendering 3 entries for the active gameType
 * (names, 6-digit BCD scores, and 2-digit BCD levels) to PPU nametable.
 * Source address: $9FFB (main.asm lines 4021-4092).
 */
void showHighScores_real(NES_State *nes);

/*
 * Dispatches high score display if numberOfPlayers == 1, otherwise returns.
 * Source address: $9FF2 (main.asm lines 4015-4020).
 */
void showHighScores(NES_State *nes);

/*
 * Single-frame step handler for the high score entry screen.
 * Handles cursor horizontal navigation (A / Right / B / Left),
 * letter cycling (Down / Up with 44-character alphabet wrap),
 * and confirmation (Start button).
 * Returns true if name entry is finished (Start pressed), false if ongoing.
 */
bool highScoreEntryScreen_step(NES_State *nes);

/*
 * Interactive high score entry screen: sets music track 9, renderMode 2,
 * initializes cursor and row offsets, and displays high score screen.
 * Source address: $A201 (main.asm lines 4276-4438).
 */
void highScoreEntryScreen(NES_State *nes);

#ifdef __cplusplus
}
#endif

#endif /* TDECOMP_GAME_HIGHSCORE_H */
