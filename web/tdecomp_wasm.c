#include <emscripten.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "nes.h"
#include "constants/hardware.h"
#include "constants/memory.h"
#include "constants/gameplay.h"
#include "game/system.h"
#include "game/state.h"
#include "game/tetrimino.h"
#include "game/playfield.h"
#include "game/input.h"
#include "game/menu.h"
#include "game/highscore.h"
#include "math/rng.h"
#include "math/bcd.h"

static NES_State s_nes;
static int s_saved_start_level = 0;
static int s_saved_game_type = 0;

/* Converts 3 BCD bytes (little-endian: low, mid, high) to uint32_t */
static uint32_t bcd3_to_uint32(const uint8_t *bcd) {
    uint32_t val = 0;
    val += ((uint32_t)((bcd[2] >> 4) * 10 + (bcd[2] & 0x0F))) * 10000;
    val += ((uint32_t)((bcd[1] >> 4) * 10 + (bcd[1] & 0x0F))) * 100;
    val += ((uint32_t)((bcd[0] >> 4) * 10 + (bcd[0] & 0x0F)));
    return val;
}

/* Converts 2 BCD bytes (little-endian: low, high) to uint32_t */
static uint32_t bcd2_to_uint32(const uint8_t *bcd) {
    uint32_t val = 0;
    val += ((uint32_t)((bcd[1] >> 4) * 10 + (bcd[1] & 0x0F))) * 100;
    val += ((uint32_t)((bcd[0] >> 4) * 10 + (bcd[0] & 0x0F)));
    return val;
}

EMSCRIPTEN_KEEPALIVE
int tdecomp_wasm_init(int start_level, int game_type) {
    if (start_level < 0) start_level = 0;
    if (start_level > 29) start_level = 29;
    if (game_type != GAMETYPE_A && game_type != GAMETYPE_B) {
        game_type = GAMETYPE_A;
    }
    s_saved_start_level = start_level;
    s_saved_game_type = game_type;

    nes_init(&s_nes);
    initHighScoreTable(&s_nes);

    s_nes.ram[ADDR_RNG_SEED] = 0x89;
    s_nes.ram[ADDR_RNG_SEED + 1] = 0x88;
    s_nes.ram[ADDR_P1_START_LEVEL] = (uint8_t)start_level;
    s_nes.ram[ADDR_START_LEVEL] = (uint8_t)start_level;
    s_nes.ram[ADDR_LEVEL_NUMBER] = (uint8_t)start_level;
    s_nes.ram[ADDR_P1_LEVEL_NUMBER] = (uint8_t)start_level;
    s_nes.ram[ADDR_GAME_TYPE] = (uint8_t)game_type;
    s_nes.ram[ADDR_GAME_MODE] = MODE_PLAY;
    s_nes.ram[ADDR_NUMBER_OF_PLAYERS] = 1;
    s_nes.ram[ADDR_GAME_MODE_STATE] = 0;

    gameModeState_initGameBackground(&s_nes);
    gameModeState_initGameState(&s_nes);

    return 0;
}

EMSCRIPTEN_KEEPALIVE
int tdecomp_wasm_reset(void) {
    return tdecomp_wasm_init(s_saved_start_level, s_saved_game_type);
}

