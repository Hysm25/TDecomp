#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "nes.h"
#include "game/input.h"
#include "constants/gameplay.h"
#include "constants/memory.h"

static void test_controller_serial_read(void) {
    NES_State nes;
    nes_init(&nes);

    /* Test single buttons for Player 1 */
    uint8_t buttons[] = {
        BUTTON_A, BUTTON_B, BUTTON_SELECT, BUTTON_START,
        BUTTON_UP, BUTTON_DOWN, BUTTON_LEFT, BUTTON_RIGHT
    };
    for (size_t i = 0; i < sizeof(buttons)/sizeof(buttons[0]); i++) {
        uint8_t btn = buttons[i];
        nes_set_controller(&nes, 0, btn);
        nes_set_controller(&nes, 1, 0);

        pollController_actualRead(&nes);

        assert(nes.ram[ADDR_NEWLY_PRESSED_P1] == btn);
        assert(nes.ram[ADDR_NEWLY_PRESSED_P2] == 0);
    }

    /* Test single buttons for Player 2 */
    for (size_t i = 0; i < sizeof(buttons)/sizeof(buttons[0]); i++) {
        uint8_t btn = buttons[i];
        nes_set_controller(&nes, 0, 0);
        nes_set_controller(&nes, 1, btn);

        pollController_actualRead(&nes);

        assert(nes.ram[ADDR_NEWLY_PRESSED_P1] == 0);
        assert(nes.ram[ADDR_NEWLY_PRESSED_P2] == btn);
    }

    /* Test simultaneous multi-button combinations */
    uint8_t p1_combo = BUTTON_A | BUTTON_B | BUTTON_START;
    uint8_t p2_combo = BUTTON_LEFT | BUTTON_DOWN;
    nes_set_controller(&nes, 0, p1_combo);
    nes_set_controller(&nes, 1, p2_combo);

    pollController_actualRead(&nes);

    assert(nes.ram[ADDR_NEWLY_PRESSED_P1] == p1_combo);
    assert(nes.ram[ADDR_NEWLY_PRESSED_P2] == p2_combo);

    /* Test all 8 buttons pressed simultaneously */
    uint8_t all_buttons = 0xFF;
    nes_set_controller(&nes, 0, all_buttons);
    nes_set_controller(&nes, 1, all_buttons);

    pollController_actualRead(&nes);

    assert(nes.ram[ADDR_NEWLY_PRESSED_P1] == all_buttons);
    assert(nes.ram[ADDR_NEWLY_PRESSED_P2] == all_buttons);

    printf("  [PASS] test_controller_serial_read\n");
}

static void test_expansion_port_mixing(void) {
    NES_State nes;
    nes_init(&nes);

    nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;
    nes.ram[ADDR_TMP1] = BUTTON_A;
    nes.ram[ADDR_NEWLY_PRESSED_P2] = BUTTON_LEFT;
    nes.ram[ADDR_TMP2] = BUTTON_RIGHT;

    addExpansionPortInputAsControllerInput(&nes);

    assert(nes.ram[ADDR_NEWLY_PRESSED_P1] == (BUTTON_START | BUTTON_A));
    assert(nes.ram[ADDR_NEWLY_PRESSED_P2] == (BUTTON_LEFT | BUTTON_RIGHT));

    printf("  [PASS] test_expansion_port_mixing\n");
}

