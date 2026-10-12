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

    /* Mode 2: Game Type Menu with START -> transitions to 3 */
    nes.ram[ADDR_GAME_MODE] = MODE_GAME_TYPE_MENU;
    nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;
    branchOnGameMode(&nes);
    assert(nes.ram[ADDR_GAME_MODE] == MODE_LEVEL_MENU);
    assert(nes.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == SFX_MENU_SELECT);

    /* Mode 3: Level Menu with START -> transitions to 4 */
    nes.ram[ADDR_GAME_MODE] = MODE_LEVEL_MENU;
    nes.ram[ADDR_P1_START_LEVEL] = 0;
    nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;
    nes.ram[ADDR_HELD_BUTTONS_P1] = BUTTON_START;
    branchOnGameMode(&nes);
    assert(nes.ram[ADDR_GAME_MODE] == MODE_PLAY);
    assert(nes.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == SFX_MENU_SELECT);

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

static void test_game_type_menu(void) {
    NES_State nes;
    nes_init(&nes);

    /* Test 1: Navigation across game types (A <-> B) */
    nes.ram[ADDR_GAME_MODE] = MODE_GAME_TYPE_MENU;
    nes.ram[ADDR_GAME_TYPE] = GAMETYPE_A;
    nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_RIGHT;

    gameMode_gameTypeMenu_step(&nes);

    assert(nes.ram[ADDR_GAME_TYPE] == GAMETYPE_B);
    assert(nes.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == SFX_MENU_MOVE);

    nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_LEFT;
    gameMode_gameTypeMenu_step(&nes);

    assert(nes.ram[ADDR_GAME_TYPE] == GAMETYPE_A);
    assert(nes.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == SFX_MENU_MOVE);

    /* Test 2: Music selection and clamping (0..3) */
    nes.ram[ADDR_MUSIC_TYPE] = 0;
    nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_DOWN;
    gameMode_gameTypeMenu_step(&nes);
    assert(nes.ram[ADDR_MUSIC_TYPE] == 1);
    assert(nes.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == SFX_MENU_MOVE);

    nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_DOWN;
    gameMode_gameTypeMenu_step(&nes);
    assert(nes.ram[ADDR_MUSIC_TYPE] == 2);

    nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_DOWN;
    gameMode_gameTypeMenu_step(&nes);
    assert(nes.ram[ADDR_MUSIC_TYPE] == 3);

    /* Clamped at max (3) */
    nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_DOWN;
    gameMode_gameTypeMenu_step(&nes);
    assert(nes.ram[ADDR_MUSIC_TYPE] == 3);

    /* Moving up */
    nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_UP;
    gameMode_gameTypeMenu_step(&nes);
    assert(nes.ram[ADDR_MUSIC_TYPE] == 2);

    /* Test 3: START button advances to Level Menu (Mode 3) */
    nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;
    gameMode_gameTypeMenu_step(&nes);
    assert(nes.ram[ADDR_GAME_MODE] == MODE_LEVEL_MENU);
    assert(nes.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == SFX_MENU_SELECT);

    /* Test 4: BUTTON_B retreats to Title Screen (Mode 1) and resets frameCounter+1 */
    nes.ram[ADDR_GAME_MODE] = MODE_GAME_TYPE_MENU;
    nes.ram[ADDR_FRAME_COUNTER + 1] = 4;
    nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_B;
    gameMode_gameTypeMenu_step(&nes);
    assert(nes.ram[ADDR_GAME_MODE] == MODE_TITLE_SCREEN);
    assert(nes.ram[ADDR_FRAME_COUNTER + 1] == 0);
    assert(nes.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == SFX_MENU_SELECT);

    /* Test 5: Full gameMode_gameTypeMenu entry */
    nes_init(&nes);
    nes.ram[ADDR_GAME_MODE] = MODE_GAME_TYPE_MENU;
    nes.ram[ADDR_MUSIC_TYPE] = 2;
    nes.ram[ADDR_NEWLY_PRESSED_P1] = 0;
    gameMode_gameTypeMenu(&nes);
    assert(nes.ram[ADDR_RENDER_MODE] == 1);

    printf("  [PASS] test_game_type_menu\n");
}