EMSCRIPTEN_KEEPALIVE
int tdecomp_wasm_step_frame(int button_mask) {
    if (s_nes.ram[ADDR_GAME_MODE] != MODE_PLAY) {
        return 1; /* Game over */
    }

    nes_set_controller(&s_nes, 0, (uint8_t)button_mask);
    nmi(&s_nes);

    int max_steps = 10;
    do {
        gameMode_playAndEndingHighScore(&s_nes);
        max_steps--;
    } while (s_nes.ram[ADDR_GAME_MODE_STATE] != 2 &&
             s_nes.ram[ADDR_GAME_MODE] == MODE_PLAY &&
             max_steps > 0);

    return (s_nes.ram[ADDR_GAME_MODE] != MODE_PLAY) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
int tdecomp_wasm_action_left(void) {
    if (s_nes.ram[ADDR_GAME_MODE] != MODE_PLAY) return 0;
    makePlayer1Active(&s_nes);
    if (s_nes.ram[ADDR_PLAY_STATE] != PLAYSTATE_CONTROL_ACTIVE) {
        savePlayer1State(&s_nes);
        return 0;
    }
    uint8_t old_x = s_nes.ram[ADDR_TETRIMINO_X];
    s_nes.ram[ADDR_ORIGINAL_Y] = old_x;
    s_nes.ram[ADDR_TETRIMINO_X] = (uint8_t)(old_x - 1);
    if (isPositionValid(&s_nes) != 0) {
        s_nes.ram[ADDR_TETRIMINO_X] = old_x;
        savePlayer1State(&s_nes);
        return 0;
    }
    s_nes.ram[ADDR_AUTOREPEAT_X] = 0;
    savePlayer1State(&s_nes);
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int tdecomp_wasm_action_right(void) {
    if (s_nes.ram[ADDR_GAME_MODE] != MODE_PLAY) return 0;
    makePlayer1Active(&s_nes);
    if (s_nes.ram[ADDR_PLAY_STATE] != PLAYSTATE_CONTROL_ACTIVE) {
        savePlayer1State(&s_nes);
        return 0;
    }
    uint8_t old_x = s_nes.ram[ADDR_TETRIMINO_X];
    s_nes.ram[ADDR_ORIGINAL_Y] = old_x;
    s_nes.ram[ADDR_TETRIMINO_X] = (uint8_t)(old_x + 1);
    if (isPositionValid(&s_nes) != 0) {
        s_nes.ram[ADDR_TETRIMINO_X] = old_x;
        savePlayer1State(&s_nes);
        return 0;
    }
    s_nes.ram[ADDR_AUTOREPEAT_X] = 0;
    savePlayer1State(&s_nes);
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int tdecomp_wasm_action_rotate_cw(void) {
    if (s_nes.ram[ADDR_GAME_MODE] != MODE_PLAY) return 0;
    makePlayer1Active(&s_nes);
    if (s_nes.ram[ADDR_PLAY_STATE] != PLAYSTATE_CONTROL_ACTIVE) {
        savePlayer1State(&s_nes);
        return 0;
    }
    s_nes.ram[ADDR_NEWLY_PRESSED_BUTTONS] = BUTTON_A;
    rotate_tetrimino(&s_nes);
    savePlayer1State(&s_nes);
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int tdecomp_wasm_action_rotate_ccw(void) {
    if (s_nes.ram[ADDR_GAME_MODE] != MODE_PLAY) return 0;
    makePlayer1Active(&s_nes);
    if (s_nes.ram[ADDR_PLAY_STATE] != PLAYSTATE_CONTROL_ACTIVE) {
        savePlayer1State(&s_nes);
        return 0;
    }
    s_nes.ram[ADDR_NEWLY_PRESSED_BUTTONS] = BUTTON_B;
    rotate_tetrimino(&s_nes);
    savePlayer1State(&s_nes);
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int tdecomp_wasm_action_drop(void) {
    if (s_nes.ram[ADDR_GAME_MODE] != MODE_PLAY) return 0;
    makePlayer1Active(&s_nes);
    if (s_nes.ram[ADDR_PLAY_STATE] != PLAYSTATE_CONTROL_ACTIVE) {
        savePlayer1State(&s_nes);
        return 0;
    }
    uint8_t old_y = s_nes.ram[ADDR_TETRIMINO_Y];
    s_nes.ram[ADDR_ORIGINAL_Y] = old_y;
    s_nes.ram[ADDR_TETRIMINO_Y] = (uint8_t)(old_y + 1);
    if (isPositionValid(&s_nes) == 0) {
        savePlayer1State(&s_nes);
        return 1;
    }
    s_nes.ram[ADDR_TETRIMINO_Y] = old_y;
    s_nes.ram[ADDR_PLAY_STATE] = PLAYSTATE_LOCK_TETRIMINO;
    updatePlayfield(&s_nes);
    savePlayer1State(&s_nes);
    return 2; /* Locked */
}

EMSCRIPTEN_KEEPALIVE
int tdecomp_wasm_action_hard_drop(void) {
    if (s_nes.ram[ADDR_GAME_MODE] != MODE_PLAY) return 0;
    int drops = 0;
    while (drops < 22) {
        int res = tdecomp_wasm_action_drop();
        if (res == 2 || res == 0) break;
        drops++;
    }
    return drops;
}

EMSCRIPTEN_KEEPALIVE
const uint8_t *tdecomp_wasm_get_playfield(void) {
    return &s_nes.ram[ADDR_PLAYFIELD];
}

EMSCRIPTEN_KEEPALIVE
int tdecomp_wasm_get_playfield_cell(int x, int y) {
    if (x < 0 || x >= PLAYFIELD_WIDTH || y < 0 || y >= PLAYFIELD_HEIGHT) {
        return 0xFF;
    }
    return s_nes.ram[ADDR_PLAYFIELD + y * PLAYFIELD_WIDTH + x];
}

EMSCRIPTEN_KEEPALIVE
int tdecomp_wasm_get_current_piece(void) {
    return s_nes.ram[ADDR_P1_CURRENT_PIECE];
}

EMSCRIPTEN_KEEPALIVE
int tdecomp_wasm_get_current_x(void) {
    return (int)(int8_t)s_nes.ram[ADDR_P1_TETRIMINO_X];
}

EMSCRIPTEN_KEEPALIVE
int tdecomp_wasm_get_current_y(void) {
    return (int)(int8_t)s_nes.ram[ADDR_P1_TETRIMINO_Y];
}

EMSCRIPTEN_KEEPALIVE
int tdecomp_wasm_get_next_piece(void) {
    return s_nes.ram[ADDR_NEXT_PIECE];
}

EMSCRIPTEN_KEEPALIVE
uint32_t tdecomp_wasm_get_score(void) {
    return bcd3_to_uint32(&s_nes.ram[ADDR_P1_SCORE]);
}

EMSCRIPTEN_KEEPALIVE
uint32_t tdecomp_wasm_get_lines(void) {
    return bcd2_to_uint32(&s_nes.ram[ADDR_P1_LINES]);
}

EMSCRIPTEN_KEEPALIVE
int tdecomp_wasm_get_level(void) {
    return s_nes.ram[ADDR_P1_LEVEL_NUMBER];
}

EMSCRIPTEN_KEEPALIVE
int tdecomp_wasm_get_play_state(void) {
    return s_nes.ram[ADDR_P1_PLAY_STATE];
}

EMSCRIPTEN_KEEPALIVE
int tdecomp_wasm_is_game_over(void) {
    return (s_nes.ram[ADDR_GAME_MODE] != MODE_PLAY) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
int tdecomp_wasm_get_mino_x(int orientation, int mino_idx) {
    if (orientation < 0 || orientation >= ORIENTATION_COUNT || mino_idx < 0 || mino_idx >= 4) {
        return 0;
    }
    return (int)(int8_t)orientationTable[orientation * 12 + mino_idx * 3 + 2];
}

EMSCRIPTEN_KEEPALIVE
int tdecomp_wasm_get_mino_y(int orientation, int mino_idx) {
    if (orientation < 0 || orientation >= ORIENTATION_COUNT || mino_idx < 0 || mino_idx >= 4) {
        return 0;
    }
    return (int)(int8_t)orientationTable[orientation * 12 + mino_idx * 3 + 0];
}

EMSCRIPTEN_KEEPALIVE
int tdecomp_wasm_get_mino_tile(int orientation, int mino_idx) {
    if (orientation < 0 || orientation >= ORIENTATION_COUNT || mino_idx < 0 || mino_idx >= 4) {
        return 0xFF;
    }
    return (int)orientationTable[orientation * 12 + mino_idx * 3 + 1];
}

EMSCRIPTEN_KEEPALIVE
int tdecomp_wasm_get_stat(int piece_id) {
    if (piece_id < 0 || piece_id >= PIECE_COUNT) return 0;
    return (int)bcd2_to_uint32(&s_nes.ram[ADDR_STATS_BY_TYPE + piece_id * 2]);
}

EMSCRIPTEN_KEEPALIVE
uint32_t tdecomp_wasm_get_high_score(int rank) {
    if (rank < 0 || rank >= 3) return 0;
    uint16_t addr = (s_saved_game_type == GAMETYPE_B) ? ADDR_HIGH_SCORE_B : ADDR_HIGH_SCORE_A;
    return bcd3_to_uint32(&s_nes.ram[addr + rank * 3]);
}

EMSCRIPTEN_KEEPALIVE
const NES_State *tdecomp_wasm_get_nes_state_ptr(void) {
    return &s_nes;
}
