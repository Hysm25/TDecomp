#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "nes.h"
#include "game/state.h"
#include "game/tetrimino.h"
#include "game/playfield.h"
#include "math/rng.h"
#include "constants/gameplay.h"
#include "constants/memory.h"

static void test_player_context_switching(void) {
    NES_State nes;
    nes_init(&nes);

    /* Setup distinct state in Player 1 mirror ($0060 - $007F) */
    for (int i = 0; i < 32; i++) {
        nes.ram[ADDR_P1_BASE + i] = (uint8_t)(0x10 + i);
    }
    nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_A;
    nes.ram[ADDR_HELD_BUTTONS_P1] = BUTTON_A | BUTTON_START;
    nes.ram[ADDR_NUMBER_OF_PLAYERS] = 1;

    makePlayer1Active(&nes);

    assert(nes.ram[ADDR_ACTIVE_PLAYER] == 1);
    assert(nes.ram[ADDR_PLAYFIELD_ADDR + 1] == 0x04);
    assert(nes.ram[ADDR_NEWLY_PRESSED_BUTTONS] == BUTTON_A);
    assert(nes.ram[ADDR_HELD_BUTTONS] == (BUTTON_A | BUTTON_START));
    for (int i = 0; i < 32; i++) {
        assert(nes.ram[ADDR_TETRIMINO_X + i] == (uint8_t)(0x10 + i));
    }

    /* Modify active state and save in 1-player mode */
    nes.ram[ADDR_TETRIMINO_X] = 0x99;
    nes.ram[ADDR_PENDING_GARBAGE] = 2;
    nes.ram[ADDR_PENDING_GARBAGE_INACTIVE] = 5;

    savePlayer1State(&nes);

    assert(nes.ram[ADDR_P1_BASE] == 0x99);
    /* In 1-player mode, garbage is NOT swapped */
    assert(nes.ram[ADDR_PENDING_GARBAGE] == 2);
    assert(nes.ram[ADDR_PENDING_GARBAGE_INACTIVE] == 5);

    /* Test 2-player mode garbage swapping */
    nes.ram[ADDR_NUMBER_OF_PLAYERS] = 2;
    savePlayer1State(&nes);
    assert(nes.ram[ADDR_PENDING_GARBAGE] == 5);
    assert(nes.ram[ADDR_PENDING_GARBAGE_INACTIVE] == 2);

    /* Setup Player 2 mirror ($0080 - $009F) */
    for (int i = 0; i < 32; i++) {
        nes.ram[ADDR_P2_BASE + i] = (uint8_t)(0x50 + i);
    }
    nes.ram[ADDR_NEWLY_PRESSED_P2] = BUTTON_B;
    nes.ram[ADDR_HELD_BUTTONS_P2] = BUTTON_B | BUTTON_SELECT;

    makePlayer2Active(&nes);

    assert(nes.ram[ADDR_ACTIVE_PLAYER] == 2);
    assert(nes.ram[ADDR_PLAYFIELD_ADDR + 1] == 0x05);
    assert(nes.ram[ADDR_NEWLY_PRESSED_BUTTONS] == BUTTON_B);
    assert(nes.ram[ADDR_HELD_BUTTONS] == (BUTTON_B | BUTTON_SELECT));
    for (int i = 0; i < 32; i++) {
        assert(nes.ram[ADDR_TETRIMINO_X + i] == (uint8_t)(0x50 + i));
    }

    /* Save Player 2 state and verify garbage swap */
    nes.ram[ADDR_TETRIMINO_X] = 0xAA;
    savePlayer2State(&nes);
    assert(nes.ram[ADDR_P2_BASE] == 0xAA);
    assert(nes.ram[ADDR_PENDING_GARBAGE] == 2);
    assert(nes.ram[ADDR_PENDING_GARBAGE_INACTIVE] == 5);

    printf("  [PASS] test_player_context_switching\n");
}

