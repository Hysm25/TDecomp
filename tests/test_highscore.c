#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "nes.h"
#include "game/highscore.h"
#include "constants/memory.h"
#include "constants/gameplay.h"
#include "constants/hardware.h"

int test_highscore_subsystem(void) {
    int failures = 0;
    printf("--- Running High Score Subsystem Tests ---\n");

    /* Test 1: Cold boot initialization & warm boot detection */
    {
        NES_State nes;
        nes_init(&nes);

        assert(!isWarmBoot(&nes));

        initHighScoreTable(&nes);

        assert(isWarmBoot(&nes));
        assert(nes.ram[ADDR_INIT_MAGIC]     == 0x12);
        assert(nes.ram[ADDR_INIT_MAGIC + 1] == 0x34);
        assert(nes.ram[ADDR_INIT_MAGIC + 2] == 0x56);
        assert(nes.ram[ADDR_INIT_MAGIC + 3] == 0x78);
        assert(nes.ram[ADDR_INIT_MAGIC + 4] == 0x9A);

        /* Verify default names */
        /* Entry 0: HOWARD */
        assert(nes.ram[ADDR_HIGH_SCORE_NAMES + 0] == 0x08);
        assert(nes.ram[ADDR_HIGH_SCORE_NAMES + 1] == 0x0F);
        assert(nes.ram[ADDR_HIGH_SCORE_NAMES + 2] == 0x17);
        assert(nes.ram[ADDR_HIGH_SCORE_NAMES + 3] == 0x01);
        assert(nes.ram[ADDR_HIGH_SCORE_NAMES + 4] == 0x12);
        assert(nes.ram[ADDR_HIGH_SCORE_NAMES + 5] == 0x04);

        /* Entry 1: OTASAN */
        assert(nes.ram[ADDR_HIGH_SCORE_NAMES + 6]  == 0x0F);
        assert(nes.ram[ADDR_HIGH_SCORE_NAMES + 7]  == 0x14);
        assert(nes.ram[ADDR_HIGH_SCORE_NAMES + 8]  == 0x01);
        assert(nes.ram[ADDR_HIGH_SCORE_NAMES + 9]  == 0x13);
        assert(nes.ram[ADDR_HIGH_SCORE_NAMES + 10] == 0x01);
        assert(nes.ram[ADDR_HIGH_SCORE_NAMES + 11] == 0x0E);

        /* Default scores */
        /* Entry 0: 10,000 (0x01, 0x00, 0x00) */
        assert(nes.ram[ADDR_HIGH_SCORE_A + 0] == 0x01);
        assert(nes.ram[ADDR_HIGH_SCORE_A + 1] == 0x00);
        assert(nes.ram[ADDR_HIGH_SCORE_A + 2] == 0x00);

        /* Entry 1: 7,500 (0x00, 0x75, 0x00) */
        assert(nes.ram[ADDR_HIGH_SCORE_A + 3] == 0x00);
        assert(nes.ram[ADDR_HIGH_SCORE_A + 4] == 0x75);
        assert(nes.ram[ADDR_HIGH_SCORE_A + 5] == 0x00);

        /* Entry 2: 5,000 (0x00, 0x50, 0x00) */
        assert(nes.ram[ADDR_HIGH_SCORE_A + 6] == 0x00);
        assert(nes.ram[ADDR_HIGH_SCORE_A + 7] == 0x50);
        assert(nes.ram[ADDR_HIGH_SCORE_A + 8] == 0x00);

        /* Default levels */
        assert(nes.ram[ADDR_HIGH_SCORE_LEVELS + 0] == 0x09);
        assert(nes.ram[ADDR_HIGH_SCORE_LEVELS + 1] == 0x05);
        assert(nes.ram[ADDR_HIGH_SCORE_LEVELS + 2] == 0x00);

        /* Type B entries */
        /* Entry 4: ALEX__ */
        assert(nes.ram[ADDR_HIGH_SCORE_NAMES + 24] == 0x01);
        assert(nes.ram[ADDR_HIGH_SCORE_NAMES + 25] == 0x0C);
        assert(nes.ram[ADDR_HIGH_SCORE_NAMES + 26] == 0x05);
        assert(nes.ram[ADDR_HIGH_SCORE_NAMES + 27] == 0x18);
        assert(nes.ram[ADDR_HIGH_SCORE_NAMES + 28] == 0x2B);
        assert(nes.ram[ADDR_HIGH_SCORE_NAMES + 29] == 0x2B);

        /* Type B scores */
        /* Entry 4: 2,000 (0x00, 0x20, 0x00) */
        assert(nes.ram[ADDR_HIGH_SCORE_B + 0] == 0x00);
        assert(nes.ram[ADDR_HIGH_SCORE_B + 1] == 0x20);
        assert(nes.ram[ADDR_HIGH_SCORE_B + 2] == 0x00);
    }
    printf("  [PASS] test_highscore_cold_and_warm_boot\n");

    /* Test 2: twoDigsToPPU */
    {
        NES_State nes;
        nes_init(&nes);

        twoDigsToPPU(&nes, 0x42);
        assert(nes.ram[ADDR_GENERAL_COUNTER] == 0x42);

        twoDigsToPPU(&nes, 0x90);
        assert(nes.ram[ADDR_GENERAL_COUNTER] == 0x90);

        twoDigsToPPU(&nes, 0x07);
        assert(nes.ram[ADDR_GENERAL_COUNTER] == 0x07);
    }
    printf("  [PASS] test_two_digs_to_ppu\n");

    /* Test 3: copyHighScoreNameToNextIndex (Type A & Type B) */
    {
        NES_State nes;
        nes_init(&nes);
        initHighScoreTable(&nes);

        /* Type A: Copy entry 1 (OTASAN) to entry 2 */
        nes.ram[ADDR_GAME_TYPE] = 0;
        copyHighScoreNameToNextIndex(&nes, 0x06);
        assert(nes.ram[ADDR_GENERAL_COUNTER2] == 0xFF);
        /* Entry 2 should now have OTASAN */
        assert(memcmp(&nes.ram[ADDR_HIGH_SCORE_NAMES + 12], &nes.ram[ADDR_HIGH_SCORE_NAMES + 6], 6) == 0);

        /* Type A: Copy entry 0 (HOWARD) to entry 1 */
        copyHighScoreNameToNextIndex(&nes, 0x00);
        assert(memcmp(&nes.ram[ADDR_HIGH_SCORE_NAMES + 6], "HOWARD", 6) != 0); /* char codes != ascii */
        assert(nes.ram[ADDR_HIGH_SCORE_NAMES + 6] == 0x08);
        assert(nes.ram[ADDR_HIGH_SCORE_NAMES + 7] == 0x0F);

        /* Type B: Copy entry 4 (ALEX__) to entry 5 */
        nes.ram[ADDR_GAME_TYPE] = 1;
        copyHighScoreNameToNextIndex(&nes, 0x00); /* Start byte 0 -> +0x18 = offset 24 (entry 4) */
        /* Entry 5 (offset 30) should now have ALEX__ */
        assert(nes.ram[ADDR_HIGH_SCORE_NAMES + 30] == 0x01);
        assert(nes.ram[ADDR_HIGH_SCORE_NAMES + 31] == 0x0C);
    }
    printf("  [PASS] test_copy_high_score_name_to_next_index\n");

    /* Test 4: copyHighScoreScoreToNextIndex & copyHighScoreLevelToNextIndex */
    {
        NES_State nes;
        nes_init(&nes);
        initHighScoreTable(&nes);

        /* Type A: Copy entry 1 score (7,500) to entry 2 */
        nes.ram[ADDR_GAME_TYPE] = 0;
        copyHighScoreScoreToNextIndex(&nes, 0x03);
        assert(nes.ram[ADDR_HIGH_SCORE_A + 6] == 0x00);
        assert(nes.ram[ADDR_HIGH_SCORE_A + 7] == 0x75);
        assert(nes.ram[ADDR_HIGH_SCORE_A + 8] == 0x00);

        /* Type A: Copy entry 1 level (5) to entry 2 */
        copyHighScoreLevelToNextIndex(&nes, 0x01);
        assert(nes.ram[ADDR_HIGH_SCORE_LEVELS + 2] == 0x05);

        /* Type B: Copy entry 4 score (2,000) to entry 5 */
        nes.ram[ADDR_GAME_TYPE] = 1;
        copyHighScoreScoreToNextIndex(&nes, 0x00);
        assert(nes.ram[ADDR_HIGH_SCORE_B + 3] == 0x00);
        assert(nes.ram[ADDR_HIGH_SCORE_B + 4] == 0x20);
        assert(nes.ram[ADDR_HIGH_SCORE_B + 5] == 0x00);

        /* Type B: Copy entry 4 level (9) to entry 5 */
        copyHighScoreLevelToNextIndex(&nes, 0x00);
        assert(nes.ram[ADDR_HIGH_SCORE_LEVELS + 5] == 0x09);
    }
    printf("  [PASS] test_copy_high_score_score_and_level\n");

    /* Test 5: handleHighScoreIfNecessary - non-qualifying score */
    {
        NES_State nes;
        nes_init(&nes);
        initHighScoreTable(&nes);

        nes.ram[ADDR_GAME_TYPE] = 0;
        /* Player score 1,000 (less than 3rd place 5,000) */
        nes.ram[ADDR_P1_SCORE]     = 0x00;
        nes.ram[ADDR_P1_SCORE + 1] = 0x10;
        nes.ram[ADDR_P1_SCORE + 2] = 0x00;
        nes.ram[ADDR_P1_LEVEL_NUMBER] = 3;

        handleHighScoreIfNecessary(&nes);

        /* Must not qualify: rawPos reached 3 */
        assert(nes.ram[ADDR_HIGH_SCORE_RAW_POS] == 3);
        /* High score table remains untouched */
        assert(nes.ram[ADDR_HIGH_SCORE_A + 0] == 0x01);
        assert(nes.ram[ADDR_HIGH_SCORE_A + 3] == 0x00);
        assert(nes.ram[ADDR_HIGH_SCORE_A + 6] == 0x00);
    }
    printf("  [PASS] test_handle_high_score_non_qualifying\n");

    /* Test 6: handleHighScoreIfNecessary - qualify for 1st place */
    {
        NES_State nes;
        nes_init(&nes);
        initHighScoreTable(&nes);

        nes.ram[ADDR_GAME_TYPE] = 0;
        /* Player score 25,000 beats HOWARD's 10,000 */
        nes.ram[ADDR_P1_SCORE]     = 0x00;
        nes.ram[ADDR_P1_SCORE + 1] = 0x50;
        nes.ram[ADDR_P1_SCORE + 2] = 0x02;
        nes.ram[ADDR_P1_LEVEL_NUMBER] = 7;

        handleHighScoreIfNecessary(&nes);

        /* Position 0 qualified */
        assert(nes.ram[ADDR_HIGH_SCORE_RAW_POS] == 0);

        /* New 1st place score is 25,000 */
        assert(nes.ram[ADDR_HIGH_SCORE_A + 0] == 0x02);
        assert(nes.ram[ADDR_HIGH_SCORE_A + 1] == 0x50);
        assert(nes.ram[ADDR_HIGH_SCORE_A + 2] == 0x00);
        assert(nes.ram[ADDR_HIGH_SCORE_LEVELS + 0] == 7);
        /* Name slot for entry 0 cleared to 0 for input */
        for (int i = 0; i < 6; i++) {
            assert(nes.ram[ADDR_HIGH_SCORE_NAMES + i] == 0x00);
        }

        /* 2nd place should now be HOWARD (10,000, level 9) */
        assert(nes.ram[ADDR_HIGH_SCORE_A + 3] == 0x01);
        assert(nes.ram[ADDR_HIGH_SCORE_A + 4] == 0x00);
        assert(nes.ram[ADDR_HIGH_SCORE_A + 5] == 0x00);
        assert(nes.ram[ADDR_HIGH_SCORE_LEVELS + 1] == 9);
        assert(nes.ram[ADDR_HIGH_SCORE_NAMES + 6] == 0x08);

        /* 3rd place should now be OTASAN (7,500, level 5) */
        assert(nes.ram[ADDR_HIGH_SCORE_A + 6] == 0x00);
        assert(nes.ram[ADDR_HIGH_SCORE_A + 7] == 0x75);
        assert(nes.ram[ADDR_HIGH_SCORE_A + 8] == 0x00);
        assert(nes.ram[ADDR_HIGH_SCORE_LEVELS + 2] == 5);
        assert(nes.ram[ADDR_HIGH_SCORE_NAMES + 12] == 0x0F);
    }
    printf("  [PASS] test_handle_high_score_1st_place\n");

    /* Test 7: handleHighScoreIfNecessary - tie-breaking preference */
    {
        NES_State nes;
        nes_init(&nes);
        initHighScoreTable(&nes);

        nes.ram[ADDR_GAME_TYPE] = 0;
        /* Player exactly ties 2nd place score 7,500 */
        nes.ram[ADDR_P1_SCORE]     = 0x00;
        nes.ram[ADDR_P1_SCORE + 1] = 0x75;
        nes.ram[ADDR_P1_SCORE + 2] = 0x00;
        nes.ram[ADDR_P1_LEVEL_NUMBER] = 4;

        handleHighScoreIfNecessary(&nes);

        /* Tie breaks in favor of new score: enters at position 1 */
        assert(nes.ram[ADDR_HIGH_SCORE_RAW_POS] == 1);
        /* 1st place unchanged (HOWARD, 10,000) */
        assert(nes.ram[ADDR_HIGH_SCORE_A + 0] == 0x01);
        assert(nes.ram[ADDR_HIGH_SCORE_NAMES + 0] == 0x08);
        /* 2nd place is player (level 4) */
        assert(nes.ram[ADDR_HIGH_SCORE_A + 3] == 0x00);
        assert(nes.ram[ADDR_HIGH_SCORE_A + 4] == 0x75);
        assert(nes.ram[ADDR_HIGH_SCORE_LEVELS + 1] == 4);
        /* 3rd place is previous 2nd place (OTASAN, 7,500, level 5) */
        assert(nes.ram[ADDR_HIGH_SCORE_A + 6] == 0x00);
        assert(nes.ram[ADDR_HIGH_SCORE_A + 7] == 0x75);
        assert(nes.ram[ADDR_HIGH_SCORE_LEVELS + 2] == 5);
    }
    printf("  [PASS] test_handle_high_score_tie_breaking\n");

    /* Test 8: highScoreEntryScreen_step navigation & letter cycling */
    {
        NES_State nes;
        nes_init(&nes);
        initHighScoreTable(&nes);

        nes.ram[ADDR_HIGH_SCORE_RAW_POS] = 0;
        nes.ram[ADDR_HIGH_SCORE_NAME_OFFSET_ROW] = 0;
        nes.ram[ADDR_HIGH_SCORE_NAME_OFFSET_LETTER] = 0;
        nes.ram[ADDR_FRAME_COUNTER] = 0;

        /* Initial letter is 0x08 ('H') */
        assert(nes.ram[ADDR_HIGH_SCORE_NAMES + 0] == 0x08);

        /* Press RIGHT: cursor advances to letter 1 */
        nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_RIGHT;
        bool done = highScoreEntryScreen_step(&nes);
        assert(!done);
        (void)done;
        assert(nes.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == SFX_MENU_MOVE);
        assert(nes.ram[ADDR_HIGH_SCORE_NAME_OFFSET_LETTER] == 1);

        /* Next frame: sprite X offset updates to letter 1 coordinate */
        nes.ram[ADDR_NEWLY_PRESSED_P1] = 0;
        done = highScoreEntryScreen_step(&nes);
        assert(!done);
        assert(nes.ram[ADDR_SPRITE_X_OFFSET] == highScoreNamePosToX[1]);

        /* Press LEFT: cursor retreats to letter 0 */
        nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_LEFT;
        done = highScoreEntryScreen_step(&nes);
        assert(!done);
        assert(nes.ram[ADDR_HIGH_SCORE_NAME_OFFSET_LETTER] == 0);

        /* Press LEFT again: wraps to letter 5 */
        nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_LEFT;
        done = highScoreEntryScreen_step(&nes);
        assert(!done);
        assert(nes.ram[ADDR_HIGH_SCORE_NAME_OFFSET_LETTER] == 5);

        /* Press RIGHT: wraps back to letter 0 */
        nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_RIGHT;
        done = highScoreEntryScreen_step(&nes);
        assert(!done);
        assert(nes.ram[ADDR_HIGH_SCORE_NAME_OFFSET_LETTER] == 0);

        /* Hold UP on frameCounter % 8 == 0: letter increments from 0x08 to 0x09 */
        nes.ram[ADDR_NEWLY_PRESSED_P1] = 0;
        nes.ram[ADDR_HELD_BUTTONS_P1] = BUTTON_UP;
        nes.ram[ADDR_FRAME_COUNTER] = 0x08;
        done = highScoreEntryScreen_step(&nes);
        assert(!done);
        assert(nes.ram[ADDR_HIGH_SCORE_NAMES + 0] == 0x09);
        assert(nes.ram[ADDR_HIGH_SCORE_CURRENT_LETTER] == 0x09);
        assert(nes.ram[ADDR_OUT_OF_DATE_RENDER_FLAGS] == RENDER_HIGH_SCORE_LETTER);

        /* Hold DOWN on frameCounter % 8 == 0: letter decrements back to 0x08 */
        nes.ram[ADDR_HELD_BUTTONS_P1] = BUTTON_DOWN;
        nes.ram[ADDR_FRAME_COUNTER] = 0x10;
        done = highScoreEntryScreen_step(&nes);
        assert(!done);
        assert(nes.ram[ADDR_HIGH_SCORE_NAMES + 0] == 0x08);

        /* Press START: name entry finished */
        nes.ram[ADDR_HELD_BUTTONS_P1] = 0;
        nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;
        done = highScoreEntryScreen_step(&nes);
        assert(done);
        (void)done;
        assert(nes.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == SFX_MENU_SELECT);
    }
    printf("  [PASS] test_high_score_entry_screen_navigation\n");

    /* Test 9: showHighScores (1P render vs 2P skip) */
    {
        NES_State nes;
        nes_init(&nes);
        initHighScoreTable(&nes);

        /* 2-player mode: bypasses PPU display */
        nes.ram[ADDR_NUMBER_OF_PLAYERS] = 2;
        showHighScores(&nes);

        /* 1-player mode Type A */
        nes.ram[ADDR_NUMBER_OF_PLAYERS] = 1;
        nes.ram[ADDR_GAME_TYPE] = 0;
        showHighScores(&nes);
        assert(nes.ram[ADDR_GENERAL_COUNTER2] == 3);

        /* 1-player mode Type B */
        nes.ram[ADDR_GAME_TYPE] = 1;
        showHighScores(&nes);
        assert(nes.ram[ADDR_GENERAL_COUNTER2] == 7);
    }
    /* Test 10: render_mode_congratulations_screen */
    {
        NES_State nes;
        nes_init(&nes);

        /* Flag not set: no-op */
        nes.ram[ADDR_OUT_OF_DATE_RENDER_FLAGS] = 0;
        render_mode_congratulations_screen(&nes);
        assert(nes.ram[ADDR_OUT_OF_DATE_RENDER_FLAGS] == 0);

        /* Flag set: writes to PPU, clears flags, resets scroll */
        nes.ram[ADDR_OUT_OF_DATE_RENDER_FLAGS] = RENDER_HIGH_SCORE_LETTER;
        nes.ram[ADDR_HIGH_SCORE_RAW_POS] = 0;
        nes.ram[ADDR_HIGH_SCORE_NAME_OFFSET_LETTER] = 2;
        nes.ram[ADDR_HIGH_SCORE_CURRENT_LETTER] = 1; /* 'A' */
        nes.ram[ADDR_PPU_SCROLL_X] = 0x10;
        nes.ram[ADDR_PPU_SCROLL_Y] = 0x20;

        render_mode_congratulations_screen(&nes);

        assert(nes.ram[ADDR_OUT_OF_DATE_RENDER_FLAGS] == 0);
        assert(nes.ram[ADDR_PPU_SCROLL_X] == 0);
        assert(nes.ram[ADDR_PPU_SCROLL_Y] == 0);
        assert(nes.ram[ADDR_GENERAL_COUNTER] == 0x89);
    }
    printf("  [PASS] test_render_mode_congratulations_screen\n");

    printf("--- All High Score Subsystem Tests Passed ---\n");
    return failures;
}
