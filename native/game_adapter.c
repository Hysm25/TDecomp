#include "game_adapter.h"
#include "game/state.h"
#include "game/menu.h"
#include "game/input.h"
#include "game/tetrimino.h"
#include "game/playfield.h"
#include "game/render.h"
#include "game/audio.h"
#include "game/highscore.h"
#include "math/bcd.h"
#include "math/rng.h"
#include <string.h>

static inline uint32_t bcd_to_dec(uint8_t val) {
    return (uint32_t)((val >> 4) * 10 + (val & 0x0F));
}

static void sync_state(GameAdapter *adapter) {
    if (!adapter) return;

    /* Decode score (3 BCD bytes: low, mid, high) */
    uint8_t s_lo  = adapter->nes.ram[ADDR_P1_SCORE];
    uint8_t s_mid = adapter->nes.ram[ADDR_P1_SCORE + 1];
    uint8_t s_hi  = adapter->nes.ram[ADDR_P1_SCORE + 2];
    adapter->score = bcd_to_dec(s_lo) +
                     bcd_to_dec(s_mid) * 100 +
                     bcd_to_dec(s_hi)  * 10000;

    /* Decode high score (Type A top score at ADDR_HIGH_SCORE_A) */
    uint8_t hs_hi  = adapter->nes.ram[ADDR_HIGH_SCORE_A];
    uint8_t hs_mid = adapter->nes.ram[ADDR_HIGH_SCORE_A + 1];
    uint8_t hs_lo  = adapter->nes.ram[ADDR_HIGH_SCORE_A + 2];
    adapter->high_score = bcd_to_dec(hs_lo) +
                          bcd_to_dec(hs_mid) * 100 +
                          bcd_to_dec(hs_hi)  * 10000;
    if (adapter->score > adapter->high_score) {
        adapter->high_score = adapter->score;
    }

    /* Decode lines (2 BCD bytes: low, high) */
    uint8_t l_lo = adapter->nes.ram[ADDR_P1_LINES];
    uint8_t l_hi = adapter->nes.ram[ADDR_P1_LINES + 1];
    adapter->lines = bcd_to_dec(l_lo) + bcd_to_dec(l_hi) * 100;

    /* Decode level */
    adapter->level = adapter->nes.ram[ADDR_P1_LEVEL_NUMBER];

    /* Decode piece statistics (T, J, Z, O, S, L, I) */
    for (int p = 0; p < 7; p++) {
        uint8_t st_lo = adapter->nes.ram[ADDR_STATS_BY_TYPE + p * 2];
        uint8_t st_hi = adapter->nes.ram[ADDR_STATS_BY_TYPE + p * 2 + 1];
        adapter->stats[p] = (uint8_t)(bcd_to_dec(st_lo) + bcd_to_dec(st_hi) * 100);
    }

    /* Decode playfield matrix (10 cols x 20 rows) */
    for (int r = 0; r < PLAYFIELD_HEIGHT; r++) {
        for (int c = 0; c < PLAYFIELD_WIDTH; c++) {
            uint8_t t = adapter->nes.ram[ADDR_PLAYFIELD + r * 10 + c];
            adapter->playfield[r][c].tile_id = t;
            adapter->playfield[r][c].occupied = (t != TILE_EMPTY && t != TILE_HIDDEN);
        }
    }

    /* Decode active falling tetromino */
    uint8_t orient = adapter->nes.ram[ADDR_P1_CURRENT_PIECE];
    adapter->current_orientation = orient;
    if (orient < ORIENTATION_COUNT && adapter->nes.ram[ADDR_P1_PLAY_STATE] == PLAYSTATE_CONTROL_ACTIVE) {
        int px = (int)adapter->nes.ram[ADDR_P1_TETRIMINO_X];
        int py = (int)adapter->nes.ram[ADDR_P1_TETRIMINO_Y];
        for (int m = 0; m < 4; m++) {
            int8_t my  = (int8_t)orientationTable[orient * 12 + m * 3 + 0];
            uint8_t mt = orientationTable[orient * 12 + m * 3 + 1];
            int8_t mx  = (int8_t)orientationTable[orient * 12 + m * 3 + 2];
            adapter->active_minos[m].x = px + mx;
            adapter->active_minos[m].y = py + my;
            adapter->active_minos[m].tile_id = mt;
            adapter->active_minos[m].visible = (py + my >= 0);
        }
    } else {
        for (int m = 0; m < 4; m++) {
            adapter->active_minos[m].visible = false;
        }
    }

    /* Decode next tetromino preview */
    uint8_t next_orient = adapter->nes.ram[ADDR_NEXT_PIECE];
    adapter->next_orientation = next_orient;
    adapter->show_next_piece = (adapter->nes.ram[ADDR_DISPLAY_NEXT_PIECE] == 0);

    /* Line clear animation state */
    adapter->line_clear_anim_active = (adapter->nes.ram[ADDR_P1_PLAY_STATE] == PLAYSTATE_LINE_CLEAR_ANIM);
}

