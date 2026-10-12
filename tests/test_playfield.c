#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "nes.h"
#include "game/playfield.h"
#include "game/tetrimino.h"
#include "math/rng.h"
#include "constants/gameplay.h"
#include "constants/memory.h"

static void test_playfield_lock_valid(void) {
    NES_State nes;
    nes_init(&nes);

    /* Setup T-Down piece (orientation 2) at (4, 5) */
    nes.ram[ADDR_TETRIMINO_X] = 4;
    nes.ram[ADDR_TETRIMINO_Y] = 5;
    nes.ram[ADDR_CURRENT_PIECE] = ORIENT_T_DOWN;
    nes.ram[ADDR_VRAM_ROW] = 0x20;
    nes.ram[ADDR_PLAY_STATE] = 2; /* PLAYSTATE_LOCK_TETRIMINO */
    nes.ram[ADDR_NUMBER_OF_PLAYERS] = 1;

    playState_lockTetrimino(&nes);

    /* Verify playState advanced to 3 */
    assert(nes.ram[ADDR_PLAY_STATE] == 3);
    assert(nes.ram[ADDR_LINE_INDEX] == 0);

    /* Verify minos locked into playfield */
    /* T-Down minos: (0, -1), (0, 0), (0, 1), (1, 0) relative to (4, 5) */
    uint16_t pf_base = ADDR_PLAYFIELD;
    assert(nes.ram[pf_base + 5 * 10 + 3] == TILE_1);
    assert(nes.ram[pf_base + 5 * 10 + 4] == TILE_1);
    assert(nes.ram[pf_base + 5 * 10 + 5] == TILE_1);
    assert(nes.ram[pf_base + 6 * 10 + 4] == TILE_1);
    (void)pf_base;
}

static void test_playfield_lock_invalid_game_over(void) {
    NES_State nes;
    nes_init(&nes);

    /* Place piece out of bounds */
    nes.ram[ADDR_TETRIMINO_X] = 12; /* Out of horizontal bounds */
    nes.ram[ADDR_TETRIMINO_Y] = 5;
    nes.ram[ADDR_CURRENT_PIECE] = ORIENT_T_DOWN;
    nes.ram[ADDR_VRAM_ROW] = 0x20;
    nes.ram[ADDR_PLAY_STATE] = 2;

    playState_lockTetrimino(&nes);

    /* Must trigger game over state */
    assert(nes.ram[ADDR_PLAY_STATE] == 0x0A);
    assert(nes.ram[ADDR_SOUND_EFFECT_SLOT0_INIT] == 0x02);
    assert(nes.ram[ADDR_CURTAIN_ROW] == 0xF0);
}

static void test_playfield_checkForCompletedRows_single(void) {
    NES_State nes;
    nes_init(&nes);

    /* Fill row 18 completely with TILE_1 */
    uint16_t pf_base = ADDR_PLAYFIELD;
    for (int col = 0; col < 10; col++) {
        nes.ram[pf_base + 18 * 10 + col] = TILE_1;
    }
    /* Place one block in row 17 */
    nes.ram[pf_base + 17 * 10 + 3] = TILE_2;

    nes.ram[ADDR_TETRIMINO_Y] = 19;
    nes.ram[ADDR_VRAM_ROW] = 0x20;
    nes.ram[ADDR_LINE_INDEX] = 0;
    nes.ram[ADDR_COMPLETED_LINES] = 0;
    nes.ram[ADDR_PLAY_STATE] = 3;

    /* Execute 4 scan iterations for candidate rows */
    for (int i = 0; i < 4; i++) {
        playState_checkForCompletedRows(&nes);
    }

    assert(nes.ram[ADDR_COMPLETED_LINES] == 1);
    assert(nes.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == 0x0A);

    /* Row 18 should now have the collapsed content from row 17 */
    assert(nes.ram[pf_base + 18 * 10 + 3] == TILE_2);

    /* Top row 0 should be empty */
    for (int col = 0; col < 10; col++) {
        assert(nes.ram[pf_base + col] == TILE_EMPTY);
    }

    /* playState transitions to 4 (line clear anim) */
    assert(nes.ram[ADDR_PLAY_STATE] == 4);
}

static void test_playfield_checkForCompletedRows_tetris(void) {
    NES_State nes;
    nes_init(&nes);

    /* Fill rows 16, 17, 18, 19 completely */
    uint16_t pf_base = ADDR_PLAYFIELD;
    for (int r = 16; r < 20; r++) {
        for (int c = 0; c < 10; c++) {
            nes.ram[pf_base + r * 10 + c] = TILE_3;
        }
    }

    /* tetriminoY = 18 -> check rows 16, 17, 18, 19 */
    nes.ram[ADDR_TETRIMINO_Y] = 18;
    nes.ram[ADDR_VRAM_ROW] = 0x20;
    nes.ram[ADDR_LINE_INDEX] = 0;
    nes.ram[ADDR_COMPLETED_LINES] = 0;
    nes.ram[ADDR_PENDING_GARBAGE_INACTIVE] = 0;
    nes.ram[ADDR_PLAY_STATE] = 3;

    for (int i = 0; i < 4; i++) {
        playState_checkForCompletedRows(&nes);
    }

    assert(nes.ram[ADDR_COMPLETED_LINES] == 4);
    assert(nes.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == 0x04); /* Tetris SFX */
    assert(nes.ram[ADDR_PENDING_GARBAGE_INACTIVE] == 4);    /* 4 lines of garbage */
    assert(nes.ram[ADDR_PLAY_STATE] == 4);
}