static void test_diff_old_and_new_buttons(void) {
    NES_State nes;
    nes_init(&nes);

    /* Frame 1: Player 1 presses A */
    nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_A;
    nes.ram[ADDR_HELD_BUTTONS_P1] = 0;
    diffOldAndNewButtons(&nes);
    assert(nes.ram[ADDR_NEWLY_PRESSED_P1] == BUTTON_A);
    assert(nes.ram[ADDR_HELD_BUTTONS_P1] == BUTTON_A);

    /* Frame 2: Player 1 continues holding A */
    nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_A;
    diffOldAndNewButtons(&nes);
    assert(nes.ram[ADDR_NEWLY_PRESSED_P1] == 0);
    assert(nes.ram[ADDR_HELD_BUTTONS_P1] == BUTTON_A);

    /* Frame 3: Player 1 releases A */
    nes.ram[ADDR_NEWLY_PRESSED_P1] = 0;
    diffOldAndNewButtons(&nes);
    assert(nes.ram[ADDR_NEWLY_PRESSED_P1] == 0);
    assert(nes.ram[ADDR_HELD_BUTTONS_P1] == 0);

    /* Frame 4: Player 1 presses A and B together */
    nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_A | BUTTON_B;
    diffOldAndNewButtons(&nes);
    assert(nes.ram[ADDR_NEWLY_PRESSED_P1] == (BUTTON_A | BUTTON_B));
    assert(nes.ram[ADDR_HELD_BUTTONS_P1] == (BUTTON_A | BUTTON_B));

    /* Frame 5: Player 1 releases B while still holding A */
    nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_A;
    diffOldAndNewButtons(&nes);
    assert(nes.ram[ADDR_NEWLY_PRESSED_P1] == 0);
    assert(nes.ram[ADDR_HELD_BUTTONS_P1] == BUTTON_A);

    /* Frame 6: Player 1 presses B again while still holding A */
    nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_A | BUTTON_B;
    diffOldAndNewButtons(&nes);
    assert(nes.ram[ADDR_NEWLY_PRESSED_P1] == BUTTON_B); /* Only B is newly pressed */
    assert(nes.ram[ADDR_HELD_BUTTONS_P1] == (BUTTON_A | BUTTON_B));

    /* Test Player 2 independently */
    nes.ram[ADDR_NEWLY_PRESSED_P2] = BUTTON_START;
    nes.ram[ADDR_HELD_BUTTONS_P2] = 0;
    diffOldAndNewButtons(&nes);
    assert(nes.ram[ADDR_NEWLY_PRESSED_P2] == BUTTON_START);
    assert(nes.ram[ADDR_HELD_BUTTONS_P2] == BUTTON_START);

    printf("  [PASS] test_diff_old_and_new_buttons\n");
}

static void test_poll_controller_debouncing(void) {
    NES_State nes;
    nes_init(&nes);

    /* Clean reads */
    nes_set_controller(&nes, 0, BUTTON_A | BUTTON_RIGHT);
    nes_set_controller(&nes, 1, BUTTON_B);

    pollController(&nes);

    assert(nes.ram[ADDR_NEWLY_PRESSED_P1] == (BUTTON_A | BUTTON_RIGHT));
    assert(nes.ram[ADDR_HELD_BUTTONS_P1] == (BUTTON_A | BUTTON_RIGHT));
    assert(nes.ram[ADDR_NEWLY_PRESSED_P2] == BUTTON_B);
    assert(nes.ram[ADDR_HELD_BUTTONS_P2] == BUTTON_B);
    assert(nes.ram[ADDR_GENERAL_COUNTER2] == (BUTTON_A | BUTTON_RIGHT));
    assert(nes.ram[ADDR_GENERAL_COUNTER3] == BUTTON_B);

    printf("  [PASS] test_poll_controller_debouncing\n");
}