static void test_level_menu_handle_navigation(void) {
    NES_State nes;
    nes_init(&nes);

    /* Type A level navigation: 0..9 grid */
    nes.ram[ADDR_GAME_TYPE] = GAMETYPE_A;
    nes.ram[ADDR_SELECTING_LEVEL_OR_HEIGHT] = 0;
    nes.ram[ADDR_START_LEVEL] = 0;

    /* Move Right */
    nes.ram[ADDR_NEWLY_PRESSED_BUTTONS] = BUTTON_RIGHT;
    gameMode_levelMenu_handleLevelHeightNavigation(&nes);
    assert(nes.ram[ADDR_START_LEVEL] == 1);
    assert(nes.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == SFX_MENU_MOVE);

    /* Move Left */
    nes.ram[ADDR_NEWLY_PRESSED_BUTTONS] = BUTTON_LEFT;
    gameMode_levelMenu_handleLevelHeightNavigation(&nes);
    assert(nes.ram[ADDR_START_LEVEL] == 0);

    /* Move Left clamped at 0 */
    nes.ram[ADDR_NEWLY_PRESSED_BUTTONS] = BUTTON_LEFT;
    gameMode_levelMenu_handleLevelHeightNavigation(&nes);
    assert(nes.ram[ADDR_START_LEVEL] == 0);

    /* Move Down (+5) */
    nes.ram[ADDR_NEWLY_PRESSED_BUTTONS] = BUTTON_DOWN;
    gameMode_levelMenu_handleLevelHeightNavigation(&nes);
    assert(nes.ram[ADDR_START_LEVEL] == 5);

    /* Move Down clamped (already >= 5) */
    nes.ram[ADDR_NEWLY_PRESSED_BUTTONS] = BUTTON_DOWN;
    gameMode_levelMenu_handleLevelHeightNavigation(&nes);
    assert(nes.ram[ADDR_START_LEVEL] == 5);

    /* Move Up (-5) */
    nes.ram[ADDR_NEWLY_PRESSED_BUTTONS] = BUTTON_UP;
    gameMode_levelMenu_handleLevelHeightNavigation(&nes);
    assert(nes.ram[ADDR_START_LEVEL] == 0);

    /* Clamp right at 9 */
    nes.ram[ADDR_START_LEVEL] = 9;
    nes.ram[ADDR_NEWLY_PRESSED_BUTTONS] = BUTTON_RIGHT;
    gameMode_levelMenu_handleLevelHeightNavigation(&nes);
    assert(nes.ram[ADDR_START_LEVEL] == 9);

    /* Button A has no effect in Type A */
    nes.ram[ADDR_NEWLY_PRESSED_BUTTONS] = BUTTON_A;
    gameMode_levelMenu_handleLevelHeightNavigation(&nes);
    assert(nes.ram[ADDR_SELECTING_LEVEL_OR_HEIGHT] == 0);

    /* Type B height navigation */
    nes.ram[ADDR_GAME_TYPE] = GAMETYPE_B;
    nes.ram[ADDR_SELECTING_LEVEL_OR_HEIGHT] = 0;
    nes.ram[ADDR_START_HEIGHT] = 0;

    /* Button A toggles to height selection */
    nes.ram[ADDR_NEWLY_PRESSED_BUTTONS] = BUTTON_A;
    gameMode_levelMenu_handleLevelHeightNavigation(&nes);
    assert(nes.ram[ADDR_SELECTING_LEVEL_OR_HEIGHT] == 1);
    assert(nes.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == SFX_MENU_MOVE);

    /* Height Right */
    nes.ram[ADDR_NEWLY_PRESSED_BUTTONS] = BUTTON_RIGHT;
    gameMode_levelMenu_handleLevelHeightNavigation(&nes);
    assert(nes.ram[ADDR_START_HEIGHT] == 1);

    /* Height Left */
    nes.ram[ADDR_NEWLY_PRESSED_BUTTONS] = BUTTON_LEFT;
    gameMode_levelMenu_handleLevelHeightNavigation(&nes);
    assert(nes.ram[ADDR_START_HEIGHT] == 0);

    /* Height Down (+3) */
    nes.ram[ADDR_NEWLY_PRESSED_BUTTONS] = BUTTON_DOWN;
    gameMode_levelMenu_handleLevelHeightNavigation(&nes);
    assert(nes.ram[ADDR_START_HEIGHT] == 3);

    /* Height Up (-3) */
    nes.ram[ADDR_NEWLY_PRESSED_BUTTONS] = BUTTON_UP;
    gameMode_levelMenu_handleLevelHeightNavigation(&nes);
    assert(nes.ram[ADDR_START_HEIGHT] == 0);

    /* Clamping height at 5 */
    nes.ram[ADDR_START_HEIGHT] = 5;
    nes.ram[ADDR_NEWLY_PRESSED_BUTTONS] = BUTTON_RIGHT;
    gameMode_levelMenu_handleLevelHeightNavigation(&nes);
    assert(nes.ram[ADDR_START_HEIGHT] == 5);

    /* Player 2 offset (+0x50) in Type A (Level cursor) */
    nes.ram[ADDR_GAME_TYPE] = GAMETYPE_A;
    nes.ram[ADDR_SELECTING_LEVEL_OR_HEIGHT] = 0;
    nes.ram[ADDR_ACTIVE_PLAYER] = 1;
    nes.ram[ADDR_START_LEVEL] = 2;
    nes.ram[ADDR_FRAME_COUNTER] = 1;
    nes.ram[ADDR_NEWLY_PRESSED_BUTTONS] = 0;
    gameMode_levelMenu_handleLevelHeightNavigation(&nes);
    assert(nes.ram[ADDR_SPRITE_Y_OFFSET] == (uint8_t)(levelToSpriteYOffset[2] + 0x50));

    /* Player 2 offset (+0x50) in Type B (Height cursor staged last) */
    nes.ram[ADDR_GAME_TYPE] = GAMETYPE_B;
    nes.ram[ADDR_START_HEIGHT] = 3;
    gameMode_levelMenu_handleLevelHeightNavigation(&nes);
    assert(nes.ram[ADDR_SPRITE_Y_OFFSET] == (uint8_t)(heightToPpuHighAddr[3] + 0x50));

    printf("  [PASS] test_level_menu_handle_navigation\n");
}

