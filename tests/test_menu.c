#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "nes.h"
#include "game/menu.h"
#include "constants/gameplay.h"
#include "constants/memory.h"

static void test_legal_screen(void) {
    NES_State nes;
    nes_init(&nes);

    /* Test 1: Full gameMode_legalScreen transition */
    nes.ram[ADDR_GAME_MODE] = MODE_LEGAL_SCREEN;
    nes.ram[ADDR_RENDER_MODE] = 3;
    nes.ram[ADDR_OAM_STAGING] = 0x55;

    gameMode_legalScreen(&nes);

    assert(nes.ram[ADDR_GAME_MODE] == MODE_TITLE_SCREEN);
    assert(nes.ram[ADDR_RENDER_MODE] == 0);
    assert(nes.ram[ADDR_OAM_STAGING] == 0);
    assert(nes.ram[ADDR_SLEEP_COUNTER] == 0);
    assert(nes.ram[ADDR_GENERAL_COUNTER] == 0);

    /* Test 2: Frame-by-frame step - START button skips remaining wait */
    nes_init(&nes);
    nes.ram[ADDR_GAME_MODE] = MODE_LEGAL_SCREEN;
    nes.ram[ADDR_SLEEP_COUNTER] = 50;
    nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;

    gameMode_legalScreen_step(&nes);

    assert(nes.ram[ADDR_GAME_MODE] == MODE_TITLE_SCREEN);
    assert(nes.ram[ADDR_RENDER_MODE] == 0);

    /* Test 3: Frame-by-frame step - sleep countdown */
    nes_init(&nes);
    nes.ram[ADDR_GAME_MODE] = MODE_LEGAL_SCREEN;
    nes.ram[ADDR_SLEEP_COUNTER] = 2;
    nes.ram[ADDR_GENERAL_COUNTER] = 5;
    nes.ram[ADDR_NEWLY_PRESSED_P1] = 0;

    gameMode_legalScreen_step(&nes);
    assert(nes.ram[ADDR_SLEEP_COUNTER] == 1);
    assert(nes.ram[ADDR_GAME_MODE] == MODE_LEGAL_SCREEN);

    gameMode_legalScreen_step(&nes);
    assert(nes.ram[ADDR_SLEEP_COUNTER] == 0);
    assert(nes.ram[ADDR_GAME_MODE] == MODE_LEGAL_SCREEN);

    /* Countdown generalCounter */
    for (int i = 0; i < 5; i++) {
        gameMode_legalScreen_step(&nes);
    }
    assert(nes.ram[ADDR_GAME_MODE] == MODE_TITLE_SCREEN);

    printf("  [PASS] test_legal_screen\n");
}

static void test_title_screen(void) {
    NES_State nes;
    nes_init(&nes);

    /* Test 1: START button advances to Mode 2 with sound effect */
    nes.ram[ADDR_GAME_MODE] = MODE_TITLE_SCREEN;
    nes.ram[ADDR_RENDER_MODE] = 3;
    nes.ram[ADDR_DEMO_RECORD_FLAG] = 1;
    nes.ram[ADDR_DISPLAY_NEXT_PIECE] = 1;
    nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;

    gameMode_titleScreen(&nes);

    assert(nes.ram[ADDR_GAME_MODE] == MODE_GAME_TYPE_MENU);
    assert(nes.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == SFX_MENU_SELECT);
    assert(nes.ram[ADDR_RENDER_MODE] == 0);
    assert(nes.ram[ADDR_DEMO_RECORD_FLAG] == 0);
    assert(nes.ram[ADDR_DISPLAY_NEXT_PIECE] == 0);

    /* Test 2: Timeout advances to Mode 6 (startDemo) with sound effect */
    nes_init(&nes);
    nes.ram[ADDR_GAME_MODE] = MODE_TITLE_SCREEN;
    nes.ram[ADDR_NEWLY_PRESSED_P1] = 0;
    nes.ram[ADDR_FRAME_COUNTER + 1] = TITLE_SCREEN_TIMEOUT_HI; /* 5 */

    gameMode_titleScreen(&nes);

    assert(nes.ram[ADDR_GAME_MODE] == MODE_START_DEMO);
    assert(nes.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == SFX_MENU_SELECT);

    /* Test 3: Frame-by-frame step helper */
    nes_init(&nes);
    nes.ram[ADDR_GAME_MODE] = MODE_TITLE_SCREEN;
    nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;
    gameMode_titleScreen_step(&nes);
    assert(nes.ram[ADDR_GAME_MODE] == MODE_GAME_TYPE_MENU);

    nes_init(&nes);
    nes.ram[ADDR_GAME_MODE] = MODE_TITLE_SCREEN;
    nes.ram[ADDR_NEWLY_PRESSED_P1] = 0;
    nes.ram[ADDR_FRAME_COUNTER + 1] = TITLE_SCREEN_TIMEOUT_HI;
    gameMode_titleScreen_step(&nes);
    assert(nes.ram[ADDR_GAME_MODE] == MODE_START_DEMO);

    printf("  [PASS] test_title_screen\n");
}