static void test_playstate_basic_handlers(void) {
    NES_State nes;
    nes_init(&nes);

    /* State 0: playState_unassignOrientationId */
    nes.ram[ADDR_CURRENT_PIECE] = 5;
    playState_unassignOrientationId(&nes);
    assert(nes.ram[ADDR_CURRENT_PIECE] == TETRIMINO_HIDDEN);

    /* State 11: playState_incrementPlayState */
    nes.ram[ADDR_PLAY_STATE] = 11;
    playState_incrementPlayState(&nes);
    assert(nes.ram[ADDR_PLAY_STATE] == 12);

    /* State 4/9: playState_noop */
    nes.ram[ADDR_PLAY_STATE] = 4;
    playState_noop(&nes);
    assert(nes.ram[ADDR_PLAY_STATE] == 4);

    printf("  [PASS] test_playstate_basic_handlers\n");
}

static void test_playstate_b_type_goal_check(void) {
    NES_State nes;
    nes_init(&nes);

    /* Case 1: Type A mode -> increments playState */
    nes.ram[ADDR_GAME_TYPE] = GAMETYPE_A;
    nes.ram[ADDR_PLAY_STATE] = PLAYSTATE_B_TYPE_GOAL_CHECK;
    playState_bTypeGoalCheck(&nes);
    assert(nes.ram[ADDR_PLAY_STATE] == PLAYSTATE_RECEIVE_GARBAGE); /* 6 -> 7 */

    /* Case 2: Type B mode with lines remaining -> increments playState */
    nes.ram[ADDR_GAME_TYPE] = GAMETYPE_B;
    nes.ram[ADDR_LINES] = 10;
    nes.ram[ADDR_PLAY_STATE] = PLAYSTATE_B_TYPE_GOAL_CHECK;
    playState_bTypeGoalCheck(&nes);
    assert(nes.ram[ADDR_PLAY_STATE] == PLAYSTATE_RECEIVE_GARBAGE); /* 6 -> 7 */

    /* Case 3: Type B mode with goal completed (lines == 0) */
    nes.ram[ADDR_GAME_TYPE] = GAMETYPE_B;
    nes.ram[ADDR_LINES] = 0;
    nes.ram[ADDR_PLAY_STATE] = PLAYSTATE_B_TYPE_GOAL_CHECK;
    nes.ram[ADDR_GAME_MODE_STATE] = 2;
    nes.ram[ADDR_PLAYFIELD_ADDR] = LOW_BYTE(ADDR_PLAYFIELD);
    nes.ram[ADDR_PLAYFIELD_ADDR + 1] = HIGH_BYTE(ADDR_PLAYFIELD);

    playState_bTypeGoalCheck(&nes);

    assert(nes.ram[ADDR_PLAY_STATE] == 0);
    assert(nes.ram[ADDR_GAME_MODE_STATE] == 3);
    assert(nes.ram[ADDR_P1_VRAM_ROW] == 0);
    assert(nes.ram[ADDR_RENDER_MODE] == 0);
    /* Verify victory tiles placed into playfield */
    assert(nes.ram[ADDR_PLAYFIELD + 0x46] == 0x38);
    assert(nes.ram[ADDR_PLAYFIELD + 0x47] == 0x39);

    printf("  [PASS] test_playstate_b_type_goal_check\n");
}