static void test_level_menu_process_player1(void) {
    NES_State nes;
    nes_init(&nes);

    /* Test 1: Normal START advances to Mode 4 (MODE_PLAY) */
    nes.ram[ADDR_GAME_MODE] = MODE_LEVEL_MENU;
    nes.ram[ADDR_P1_START_LEVEL] = 5;
    nes.ram[ADDR_P1_START_HEIGHT] = 2;
    nes.ram[ADDR_ORIGINAL_Y] = 0;
    nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;
    nes.ram[ADDR_HELD_BUTTONS_P1] = BUTTON_START;

    gameMode_levelMenu_processPlayer1Navigation(&nes);

    assert(nes.ram[ADDR_GAME_MODE] == MODE_PLAY);
    assert(nes.ram[ADDR_P1_START_LEVEL] == 5);
    assert(nes.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == SFX_MENU_SELECT);
    assert(nes.ram[ADDR_GAME_MODE_STATE] == 0);

    /* Test 2: Cheat Code A+START adds 10 to start level */
    nes_init(&nes);
    nes.ram[ADDR_GAME_MODE] = MODE_LEVEL_MENU;
    nes.ram[ADDR_P1_START_LEVEL] = 9;
    nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;
    nes.ram[ADDR_HELD_BUTTONS_P1] = BUTTON_A | BUTTON_START;

    gameMode_levelMenu_processPlayer1Navigation(&nes);

    assert(nes.ram[ADDR_GAME_MODE] == MODE_PLAY);
    assert(nes.ram[ADDR_P1_START_LEVEL] == 19);
    assert(nes.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == SFX_MENU_SELECT);

    /* Test 3: BUTTON_B retreats to Game Type Menu (Mode 2) */
    nes_init(&nes);
    nes.ram[ADDR_GAME_MODE] = MODE_LEVEL_MENU;
    nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_B;

    gameMode_levelMenu_processPlayer1Navigation(&nes);

    assert(nes.ram[ADDR_GAME_MODE] == MODE_GAME_TYPE_MENU);
    assert(nes.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == SFX_MENU_SELECT);

    /* Test 4: Idle frame generates random garbage holes 0..9 for P1 & P2 */
    nes_init(&nes);
    nes.ram[ADDR_GAME_MODE] = MODE_LEVEL_MENU;
    nes.ram[ADDR_RNG_SEED] = 0x34;
    nes.ram[ADDR_RNG_SEED + 1] = 0x12;
    nes.ram[ADDR_NEWLY_PRESSED_P1] = 0;

    gameMode_levelMenu_processPlayer1Navigation(&nes);

    assert(nes.ram[ADDR_GAME_MODE] == MODE_LEVEL_MENU);
    assert(nes.ram[ADDR_P1_GARBAGE_HOLE] <= 9);
    assert(nes.ram[ADDR_P2_GARBAGE_HOLE] <= 9);

    /* Test 5: Step helper */
    nes_init(&nes);
    nes.ram[ADDR_GAME_MODE] = MODE_LEVEL_MENU;
    nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;
    nes.ram[ADDR_HELD_BUTTONS_P1] = BUTTON_START;
    gameMode_levelMenu_step(&nes);
    assert(nes.ram[ADDR_GAME_MODE] == MODE_PLAY);

    printf("  [PASS] test_level_menu_process_player1\n");
}

