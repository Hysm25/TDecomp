#ifndef TDECOMP_CONSTANTS_GAMEPLAY_H
#define TDECOMP_CONSTANTS_GAMEPLAY_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Controller Buttons (Joypad active-high mask) */
#define BUTTON_RIGHT    0x01
#define BUTTON_LEFT     0x02
#define BUTTON_DOWN     0x04
#define BUTTON_UP       0x08
#define BUTTON_START    0x10
#define BUTTON_SELECT   0x20
#define BUTTON_B        0x40
#define BUTTON_A        0x80
#define BUTTON_DPAD     (BUTTON_UP | BUTTON_DOWN | BUTTON_LEFT | BUTTON_RIGHT)

/* Piece IDs (7 tetrominoes) */
#define PIECE_T         0x00
#define PIECE_J         0x01
#define PIECE_Z         0x02
#define PIECE_O         0x03
#define PIECE_S         0x04
#define PIECE_L         0x05
#define PIECE_I         0x06
#define PIECE_COUNT     7

/* Piece Orientations (19 distinct states + hidden) */
#define ORIENT_T_UP     0x00
#define ORIENT_T_RIGHT  0x01
#define ORIENT_T_DOWN   0x02
#define ORIENT_T_LEFT   0x03

#define ORIENT_J_LEFT   0x04
#define ORIENT_J_UP     0x05
#define ORIENT_J_RIGHT  0x06
#define ORIENT_J_DOWN   0x07

#define ORIENT_Z_HORIZ  0x08
#define ORIENT_Z_VERT   0x09

#define ORIENT_O_FIXED  0x0A

#define ORIENT_S_HORIZ  0x0B
#define ORIENT_S_VERT   0x0C

#define ORIENT_L_RIGHT  0x0D
#define ORIENT_L_DOWN   0x0E
#define ORIENT_L_LEFT   0x0F
#define ORIENT_L_UP     0x10

#define ORIENT_I_VERT   0x11
#define ORIENT_I_HORIZ  0x12

#define ORIENT_HIDDEN   0x13
#define ORIENTATION_COUNT 19

/* Playfield Tile IDs */
#define TILE_1          0x7B
#define TILE_2          0x7C
#define TILE_3          0x7D
#define TILE_EMPTY      0xEF  /* Blank tile used for empty squares */
#define TILE_HIDDEN     0xFF  /* Blank tile used to hide minos */

/* Playfield Dimensions */
#define PLAYFIELD_WIDTH   10
#define PLAYFIELD_HEIGHT  20
#define PLAYFIELD_SIZE    (PLAYFIELD_WIDTH * PLAYFIELD_HEIGHT) /* 200 ($C8) */

/* Game Modes */
#define MODE_LEGAL_SCREEN           0
#define MODE_TITLE_SCREEN           1
#define MODE_GAME_TYPE_MENU         2
#define MODE_LEVEL_MENU             3
#define MODE_PLAY                   4
#define MODE_PLAY_ENDING_HIGH_SCORE 5

/* Play States within Mode 4 */
#define PLAYSTATE_CONTROL_ACTIVE     0
#define PLAYSTATE_LOCK_TETRIMINO     1
#define PLAYSTATE_CHECK_ROWS         2
#define PLAYSTATE_LINE_CLEAR_ANIM    3
#define PLAYSTATE_UPDATE_COUNTERS    4
#define PLAYSTATE_SPAWN_NEXT         5
#define PLAYSTATE_CURTAIN_FALL       6
#define PLAYSTATE_GAME_OVER          7

/* Game Types */
#define GAMETYPE_A      0
#define GAMETYPE_B      1

/* Delayed Auto Shift (DAS) Timings (NTSC) */
#define DAS_DELAY_NTSC          0x0A  /* 10 frames */
#define DAS_RESET_NTSC          0x10  /* 16 frames initial counter */
#define INITIAL_AUTOREPEAT_Y_NTSC 0xA0

/* Delayed Auto Shift (DAS) Timings (PAL) */
#define DAS_DELAY_PAL           0x08  /* 8 frames */
#define DAS_RESET_PAL           0x0C  /* 12 frames */
#define INITIAL_AUTOREPEAT_Y_PAL 0xB4

/* Render Update Flags */
#define RENDER_LINES            0x01
#define RENDER_LEVEL            0x02
#define RENDER_SCORE            0x04
#define RENDER_STATS            0x40
#define RENDER_HIGH_SCORE_LETTER 0x80

#ifdef __cplusplus
}
#endif

#endif /* TDECOMP_CONSTANTS_GAMEPLAY_H */