static void test_poll_controller_buttons_mode_dispatch(void) {
    NES_State nes;
    nes_init(&nes);

    /* Standard game mode (e.g. MODE_PLAYING = 4) */
    nes.ram[ADDR_GAME_MODE] = 4;
    nes_set_controller(&nes, 0, BUTTON_LEFT);
    pollControllerButtons(&nes);
    assert(nes.ram[ADDR_NEWLY_PRESSED_P1] == BUTTON_LEFT);
    assert(nes.ram[ADDR_HELD_BUTTONS_P1] == BUTTON_LEFT);

    /* Demo mode ($05) - Start button pressed cancels demo */
    nes_init(&nes);
    nes.ram[ADDR_GAME_MODE] = MODE_DEMO;
    nes_set_controller(&nes, 0, BUTTON_START);
    pollControllerButtons(&nes);
    assert(nes.ram[ADDR_GAME_MODE] == MODE_TITLE_SCREEN);
    assert(nes.ram[ADDR_DEMO_BUTTONS_ADDR + 1] == DEMO_BUTTONS_TABLE_ADDR_HI);
    assert(nes.ram[ADDR_FRAME_COUNTER + 1] == 0);

    /* Demo mode ($05) - Move in progress (repeats > 0) */
    nes_init(&nes);
    nes.ram[ADDR_GAME_MODE] = MODE_DEMO;
    nes.ram[ADDR_DEMO_HELD_BUTTONS] = BUTTON_DOWN;
    nes.ram[ADDR_DEMO_REPEATS] = 5;
    pollControllerButtons(&nes);
    assert(nes.ram[ADDR_DEMO_REPEATS] == 4);
    assert(nes.ram[ADDR_NEWLY_PRESSED_P1] == 0);
    assert(nes.ram[ADDR_HELD_BUTTONS_P1] == BUTTON_DOWN);

    /* Demo mode ($05) - Move finished (repeats == 0): fetches next move from demo table */
    nes_init(&nes);
    nes.ram[ADDR_GAME_MODE] = MODE_DEMO;
    nes.ram[ADDR_DEMO_HELD_BUTTONS] = 0;
    nes.ram[ADDR_DEMO_REPEATS] = 0;
    /* Put mock demo data in RAM: button byte = BUTTON_RIGHT, repeat count = 3 */
    uint16_t demo_addr = 0x0300;
    nes.ram[ADDR_DEMO_BUTTONS_ADDR] = (uint8_t)(demo_addr & 0xFF);
    nes.ram[ADDR_DEMO_BUTTONS_ADDR + 1] = (uint8_t)((demo_addr >> 8) & 0xFF);
    nes.ram[0x0300] = BUTTON_RIGHT;
    nes.ram[0x0301] = 3;

    pollControllerButtons(&nes);

    assert(nes.ram[ADDR_NEWLY_PRESSED_P1] == BUTTON_RIGHT);
    assert(nes.ram[ADDR_DEMO_HELD_BUTTONS] == BUTTON_RIGHT);
    assert(nes.ram[ADDR_HELD_BUTTONS_P1] == BUTTON_RIGHT);
    assert(nes.ram[ADDR_DEMO_REPEATS] == 3);
    assert(nes.ram[ADDR_GENERAL_COUNTER] == BUTTON_RIGHT);
    /* demoButtonsAddr was incremented twice */
    uint16_t new_addr = (uint16_t)(nes.ram[ADDR_DEMO_BUTTONS_ADDR] |
                                  ((uint16_t)nes.ram[ADDR_DEMO_BUTTONS_ADDR + 1] << 8));
    assert(new_addr == 0x0302);
    (void)new_addr;

    printf("  [PASS] test_poll_controller_buttons_mode_dispatch\n");
}

static void test_demo_table_index_incr(void) {
    NES_State nes;
    nes_init(&nes);

    /* Standard increment */
    nes.ram[ADDR_DEMO_BUTTONS_ADDR] = 0x50;
    nes.ram[ADDR_DEMO_BUTTONS_ADDR + 1] = 0xDD;
    demoButtonsTable_indexIncr(&nes);
    assert(nes.ram[ADDR_DEMO_BUTTONS_ADDR] == 0x51);
    assert(nes.ram[ADDR_DEMO_BUTTONS_ADDR + 1] == 0xDD);

    /* 8-bit overflow into high byte */
    nes.ram[ADDR_DEMO_BUTTONS_ADDR] = 0xFF;
    nes.ram[ADDR_DEMO_BUTTONS_ADDR + 1] = 0xDD;
    demoButtonsTable_indexIncr(&nes);
    assert(nes.ram[ADDR_DEMO_BUTTONS_ADDR] == 0x00);
    assert(nes.ram[ADDR_DEMO_BUTTONS_ADDR + 1] == 0xDE);

    printf("  [PASS] test_demo_table_index_incr\n");
}