void game_adapter_init(GameAdapter *adapter) {
    if (!adapter) return;
    memset(adapter, 0, sizeof(GameAdapter));

    nes_init(&adapter->nes);
    initHighScoreTable(&adapter->nes);

    adapter->screen = SCREEN_TITLE;
    adapter->paused = false;
    adapter->selected_level = 0;
    adapter->selected_gametype = GAMETYPE_A;
    adapter->selected_height = 0;
    adapter->selected_music = 1;

    sync_state(adapter);
}

void game_adapter_start_game(GameAdapter *adapter, uint8_t level, uint8_t game_type, uint8_t height) {
    if (!adapter) return;

    nes_init(&adapter->nes);
    initHighScoreTable(&adapter->nes);

    adapter->nes.ram[ADDR_NUMBER_OF_PLAYERS] = 1;
    adapter->nes.ram[ADDR_GAME_TYPE] = game_type;
    adapter->nes.ram[ADDR_P1_START_LEVEL] = level;
    adapter->nes.ram[ADDR_P1_LEVEL_NUMBER] = level;
    adapter->nes.ram[ADDR_START_HEIGHT] = height;
    adapter->nes.ram[ADDR_P1_START_HEIGHT] = height;
    adapter->nes.ram[ADDR_MUSIC_TYPE] = adapter->selected_music;

    adapter->selected_level = level;
    adapter->selected_gametype = game_type;
    adapter->selected_height = height;

    /* Initialize background nametable and initial game state */
    adapter->nes.ram[ADDR_GAME_MODE] = MODE_PLAY;
    adapter->nes.ram[ADDR_GAME_MODE_STATE] = 0;
    gameModeState_initGameBackground(&adapter->nes);
    gameModeState_initGameState(&adapter->nes);

    adapter->screen = SCREEN_PLAYING;
    adapter->paused = false;
    adapter->pending_audio = AUDIO_EVENT_NONE;
    adapter->frame_count = 0;

    sync_state(adapter);
}

void game_adapter_restart(GameAdapter *adapter) {
    if (!adapter) return;
    game_adapter_start_game(adapter, adapter->selected_level, adapter->selected_gametype, adapter->selected_height);
}

void game_adapter_toggle_pause(GameAdapter *adapter) {
    if (!adapter) return;
    if (adapter->screen == SCREEN_PLAYING) {
        adapter->screen = SCREEN_PAUSED;
        adapter->paused = true;
    } else if (adapter->screen == SCREEN_PAUSED) {
        adapter->screen = SCREEN_PLAYING;
        adapter->paused = false;
    }
}

