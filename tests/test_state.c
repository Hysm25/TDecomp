#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "nes.h"
#include "game/state.h"
#include "game/tetrimino.h"
#include "game/playfield.h"
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

int test_state_subsystem(void) {
    printf("[RUN] Running Gameplay State Machine tests...\n");
    test_player_context_switching();
    test_playstate_basic_handlers();
    test_playstate_b_type_goal_check();
    test_branch_on_playstate_dispatch();
    printf("[PASS] All Gameplay State Machine unit tests passed successfully!\n");
    return 0;
}