static void test_start_demo(void) {
    NES_State nes;
    nes_init(&nes);

    nes.ram[ADDR_GAME_TYPE] = 1;
    nes.ram[ADDR_P1_START_LEVEL] = 5;
    nes.ram[ADDR_GAME_MODE_STATE] = 3;
    nes.ram[ADDR_P1_PLAY_STATE] = 2;
    nes.ram[ADDR_GAME_MODE] = MODE_START_DEMO;

    gameMode_startDemo(&nes);

    assert(nes.ram[ADDR_GAME_TYPE] == 0);
    assert(nes.ram[ADDR_P1_START_LEVEL] == 0);
    assert(nes.ram[ADDR_GAME_MODE_STATE] == 0);
    assert(nes.ram[ADDR_P1_PLAY_STATE] == 0);
    assert(nes.ram[ADDR_GAME_MODE] == MODE_DEMO);

    printf("  [PASS] test_start_demo\n");
}

static void test_branch_on_gamemode(void) {
    NES_State nes;
    nes_init(&nes);

    /* Mode 0: Legal Screen -> transitions to 1 */
    nes.ram[ADDR_GAME_MODE] = MODE_LEGAL_SCREEN;
    branchOnGameMode(&nes);
    assert(nes.ram[ADDR_GAME_MODE] == MODE_TITLE_SCREEN);

    /* Mode 1: Title Screen with START -> transitions to 2 */
    nes.ram[ADDR_GAME_MODE] = MODE_TITLE_SCREEN;
    nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;
    branchOnGameMode(&nes);
    assert(nes.ram[ADDR_GAME_MODE] == MODE_GAME_TYPE_MENU);
    assert(nes.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == SFX_MENU_SELECT);

    /* Mode 1: Title Screen with timeout -> transitions to 6 */
    nes.ram[ADDR_GAME_MODE] = MODE_TITLE_SCREEN;
    nes.ram[ADDR_NEWLY_PRESSED_P1] = 0;
    nes.ram[ADDR_FRAME_COUNTER + 1] = TITLE_SCREEN_TIMEOUT_HI;
    branchOnGameMode(&nes);
    assert(nes.ram[ADDR_GAME_MODE] == MODE_START_DEMO);

    /* Mode 6: Start Demo -> resets state and transitions to 5 */
    nes.ram[ADDR_GAME_MODE] = MODE_START_DEMO;
    branchOnGameMode(&nes);
    assert(nes.ram[ADDR_GAME_MODE] == MODE_DEMO);
    assert(nes.ram[ADDR_GAME_TYPE] == 0);

    /* Out of range mode safety */
    nes.ram[ADDR_GAME_MODE] = 99;
    branchOnGameMode(&nes);
    assert(nes.ram[ADDR_GAME_MODE] == 99);

    printf("  [PASS] test_branch_on_gamemode\n");
}

int test_menu_subsystem(void) {
    printf("[RUN] Running Menus and Screen Modes tests...\n");
    test_legal_screen();
    test_title_screen();
    test_start_demo();
    test_branch_on_gamemode();
    printf("[PASS] All Menus and Screen Modes unit tests passed successfully!\n");
    return 0;
}