static void test_branch_on_playstate_dispatch(void) {
    NES_State nes;
    nes_init(&nes);

    /* State 0: Unassign orientation */
    nes.ram[ADDR_PLAY_STATE] = PLAYSTATE_UNASSIGN_ORIENTATION;
    nes.ram[ADDR_CURRENT_PIECE] = 3;
    branchOnPlayStatePlayer1(&nes);
    assert(nes.ram[ADDR_CURRENT_PIECE] == TETRIMINO_HIDDEN);

    /* State 4: No-op */
    nes.ram[ADDR_PLAY_STATE] = PLAYSTATE_LINE_CLEAR_ANIM;
    branchOnPlayStatePlayer1(&nes);
    assert(nes.ram[ADDR_PLAY_STATE] == PLAYSTATE_LINE_CLEAR_ANIM);

    /* State 6: Type B goal check (Type A advances to 7) */
    nes.ram[ADDR_GAME_TYPE] = GAMETYPE_A;
    nes.ram[ADDR_PLAY_STATE] = PLAYSTATE_B_TYPE_GOAL_CHECK;
    branchOnPlayStatePlayer1(&nes);
    assert(nes.ram[ADDR_PLAY_STATE] == PLAYSTATE_RECEIVE_GARBAGE);

    /* State 9: No-op */
    nes.ram[ADDR_PLAY_STATE] = PLAYSTATE_NOOP_9;
    branchOnPlayStatePlayer1(&nes);
    assert(nes.ram[ADDR_PLAY_STATE] == PLAYSTATE_NOOP_9);

    /* State 11: Increment playState */
    nes.ram[ADDR_PLAY_STATE] = PLAYSTATE_INCREMENT_PLAY_STATE;
    branchOnPlayStatePlayer1(&nes);
    assert(nes.ram[ADDR_PLAY_STATE] == 12);

    /* Invalid / unexpected state handling */
    nes.ram[ADDR_PLAY_STATE] = 99;
    branchOnPlayStatePlayer1(&nes);
    assert(nes.ram[ADDR_PLAY_STATE] == 99);

    /* Test unified branchOnPlayState */
    nes.ram[ADDR_ACTIVE_PLAYER] = 1;
    nes.ram[ADDR_PLAY_STATE] = PLAYSTATE_UNASSIGN_ORIENTATION;
    nes.ram[ADDR_CURRENT_PIECE] = 2;
    branchOnPlayState(&nes);
    assert(nes.ram[ADDR_CURRENT_PIECE] == TETRIMINO_HIDDEN);

    nes.ram[ADDR_ACTIVE_PLAYER] = 2;
    nes.ram[ADDR_PLAY_STATE] = PLAYSTATE_UNASSIGN_ORIENTATION;
    nes.ram[ADDR_CURRENT_PIECE] = 7;
    branchOnPlayState(&nes);
    assert(nes.ram[ADDR_CURRENT_PIECE] == TETRIMINO_HIDDEN);

    printf("  [PASS] test_branch_on_playstate_dispatch\n");
}

