#ifndef TDECOMP_NATIVE_GAME_ADAPTER_H
#define TDECOMP_NATIVE_GAME_ADAPTER_H

#include "common.h"
#include "nes.h"
#include "constants/gameplay.h"
#include "constants/memory.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum GameStateScreen {
    SCREEN_TITLE = 0,
    SCREEN_LEVEL_SELECT = 1,
    SCREEN_PLAYING = 2,
    SCREEN_PAUSED = 3,
    SCREEN_GAME_OVER = 4
} GameStateScreen;

typedef struct BoardCell {
    uint8_t tile_id;
    bool occupied;
} BoardCell;

typedef struct ActivePieceMino {
    int x;
    int y;
    uint8_t tile_id;
    bool visible;
} ActivePieceMino;

typedef enum AudioEventType {
    AUDIO_EVENT_NONE = 0,
    AUDIO_EVENT_MOVE,
    AUDIO_EVENT_ROTATE,
    AUDIO_EVENT_LOCK,
    AUDIO_EVENT_LINE_CLEAR,
    AUDIO_EVENT_TETRIS,
    AUDIO_EVENT_LEVEL_UP,
    AUDIO_EVENT_GAME_OVER
} AudioEventType;

typedef struct GameAdapter {
    NES_State nes;
    GameStateScreen screen;
    bool paused;

    /* Menu selections */
    uint8_t selected_level;     /* 0..9 or 0..19 */
    uint8_t selected_gametype;  /* GAMETYPE_A or GAMETYPE_B */
    uint8_t selected_height;    /* 0..5 (for B-Type) */
    uint8_t selected_music;     /* 0..3 */

    /* Decoded game statistics */
    uint32_t score;
    uint32_t high_score;
    uint32_t lines;
    uint8_t level;
    uint8_t stats[7];           /* Piece counts for T, J, Z, O, S, L, I */

    /* Active & next tetromino */
    uint8_t current_piece_type; /* 0..6 */
    uint8_t current_orientation;/* 0..18 */
    uint8_t next_piece_type;    /* 0..6 */
    uint8_t next_orientation;   /* 0..18 */
    bool show_next_piece;
    ActivePieceMino active_minos[4];

    /* Playfield 10 columns x 20 rows */
    BoardCell playfield[PLAYFIELD_HEIGHT][PLAYFIELD_WIDTH];

    /* Animation and audio flags */
    bool line_clear_anim_active;
    uint8_t line_clear_frame;
    AudioEventType pending_audio;

    uint64_t frame_count;
} GameAdapter;

/* Initialize adapter and underlying NES RAM / tables */
void game_adapter_init(GameAdapter *adapter);

/* Start a game with given level (0..19), game type (0=A, 1=B), and start height (0..5) */
void game_adapter_start_game(GameAdapter *adapter, uint8_t level, uint8_t game_type, uint8_t height);

/* Restart game with current level and game type settings */
void game_adapter_restart(GameAdapter *adapter);

/* Toggle pause during active gameplay */
void game_adapter_toggle_pause(GameAdapter *adapter);

/* Advance 1 game frame with the given controller button bitmask (BUTTON_* from constants/gameplay.h) */
void game_adapter_update(GameAdapter *adapter, uint8_t button_mask);

/* Headless stepping for automated testing and CI verification */
void game_adapter_step_headless(GameAdapter *adapter, uint32_t frames);

/* Menu navigation helpers */
void game_adapter_menu_left(GameAdapter *adapter);
void game_adapter_menu_right(GameAdapter *adapter);
void game_adapter_menu_up(GameAdapter *adapter);
void game_adapter_menu_down(GameAdapter *adapter);
void game_adapter_menu_select(GameAdapter *adapter);
void game_adapter_menu_back(GameAdapter *adapter);

#ifdef __cplusplus
}
#endif

#endif /* TDECOMP_NATIVE_GAME_ADAPTER_H */