static void test_level_menu_normalization(void) {
    NES_State nes;
    nes_init(&nes);

    /* Calling gameMode_levelMenu normalizes level >= 10 back to 0..9 */
    nes.ram[ADDR_GAME_MODE] = MODE_LEVEL_MENU;
    nes.ram[ADDR_P1_START_LEVEL] = 19;
    nes.ram[ADDR_ORIGINAL_Y] = 5;
    nes.ram[ADDR_DROP_SPEED] = 10;
    nes.ram[ADDR_NEWLY_PRESSED_P1] = 0;

    gameMode_levelMenu(&nes);

    assert(nes.ram[ADDR_P1_START_LEVEL] == 9);
    assert(nes.ram[ADDR_ORIGINAL_Y] == 0);
    assert(nes.ram[ADDR_DROP_SPEED] == 0);
    assert(nes.ram[ADDR_RENDER_MODE] == 1);

    /* 25 -> 5 */
    nes.ram[ADDR_P1_START_LEVEL] = 25;
    gameMode_levelMenu(&nes);
    assert(nes.ram[ADDR_P1_START_LEVEL] == 5);

    printf("  [PASS] test_level_menu_normalization\n");
}

int test_menu_subsystem(void) {
    printf("[RUN] Running Menus and Screen Modes tests...\n");
    test_legal_screen();
    test_title_screen();
    test_start_demo();
    test_branch_on_gamemode();
    test_game_type_menu();
    test_level_menu_handle_navigation();
    test_level_menu_process_player1();
    test_level_menu_normalization();
    printf("[PASS] All Menus and Screen Modes unit tests passed successfully!\n");
    return 0;
}
