#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "game_adapter.h"
#include "rom_assets.h"
#include "constants/gameplay.h"
#include "constants/memory.h"

static RomAssets s_test_assets;

static void ensure_assets_loaded(void) {
    if (!s_test_assets.is_loaded) {
        char err[256];
        RomStatus st = rom_assets_find_and_load(&s_test_assets, NULL, err, sizeof(err));
        assert(st == ROM_STATUS_OK && "Reference ROM must be available for native adapter test suite");
    }
}

static void test_adapter_rom_required_enforcement(void) {
    GameAdapter adapter;

    /* Initialize with NULL assets: must be gated in SCREEN_ROM_REQUIRED */
    game_adapter_init(&adapter, NULL);
    assert(adapter.screen == SCREEN_ROM_REQUIRED);
    assert(!adapter.rom_loaded);

    /* Starting game without ROM must be blocked */
    game_adapter_start_game(&adapter, 0, GAMETYPE_A, 0);
    assert(adapter.screen == SCREEN_ROM_REQUIRED);

    /* Updating game without ROM must not advance frames */
    game_adapter_update(&adapter, BUTTON_DOWN);
    assert(adapter.frame_count == 0);

    /* Importing valid ROM unlocks title screen */
    ensure_assets_loaded();
    game_adapter_load_rom_assets(&adapter, &s_test_assets);
    assert(adapter.screen == SCREEN_TITLE);
    assert(adapter.rom_loaded);

    printf("  [PASS] test_adapter_rom_required_enforcement (gameplay strictly gated on ROM)\n");
}

static void test_adapter_initialization(void) {
    ensure_assets_loaded();
    GameAdapter adapter;
    game_adapter_init(&adapter, &s_test_assets);

    assert(adapter.screen == SCREEN_TITLE);
    assert(adapter.score == 0);
    assert(adapter.lines == 0);
    assert(adapter.selected_level == 0);
    assert(adapter.selected_gametype == GAMETYPE_A);
    assert(!adapter.paused);
    assert(adapter.high_score > 0); /* Default high score table initialized */

    printf("  [PASS] test_adapter_initialization\n");
}

static void test_adapter_game_start(void) {
    ensure_assets_loaded();
    GameAdapter adapter;
    game_adapter_init(&adapter, &s_test_assets);

    game_adapter_start_game(&adapter, 5, GAMETYPE_A, 0);

    assert(adapter.screen == SCREEN_PLAYING);
    assert(adapter.level == 5);
    assert(adapter.score == 0);
    assert(adapter.lines == 0);
    assert(adapter.nes.ram[ADDR_P1_PLAY_STATE] == PLAYSTATE_CONTROL_ACTIVE);
    assert(adapter.nes.ram[ADDR_P1_TETRIMINO_X] == 5);
    assert(adapter.nes.ram[ADDR_P1_TETRIMINO_Y] == 0);

    /* Verify 10x20 playfield initialized with empty tiles */
    for (int r = 0; r < PLAYFIELD_HEIGHT; r++) {
        for (int c = 0; c < PLAYFIELD_WIDTH; c++) {
            assert(!adapter.playfield[r][c].occupied);
        }
    }

    printf("  [PASS] test_adapter_game_start\n");
}

static void test_adapter_movement_and_rotation(void) {
    ensure_assets_loaded();
    GameAdapter adapter;
    game_adapter_init(&adapter, &s_test_assets);
    game_adapter_start_game(&adapter, 0, GAMETYPE_A, 0);

    uint8_t start_x = adapter.nes.ram[ADDR_P1_TETRIMINO_X];
    uint8_t start_orient = adapter.nes.ram[ADDR_P1_CURRENT_PIECE];

    /* Step 1: Shift Left */
    game_adapter_update(&adapter, BUTTON_LEFT);
    assert(adapter.nes.ram[ADDR_P1_TETRIMINO_X] == (uint8_t)(start_x - 1));

    /* Step 2: Rotate Clockwise (Button A) */
    game_adapter_update(&adapter, BUTTON_A);
    assert(adapter.nes.ram[ADDR_P1_CURRENT_PIECE] != start_orient || start_orient == ORIENT_O_FIXED);

    printf("  [PASS] test_adapter_movement_and_rotation\n");
}