static void test_playstate_spawn_next_tetrimino(void) {
    NES_State nes;
    nes_init(&nes);

    /* Test 1: VRAM wait guard prevents spawn if vramRow < 0x20 */
    nes.ram[ADDR_VRAM_ROW] = 0x05;
    nes.ram[ADDR_PLAY_STATE] = PLAYSTATE_SPAWN_NEXT;
    playState_spawnNextTetrimino(&nes);
    assert(nes.ram[ADDR_PLAY_STATE] == PLAYSTATE_SPAWN_NEXT);

    nes.ram[ADDR_VRAM_ROW] = 0x1F;
    playState_spawnNextTetrimino(&nes);
    assert(nes.ram[ADDR_PLAY_STATE] == PLAYSTATE_SPAWN_NEXT);

    /* Test 2: 1-Player mode successful spawn */
    nes.ram[ADDR_VRAM_ROW] = VRAM_ROW_COMPLETE;
    nes.ram[ADDR_NUMBER_OF_PLAYERS] = 1;
    nes.ram[ADDR_NEXT_PIECE] = ORIENT_T_DOWN;
    nes.ram[ADDR_PLAY_STATE] = PLAYSTATE_SPAWN_NEXT;
    nes.ram[ADDR_AUTOREPEAT_Y] = 0x50;

    playState_spawnNextTetrimino(&nes);

    assert(nes.ram[ADDR_TETRIMINO_X] == TETRIMINO_SPAWN_X);
    assert(nes.ram[ADDR_TETRIMINO_Y] == TETRIMINO_SPAWN_Y);
    assert(nes.ram[ADDR_FALL_TIMER] == 0);
    assert(nes.ram[ADDR_PLAY_STATE] == PLAYSTATE_CONTROL_ACTIVE);
    assert(nes.ram[ADDR_CURRENT_PIECE] == ORIENT_T_DOWN);
    assert(nes.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER] == 0);
    assert(nes.ram[ADDR_AUTOREPEAT_Y] == 0);
    assert((nes.ram[ADDR_OUT_OF_DATE_RENDER_FLAGS] & RENDER_STATS) != 0);
    assert(nes.ram[ADDR_STATS_BY_TYPE + PIECE_T * 2] == 0x01);

    /* Test 3: Orientation mapping for all 19 orientations */
    for (uint8_t orient = 0; orient < ORIENTATION_COUNT; orient++) {
        nes_init(&nes);
        nes.ram[ADDR_VRAM_ROW] = VRAM_ROW_COMPLETE;
        nes.ram[ADDR_NUMBER_OF_PLAYERS] = 1;
        nes.ram[ADDR_NEXT_PIECE] = orient;
        nes.ram[ADDR_PLAY_STATE] = PLAYSTATE_SPAWN_NEXT;

        playState_spawnNextTetrimino(&nes);

        assert(nes.ram[ADDR_CURRENT_PIECE] == spawnOrientationFromOrientation[orient]);
        assert(nes.ram[ADDR_TETRIMINO_X] == TETRIMINO_SPAWN_X);
        assert(nes.ram[ADDR_TETRIMINO_Y] == TETRIMINO_SPAWN_Y);
        assert(nes.ram[ADDR_PLAY_STATE] == PLAYSTATE_CONTROL_ACTIVE);
    }

    /* Test 4: 2-Player mode delay mechanics */
    nes_init(&nes);
    nes.ram[ADDR_VRAM_ROW] = VRAM_ROW_COMPLETE;
    nes.ram[ADDR_NUMBER_OF_PLAYERS] = 2;
    nes.ram[ADDR_ACTIVE_PLAYER] = 1;
    nes.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER] = 0;
    nes.ram[ADDR_PLAY_STATE] = PLAYSTATE_SPAWN_NEXT;

    /* First frame (delayCounter == 0): triggers delay */
    playState_spawnNextTetrimino(&nes);
    assert(nes.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER] == 1);
    assert(nes.ram[ADDR_TWO_PLAYER_PIECE_DELAY_PLAYER] == 1);
    assert(nes.ram[ADDR_PLAY_STATE] == PLAYSTATE_SPAWN_NEXT);

    /* Intermediate frame (delayCounter == 10): still waiting */
    nes.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER] = 10;
    playState_spawnNextTetrimino(&nes);
    assert(nes.ram[ADDR_PLAY_STATE] == PLAYSTATE_SPAWN_NEXT);

    /* Inactive player frame (delayPlayer != activePlayer): still waiting */
    nes.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER] = TWO_PLAYER_PIECE_DELAY_FRAMES;
    nes.ram[ADDR_ACTIVE_PLAYER] = 2; /* Player 2 active while Player 1 is delaying */
    playState_spawnNextTetrimino(&nes);
    assert(nes.ram[ADDR_PLAY_STATE] == PLAYSTATE_SPAWN_NEXT);

    /* Delay finished (delayCounter == $1C and delayPlayer == activePlayer): spawns */
    nes.ram[ADDR_ACTIVE_PLAYER] = 1;
    nes.ram[ADDR_NEXT_PIECE] = ORIENT_Z_HORIZ;
    nes.ram[ADDR_TWO_PLAYER_PIECE_DELAY_PIECE] = ORIENT_L_DOWN;

    playState_spawnNextTetrimino(&nes);

    assert(nes.ram[ADDR_PLAY_STATE] == PLAYSTATE_CONTROL_ACTIVE);
    assert(nes.ram[ADDR_TETRIMINO_X] == TETRIMINO_SPAWN_X);
    assert(nes.ram[ADDR_TETRIMINO_Y] == TETRIMINO_SPAWN_Y);
    assert(nes.ram[ADDR_CURRENT_PIECE] == ORIENT_Z_HORIZ);
    assert(nes.ram[ADDR_NEXT_PIECE] == ORIENT_L_DOWN);
    assert(nes.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER] == 0);

    /* Test 5: Dispatcher integration via branchOnPlayState */
    nes_init(&nes);
    nes.ram[ADDR_VRAM_ROW] = VRAM_ROW_COMPLETE;
    nes.ram[ADDR_NUMBER_OF_PLAYERS] = 1;
    nes.ram[ADDR_ACTIVE_PLAYER] = 1;
    nes.ram[ADDR_NEXT_PIECE] = ORIENT_O_FIXED;
    nes.ram[ADDR_PLAY_STATE] = PLAYSTATE_SPAWN_NEXT;

    branchOnPlayState(&nes);

    assert(nes.ram[ADDR_PLAY_STATE] == PLAYSTATE_CONTROL_ACTIVE);
    assert(nes.ram[ADDR_CURRENT_PIECE] == ORIENT_O_FIXED);

    printf("  [PASS] test_playstate_spawn_next_tetrimino\n");
}