static void test_update_active_player_input(void) {
    NES_State nes;
    nes_init(&nes);

    nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_A;
    nes.ram[ADDR_HELD_BUTTONS_P1] = BUTTON_A;
    nes.ram[ADDR_NEWLY_PRESSED_P2] = BUTTON_B;
    nes.ram[ADDR_HELD_BUTTONS_P2] = BUTTON_B;

    /* Player 1 active */
    nes.ram[ADDR_ACTIVE_PLAYER] = 1;
    updateActivePlayerInput(&nes);
    assert(nes.ram[ADDR_NEWLY_PRESSED_BUTTONS] == BUTTON_A);
    assert(nes.ram[ADDR_HELD_BUTTONS] == BUTTON_A);

    /* Player 2 active */
    nes.ram[ADDR_ACTIVE_PLAYER] = 2;
    updateActivePlayerInput(&nes);
    assert(nes.ram[ADDR_NEWLY_PRESSED_BUTTONS] == BUTTON_B);
    assert(nes.ram[ADDR_HELD_BUTTONS] == BUTTON_B);

    printf("  [PASS] test_update_active_player_input\n");
}

static void test_das_timing_and_wall_charge(void) {
    uint8_t autorepeatX = 0;

    /* Down button inhibits horizontal DAS */
    bool shift = processDAS(&autorepeatX, BUTTON_LEFT, BUTTON_DOWN | BUTTON_LEFT);
    assert(!shift);
    (void)shift;

    /* Neither Left nor Right held */
    shift = processDAS(&autorepeatX, 0, BUTTON_A);
    assert(!shift);

    /* Initial Left press: resets autorepeatX to 0 and triggers immediate shift */
    autorepeatX = 5;
    shift = processDAS(&autorepeatX, BUTTON_LEFT, BUTTON_LEFT);
    assert(shift);
    assert(autorepeatX == 0);

    /* Verify exact NTSC timing over 30 consecutive frames */
    /* Frame 0 was initial press (autorepeatX == 0) */
    /* Frames 1 through 15: holding Left, autorepeatX increments from 1 to 15, no shift */
    for (int frame = 1; frame <= 15; frame++) {
        shift = processDAS(&autorepeatX, 0, BUTTON_LEFT);
        assert(!shift);
        assert(autorepeatX == frame);
    }

    /* Frame 16: autorepeatX reaches DAS_RESET (16), resets to DAS_DELAY (10), triggers shift! */
    shift = processDAS(&autorepeatX, 0, BUTTON_LEFT);
    assert(shift);
    assert(autorepeatX == DAS_DELAY_NTSC); /* 10 */

    /* Frames 17 through 21 (5 frames): autorepeatX increments 11 to 15, no shift */
    for (int frame = 11; frame <= 15; frame++) {
        shift = processDAS(&autorepeatX, 0, BUTTON_LEFT);
        assert(!shift);
        assert(autorepeatX == frame);
    }

    /* Frame 22 (6 frames after frame 16): reaches 16, resets to 10, triggers shift! */
    shift = processDAS(&autorepeatX, 0, BUTTON_LEFT);
    assert(shift);
    assert(autorepeatX == DAS_DELAY_NTSC);

    /* Frame 28 (6 frames after frame 22): reaches 16, resets to 10, triggers shift! */
    for (int i = 0; i < 5; i++) {
        shift = processDAS(&autorepeatX, 0, BUTTON_LEFT);
        assert(!shift);
    }
    shift = processDAS(&autorepeatX, 0, BUTTON_LEFT);
    assert(shift);
    assert(autorepeatX == DAS_DELAY_NTSC);

    /* Test wall charge: tetrimino hits wall and charges DAS to DAS_RESET (16) */
    autorepeatX = 2;
    dasWallCharge(&autorepeatX);
    assert(autorepeatX == DAS_RESET_NTSC); /* 16 */

    /* On next frame of holding Left against wall, immediately triggers repeat! */
    shift = processDAS(&autorepeatX, 0, BUTTON_LEFT);
    assert(shift);
    assert(autorepeatX == DAS_DELAY_NTSC);
    (void)shift;

    printf("  [PASS] test_das_timing_and_wall_charge\n");
}

int test_input_subsystem(void) {
    printf("[RUN] Running Input and DAS Subsystem tests...\n");
    test_controller_serial_read();
    test_expansion_port_mixing();
    test_diff_old_and_new_buttons();
    test_poll_controller_debouncing();
    test_poll_controller_buttons_mode_dispatch();
    test_demo_table_index_incr();
    test_update_active_player_input();
    test_das_timing_and_wall_charge();
    printf("[PASS] All Input Subsystem unit tests passed successfully!\n");
    return 0;
}