static void test_adapter_soft_drop_and_scoring(void) {
    ensure_assets_loaded();
    GameAdapter adapter;
    game_adapter_init(&adapter, &s_test_assets);
    game_adapter_start_game(&adapter, 0, GAMETYPE_A, 0);

    /* Run 60 frames holding BUTTON_DOWN to soft drop the piece */
    for (int f = 0; f < 60; f++) {
        game_adapter_update(&adapter, BUTTON_DOWN);
    }

    /* Soft drop must have accrued points */
    assert(adapter.score > 0);
    /* Frame count must match */
    assert(adapter.frame_count == 60);

    printf("  [PASS] test_adapter_soft_drop_and_scoring (score: %u)\n", (unsigned int)adapter.score);
}

static void test_adapter_pause_toggle(void) {
    ensure_assets_loaded();
    GameAdapter adapter;
    game_adapter_init(&adapter, &s_test_assets);
    game_adapter_start_game(&adapter, 0, GAMETYPE_A, 0);

    assert(adapter.screen == SCREEN_PLAYING);
    assert(!adapter.paused);

    /* Toggle pause ON */
    game_adapter_toggle_pause(&adapter);
    assert(adapter.screen == SCREEN_PAUSED);
    assert(adapter.paused);

    uint8_t y_before = adapter.nes.ram[ADDR_P1_TETRIMINO_Y];
    game_adapter_update(&adapter, BUTTON_DOWN);
    /* While paused, state must not advance */
    assert(adapter.nes.ram[ADDR_P1_TETRIMINO_Y] == y_before);

    /* Toggle pause OFF */
    game_adapter_toggle_pause(&adapter);
    assert(adapter.screen == SCREEN_PLAYING);
    assert(!adapter.paused);

    printf("  [PASS] test_adapter_pause_toggle\n");
}

static void test_adapter_menu_navigation(void) {
    ensure_assets_loaded();
    GameAdapter adapter;
    game_adapter_init(&adapter, &s_test_assets);

    assert(adapter.screen == SCREEN_TITLE);

    /* Title -> Level select */
    game_adapter_menu_select(&adapter);
    assert(adapter.screen == SCREEN_LEVEL_SELECT);
    assert(adapter.selected_level == 0);

    /* Navigate right */
    game_adapter_menu_right(&adapter);
    assert(adapter.selected_level == 1);

    /* Navigate down (+5) */
    game_adapter_menu_down(&adapter);
    assert(adapter.selected_level == 6);

    /* Navigate up (-5) */
    game_adapter_menu_up(&adapter);
    assert(adapter.selected_level == 1);

    /* Navigate left (-1) */
    game_adapter_menu_left(&adapter);
    assert(adapter.selected_level == 0);

    /* Select to start game */
    game_adapter_menu_select(&adapter);
    assert(adapter.screen == SCREEN_PLAYING);
    assert(adapter.level == 0);

    printf("  [PASS] test_adapter_menu_navigation\n");
}

static void test_adapter_restart(void) {
    ensure_assets_loaded();
    GameAdapter adapter;
    game_adapter_init(&adapter, &s_test_assets);
    game_adapter_start_game(&adapter, 3, GAMETYPE_A, 0);

    /* Play 30 frames */
    for (int f = 0; f < 30; f++) {
        game_adapter_update(&adapter, BUTTON_DOWN);
    }
    assert(adapter.frame_count == 30);

    /* Restart */
    game_adapter_restart(&adapter);
    assert(adapter.screen == SCREEN_PLAYING);
    assert(adapter.level == 3);
    assert(adapter.score == 0);
    assert(adapter.frame_count == 0);
    assert(adapter.nes.ram[ADDR_P1_TETRIMINO_Y] == 0);

    printf("  [PASS] test_adapter_restart\n");
}

int test_native_adapter_subsystem(void) {
    printf("\n--- Testing Native Game Adapter Subsystem ---\n");
    test_adapter_rom_required_enforcement();
    test_adapter_initialization();
    test_adapter_game_start();
    test_adapter_movement_and_rotation();
    test_adapter_soft_drop_and_scoring();
    test_adapter_pause_toggle();
    test_adapter_menu_navigation();
    test_adapter_restart();
    printf("[PASS] All Native Game Adapter tests passed successfully!\n");
    return 0;
}