static void test_playfield_checkForCompletedRows_zero_lines(void) {
    NES_State nes;
    nes_init(&nes);

    nes.ram[ADDR_TETRIMINO_Y] = 10;
    nes.ram[ADDR_VRAM_ROW] = 0x20;
    nes.ram[ADDR_LINE_INDEX] = 0;
    nes.ram[ADDR_COMPLETED_LINES] = 0;
    nes.ram[ADDR_PLAY_STATE] = 3;

    for (int i = 0; i < 4; i++) {
        playState_checkForCompletedRows(&nes);
    }

    assert(nes.ram[ADDR_COMPLETED_LINES] == 0);
    assert(nes.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == 0x07); /* Thud lock SFX */
    /* Must skip PlayState 4 straight to PlayState 5 */
    assert(nes.ram[ADDR_PLAY_STATE] == 5);
}

static void test_playfield_line_clear_anim(void) {
    NES_State nes;
    nes_init(&nes);

    nes.ram[ADDR_ROW_Y] = 0;
    nes.ram[ADDR_PLAY_STATE] = 4;
    nes.ram[ADDR_FRAME_COUNTER] = 0; /* frameCounter & 3 == 0 */

    for (int step = 0; step < 5; step++) {
        updateLineClearingAnimation(&nes);
    }

    assert(nes.ram[ADDR_ROW_Y] == 5);
    assert(nes.ram[ADDR_PLAY_STATE] == 5);
}

static void test_playfield_game_over_curtain(void) {
    NES_State nes;
    nes_init(&nes);

    nes.ram[ADDR_CURTAIN_ROW] = 0;
    nes.ram[ADDR_FRAME_COUNTER] = 0;

    for (int r = 0; r < 20; r++) {
        playState_updateGameOverCurtain(&nes);
    }

    assert(nes.ram[ADDR_CURTAIN_ROW] == 20);
    uint16_t pf_base = ADDR_PLAYFIELD;
    for (int i = 0; i < PLAYFIELD_SIZE; i++) {
        assert(nes.ram[pf_base + i] == TILE_CURTAIN);
    }
    (void)pf_base;
}

static void test_playfield_receive_garbage(void) {
    NES_State nes;
    nes_init(&nes);

    nes.ram[ADDR_NUMBER_OF_PLAYERS] = 2;
    nes.ram[ADDR_PENDING_GARBAGE] = 3;
    nes.ram[ADDR_GARBAGE_HOLE] = 4;
    nes.ram[ADDR_VRAM_ROW] = 0x20;
    nes.ram[ADDR_PLAY_STATE] = 7;

    uint16_t pf_base = ADDR_PLAYFIELD;
    /* Put sample blocks in row 19 */
    nes.ram[pf_base + 19 * 10 + 1] = TILE_1;

    playState_receiveGarbage(&nes);

    assert(nes.ram[ADDR_PLAY_STATE] == 8);
    assert(nes.ram[ADDR_PENDING_GARBAGE] == 0);

    /* Row 19 block should have shifted up 3 rows to row 16 */
    assert(nes.ram[pf_base + 16 * 10 + 1] == TILE_1);

    /* Bottom 3 rows (17, 18, 19) are garbage with hole at col 4 */
    for (int r = 17; r < 20; r++) {
        for (int c = 0; c < 10; c++) {
            if (c == 4) {
                assert(nes.ram[pf_base + r * 10 + c] == TILE_HIDDEN);
            } else {
                assert(nes.ram[pf_base + r * 10 + c] == TILE_GARBAGE);
            }
        }
    }
}

static void test_playfield_type_b_init(void) {
    NES_State nes;
    nes_init(&nes);

    nes.ram[ADDR_GAME_TYPE] = GAMETYPE_B;
    nes.ram[ADDR_P1_START_HEIGHT] = 3; /* Height 3: 80 garbage cells / 8 rows */
    nes.ram[ADDR_P2_START_HEIGHT] = 1; /* Height 1: 30 garbage cells / 3 rows */
    nes.ram[ADDR_RNG_SEED] = 0x35;
    nes.ram[ADDR_RNG_SEED + 1] = 0x8A;

    initPlayfieldIfTypeB(&nes);

    /* Height 3: rows 0..11 (120 cells) must be TILE_EMPTY */
    for (int i = 0; i < 120; i++) {
        assert(nes.ram[ADDR_PLAYFIELD + i] == TILE_EMPTY);
    }

    /* Rows 12..19 must have garbage blocks, each row having at least one TILE_EMPTY */
    for (int r = 12; r < 20; r++) {
        bool has_empty = false;
        bool has_block = false;
        for (int c = 0; c < 10; c++) {
            uint8_t t = nes.ram[ADDR_PLAYFIELD + r * 10 + c];
            if (t == TILE_EMPTY) has_empty = true;
            else has_block = true;
        }
        assert(has_empty);
        assert(has_block);
        (void)has_empty;
        (void)has_block;
    }

    /* Player 2 with Height 1: rows 0..16 (170 cells) must be TILE_EMPTY */
    for (int i = 0; i < 170; i++) {
        assert(nes.ram[ADDR_PLAYFIELD_P2 + i] == TILE_EMPTY);
    }
}

int test_playfield_subsystem(void) {
    test_playfield_lock_valid();
    test_playfield_lock_invalid_game_over();
    test_playfield_checkForCompletedRows_single();
    test_playfield_checkForCompletedRows_tetris();
    test_playfield_checkForCompletedRows_zero_lines();
    test_playfield_line_clear_anim();
    test_playfield_game_over_curtain();
    test_playfield_receive_garbage();
    test_playfield_type_b_init();

    printf("  [PASS] test_playfield_subsystem\n");
    return 0;
}