static void test_gamemode_framing(void) {
    NES_State nes;
    nes_init(&nes);

    /* Test 1: noop_disabledVramRowIncr does not corrupt state */
    nes.ram[ADDR_GAME_MODE_STATE] = 5;
    noop_disabledVramRowIncr(&nes);
    assert(nes.ram[ADDR_GAME_MODE_STATE] == 5);

    /* Test 2: gameModeState_vblankThenRunState2 sets gameModeState = 2 */
    gameModeState_vblankThenRunState2(&nes);
    assert(nes.ram[ADDR_GAME_MODE_STATE] == 2);

    /* Test 3: gameModeState_updateCountersAndNonPlayerState framing step */
    nes.ram[ADDR_OAM_STAGING_LENGTH] = 40;
    nes.ram[ADDR_P1_FALL_TIMER] = 12;
    nes.ram[ADDR_P2_FALL_TIMER] = 30;
    nes.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER] = 0;
    nes.ram[ADDR_DISPLAY_NEXT_PIECE] = 0;
    nes.ram[ADDR_NEWLY_PRESSED_P1] = 0;
    nes.ram[ADDR_GAME_MODE_STATE] = 2;

    gameModeState_updateCountersAndNonPlayerState(&nes);

    assert(nes.ram[ADDR_OAM_STAGING_LENGTH] == 0);
    assert(nes.ram[ADDR_P1_FALL_TIMER] == 13);
    assert(nes.ram[ADDR_P2_FALL_TIMER] == 31);
    assert(nes.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER] == 0);
    assert(nes.ram[ADDR_DISPLAY_NEXT_PIECE] == 0);
    assert(nes.ram[ADDR_GAME_MODE_STATE] == 3);

    /* Step with active 2P delay counter and SELECT pressed */
    nes.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER] = 15;
    nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_SELECT;

    gameModeState_updateCountersAndNonPlayerState(&nes);

    assert(nes.ram[ADDR_P1_FALL_TIMER] == 14);
    assert(nes.ram[ADDR_P2_FALL_TIMER] == 32);
    assert(nes.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER] == 16);
    assert(nes.ram[ADDR_DISPLAY_NEXT_PIECE] == 1);
    assert(nes.ram[ADDR_GAME_MODE_STATE] == 4);

    /* Press SELECT again: toggles displayNextPiece back to 0 */
    gameModeState_updateCountersAndNonPlayerState(&nes);
    assert(nes.ram[ADDR_DISPLAY_NEXT_PIECE] == 0);
    assert(nes.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER] == 17);
    assert(nes.ram[ADDR_GAME_MODE_STATE] == 5);

    printf("  [PASS] test_gamemode_framing\n");
}

int test_state_subsystem(void) {
    printf("[RUN] Running Gameplay State Machine tests...\n");
    test_player_context_switching();
    test_playstate_basic_handlers();
    test_playstate_b_type_goal_check();
    test_branch_on_playstate_dispatch();
    test_playstate_spawn_next_tetrimino();
    test_gamemode_framing();
    printf("[PASS] All Gameplay State Machine unit tests passed successfully!\n");
    return 0;
}
