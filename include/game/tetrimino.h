#ifndef TDECOMP_GAME_TETRIMINO_H
#define TDECOMP_GAME_TETRIMINO_H

#include "common.h"
#include "nes.h"
#include "constants/gameplay.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Structure describing one mino block within an orientation.
 * Contains: Y offset relative to piece origin, tile ID, X offset relative to piece origin.
 */
typedef struct MinoOffset {
    int8_t y_offset;
    uint8_t tile_id;
    int8_t x_offset;
} MinoOffset;

/* Orientation definition containing 4 minos */
typedef struct OrientationDef {
    MinoOffset minos[4];
} OrientationDef;

/*
 * Collision check: verifies if current piece at (tetriminoX, tetriminoY)
 * is within playfield boundaries and does not collide with existing placed minos.
 * Source address: $894E (main.asm lines 2392-2456).
 * Returns: 0 if valid (no collision), non-zero ($FF) if collision / out of bounds.
 */
uint8_t isPositionValid(NES_State *nes);

/*
 * Handles rotation input (Button A clockwise, Button B counter-clockwise).
 * Checks collision; reverts rotation if invalid; triggers rotate sound effect if valid.
 * Source address: $88AB (main.asm lines 1465-1502).
 */
void rotate_tetrimino(NES_State *nes);

/*
 * Handles horizontal shifting (Left/Right) with Delayed Auto Shift (DAS).
 * Checks collision; reverts shift if invalid; triggers shift sound effect if valid.
 * Source address: $89AE (main.asm lines 1616-1667).
 */
void shift_tetrimino(NES_State *nes);

/*
 * Handles vertical drop (natural gravity timer and player Down soft drop).
 * Transitions playState to PLAYSTATE_LOCK_TETRIMINO (2) upon touchdown.
 * Source address: $8914 (main.asm lines 1530-1600).
 */
void drop_tetrimino(NES_State *nes);

/*
 * Updates the topmost dirty row for VRAM playfield row copy.
 * Source address: $9CE0 (main.asm lines 3532-3543).
 */
void updatePlayfield(NES_State *nes);

/* Returns frames per drop for given level (1 frame on level 29+) */
uint8_t getDropSpeed(uint8_t level);

/* Orientation table: 19 orientations * 4 minos * 3 bytes */
extern const uint8_t orientationTable[19 * 12];

/* Rotation table: 19 orientations * 2 directions (B counter-clockwise, A clockwise) */
extern const uint8_t rotationTable[19 * 2];

/* Gravity table (NTSC): levels 0 to 28 */
extern const uint8_t framesPerDropTable[29];

/* Orientation to sprite index table for next piece preview (19 bytes at $8BE5) */
extern const uint8_t orientationToSpriteTable[19];

/*
 * Stages the 4 minos of the current active tetromino into OAM staging ($0200).
 * Calculates screen coordinates from tetriminoX and tetriminoY.
 * Applies 1P vs 2P offsets and clips minos with Y < $2F (transparent tile 0xFF).
 * Source address: $8A0A (main.asm lines 1668-1758).
 */
void stageSpriteForCurrentPiece(NES_State *nes);

/*
 * Stages the next piece preview sprite into OAM staging.
 * If displayNextPiece ($DF) != 0, returns early.
 * Sets sprite offsets (X = $C8, Y = $77) and loads sprite via loadSpriteIntoOamStaging.
 * Source address: $8BCE (main.asm lines 1832-1846).
 */
void stageSpriteForNextPiece(NES_State *nes);

#ifdef __cplusplus
}
#endif

#endif /* TDECOMP_GAME_TETRIMINO_H */