void game_adapter_update(GameAdapter *adapter, uint8_t button_mask) {
    if (!adapter) return;

    adapter->pending_audio = AUDIO_EVENT_NONE;

    if (adapter->screen != SCREEN_PLAYING) {
        return;
    }

    if (adapter->paused) {
        return;
    }

    /* Record state before frame execution for audio event detection */
    uint8_t prev_lvl        = adapter->nes.ram[ADDR_P1_LEVEL_NUMBER];
    uint8_t prev_playstate  = adapter->nes.ram[ADDR_P1_PLAY_STATE];
    uint8_t prev_sfx1       = adapter->nes.ram[ADDR_SOUND_EFFECT_SLOT1_INIT];

    /* Map controller input bitmask */
    nes_set_controller(&adapter->nes, 0, button_mask);
    pollControllerButtons(&adapter->nes);

    /* Video render step (drives line clear animation flashing and level palettes) */
    render(&adapter->nes);

    /* Advance the authentic state machine for exactly one frame (states 2..8) */
    while (adapter->nes.ram[ADDR_GAME_MODE_STATE] >= 2 && adapter->nes.ram[ADDR_GAME_MODE_STATE] <= 8) {
        uint8_t prev_state = adapter->nes.ram[ADDR_GAME_MODE_STATE];
        gameMode_playAndEndingHighScore(&adapter->nes);
        if (adapter->nes.ram[ADDR_GAME_MODE_STATE] == 2) {
            break; /* Completed exactly one full frame */
        }
        if (adapter->nes.ram[ADDR_GAME_MODE_STATE] == prev_state) {
            break; /* State didn't advance, avoid loop */
        }
    }

    /* Audio sequencer step */
    updateAudio(&adapter->nes);

    /* Detect audio triggers from decompiled sound effect registers */
    uint8_t curr_sfx1 = adapter->nes.ram[ADDR_SOUND_EFFECT_SLOT1_INIT];
    uint8_t curr_playstate = adapter->nes.ram[ADDR_P1_PLAY_STATE];

    if (adapter->nes.ram[ADDR_P1_LEVEL_NUMBER] > prev_lvl) {
        adapter->pending_audio = AUDIO_EVENT_LEVEL_UP;
    } else if (curr_playstate == PLAYSTATE_LINE_CLEAR_ANIM && prev_playstate != PLAYSTATE_LINE_CLEAR_ANIM) {
        if (adapter->nes.ram[ADDR_COMPLETED_LINES] == 4) {
            adapter->pending_audio = AUDIO_EVENT_TETRIS;
        } else {
            adapter->pending_audio = AUDIO_EVENT_LINE_CLEAR;
        }
    } else if (curr_playstate == PLAYSTATE_LOCK_TETRIMINO && prev_playstate == PLAYSTATE_CONTROL_ACTIVE) {
        adapter->pending_audio = AUDIO_EVENT_LOCK;
    } else if (curr_playstate == PLAYSTATE_CURTAIN_FALL && prev_playstate != PLAYSTATE_CURTAIN_FALL) {
        adapter->pending_audio = AUDIO_EVENT_GAME_OVER;
    } else if (curr_sfx1 != prev_sfx1 && curr_sfx1 == 0x05) {
        adapter->pending_audio = AUDIO_EVENT_ROTATE;
    } else if (curr_sfx1 != prev_sfx1 && curr_sfx1 == 0x03) {
        adapter->pending_audio = AUDIO_EVENT_MOVE;
    }

    /* Check game over condition */
    if (adapter->nes.ram[ADDR_P1_PLAY_STATE] == 0 || adapter->nes.ram[ADDR_GAME_MODE] != MODE_PLAY) {
        adapter->screen = SCREEN_GAME_OVER;
    }

    adapter->frame_count++;
    sync_state(adapter);
}

void game_adapter_step_headless(GameAdapter *adapter, uint32_t frames) {
    if (!adapter) return;
    for (uint32_t i = 0; i < frames; i++) {
        game_adapter_update(adapter, 0);
    }
}

void game_adapter_menu_left(GameAdapter *adapter) {
    if (!adapter) return;
    if (adapter->screen == SCREEN_LEVEL_SELECT) {
        if (adapter->selected_level > 0) {
            adapter->selected_level--;
        }
    }
}

void game_adapter_menu_right(GameAdapter *adapter) {
    if (!adapter) return;
    if (adapter->screen == SCREEN_LEVEL_SELECT) {
        if (adapter->selected_level < 19) {
            adapter->selected_level++;
        }
    }
}

void game_adapter_menu_up(GameAdapter *adapter) {
    if (!adapter) return;
    if (adapter->screen == SCREEN_LEVEL_SELECT) {
        if (adapter->selected_level >= 5) {
            adapter->selected_level -= 5;
        }
    }
}

void game_adapter_menu_down(GameAdapter *adapter) {
    if (!adapter) return;
    if (adapter->screen == SCREEN_LEVEL_SELECT) {
        if (adapter->selected_level + 5 <= 19) {
            adapter->selected_level += 5;
        }
    }
}

void game_adapter_menu_select(GameAdapter *adapter) {
    if (!adapter) return;
    if (adapter->screen == SCREEN_TITLE) {
        adapter->screen = SCREEN_LEVEL_SELECT;
    } else if (adapter->screen == SCREEN_LEVEL_SELECT) {
        game_adapter_start_game(adapter, adapter->selected_level, adapter->selected_gametype, adapter->selected_height);
    } else if (adapter->screen == SCREEN_GAME_OVER) {
        game_adapter_start_game(adapter, adapter->selected_level, adapter->selected_gametype, adapter->selected_height);
    }
}

void game_adapter_menu_back(GameAdapter *adapter) {
    if (!adapter) return;
    if (adapter->screen == SCREEN_LEVEL_SELECT) {
        adapter->screen = SCREEN_TITLE;
    }
}
