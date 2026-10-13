#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "nes.h"
#include "game/ending.h"
#include "game/render.h"

void test_ending_data_tables(void) {
    /* Verify key table properties */
    assert(sizeof(domeLiftoffDelayTable) == 4);
    assert(sizeof(domeNumberToXOffsetTable) == 4);
    assert(sizeof(domeStartHeightsTable) == 4);
    assert(sizeof(domeNumberToSpriteTable) == 4);
    assert(sizeof(domeFlameXOffsetTable) == 4);
    assert(sizeof(ending_typeBCathedralAnimSpeed) == 10);
    assert(sizeof(ending_typeBCathedralFrameDelayTable) == 10);
    assert(sizeof(cathedralStartDeltaTable) == 10);
    assert(sizeof(cathedralEndTargetTable) == 10);
    assert(sizeof(ending_typeBCathedralVectorTable) == 10);
    assert(sizeof(ending_typeBCathedralXTable) == 60);
    assert(sizeof(ending_typeBCathedralYTable) == 60);
    assert(sizeof(ending_typeBCathedralSpriteTable) == 10);
    assert(sizeof(marioFrameToYOffsetTable) == 4);
    assert(sizeof(luigiFrameToYOffsetTable) == 10);
    assert(sizeof(luigiFrameToSpriteTable) == 10);
    assert(sizeof(ending_patchToPpu_typeBConcertHeightUnused) == 18);
    assert(sizeof(ending_patchToPpu_typeBConcertHeight0) == 22);
    assert(sizeof(ending_patchToPpu_typeBConcertHeight1) == 24);
    assert(sizeof(ending_patchToPpu_typeBConcertHeight2) == 24);
    assert(sizeof(ending_patchToPpu_typeBConcertHeight3) == 28);
    assert(sizeof(ending_patchToPpu_typeBConcertHeight4) == 18);
    assert(sizeof(ending_patchToPpu_typeAOver120k) == 89);
    assert(sizeof(rocketToSpriteTable) == 5);
    assert(sizeof(rocketToJetSpriteTable) == 10);
    assert(sizeof(rocketToJetXOffsetTable) == 5);
    assert(sizeof(rocketToXOffsetTable) == 5);
    assert(sizeof(rocketStartYTable) == 5);
    printf("  [PASS] test_ending_data_tables\n");
}

void test_select_ending_screen(void) {
    NES_State nes;
    nes_init(&nes);

    /* Test 1: Score < 50,000 -> ending 0 (small rocket) */
    nes.ram[ADDR_P1_SCORE + 2] = 0x03; /* 30,000 */
    selectEndingScreen(&nes);
    assert(nes.ram[ADDR_ENDING] == 0);
    assert(nes.ram[ADDR_ENDING_CUSTOM_VARS] == 0xBF);
    assert(nes.ram[ADDR_ENDING_CUSTOM_VARS + 1] == 0x00);

    /* Test 2: Score 50,000 <= score < 70,000 -> ending 1 (medium rocket) */
    nes.ram[ADDR_P1_SCORE + 2] = 0x06; /* 60,000 */
    selectEndingScreen(&nes);
    assert(nes.ram[ADDR_ENDING] == 1);
    assert(nes.ram[ADDR_ENDING_CUSTOM_VARS] == 0xBF);
    assert(nes.ram[ADDR_ENDING_CUSTOM_VARS + 1] == 0x00);

    /* Test 3: Score 70,000 <= score < 100,000 -> ending 2 (large rocket) */
    nes.ram[ADDR_P1_SCORE + 2] = 0x08; /* 80,000 */
    selectEndingScreen(&nes);
    assert(nes.ram[ADDR_ENDING] == 2);
    assert(nes.ram[ADDR_ENDING_CUSTOM_VARS] == 0xBF);
    assert(nes.ram[ADDR_ENDING_CUSTOM_VARS + 1] == 0x00);

    /* Test 4: Score 100,000 <= score < 120,000 -> ending 3 (huge rocket) */
    nes.ram[ADDR_P1_SCORE + 2] = 0x11; /* 110,000 */
    selectEndingScreen(&nes);
    assert(nes.ram[ADDR_ENDING] == 3);
    assert(nes.ram[ADDR_ENDING_CUSTOM_VARS] == 0xBF);
    assert(nes.ram[ADDR_ENDING_CUSTOM_VARS + 1] == 0x00);

    /* Test 5: Score >= 120,000 -> ending 4 (cathedral rocket, applies PPU patch) */
    nes.ram[ADDR_P1_SCORE + 2] = 0x15; /* 150,000 */
    selectEndingScreen(&nes);
    assert(nes.ram[ADDR_ENDING] == 4);
    assert(nes.ram[ADDR_ENDING_CUSTOM_VARS] == 0xC7);
    assert(nes.ram[ADDR_ENDING_CUSTOM_VARS + 1] == 0x00);
    assert(nes.ram[ADDR_PATCH_TO_PPU] == (uint8_t)(0xA8CC & 0xFF));
    assert(nes.ram[ADDR_PATCH_TO_PPU + 1] == (uint8_t)(0xA8CC >> 8));

    printf("  [PASS] test_select_ending_screen\n");
}

void test_ending_type_a_ascent(void) {
    NES_State nes;
    nes_init(&nes);

    nes.ram[ADDR_ENDING] = 0;
    nes.ram[ADDR_ENDING_CUSTOM_VARS] = 0xBF;
    nes.ram[ADDR_ENDING_CUSTOM_VARS + 1] = 0x00;
    nes.ram[ADDR_FRAME_COUNTER] = 0x00;

    /* Calling ending_typeA should stage rocket and jet sprites, and increment delay counter */
    ending_typeA(&nes);
    assert(nes.ram[ADDR_SPRITE_Y_OFFSET] == 0xBF);
    assert(nes.ram[ADDR_SPRITE_X_OFFSET] == rocketToXOffsetTable[0]);
    assert(nes.ram[ADDR_ENDING_CUSTOM_VARS + 1] == 0x01);
    assert(nes.ram[ADDR_ENDING_CUSTOM_VARS] == 0xBF); /* Rocket has not moved yet */

    /* Advance delay counter to $F0 */
    nes.ram[ADDR_ENDING_CUSTOM_VARS + 1] = 0xEF;
    ending_typeA(&nes);
    assert(nes.ram[ADDR_ENDING_CUSTOM_VARS + 1] == 0xF0);

    /* Now at delay $F0, with frameCounter odd and Y >= $B0: no dec */
    nes.ram[ADDR_FRAME_COUNTER] = 0x01;
    ending_typeA(&nes);
    assert(nes.ram[ADDR_ENDING_CUSTOM_VARS] == 0xBF);

    /* With frameCounter even and Y >= $B0: decrements by 1 and triggers rocket SFX 0x03 */
    nes.ram[ADDR_FRAME_COUNTER] = 0x00;
    ending_typeA(&nes);
    assert(nes.ram[ADDR_ENDING_CUSTOM_VARS] == 0xBE);
    assert(nes.ram[ADDR_SOUND_EFFECT_SLOT0_INIT] == 0x03);

    /* When Y is between $80 and $AF: decrements every frame */
    nes.ram[ADDR_ENDING_CUSTOM_VARS] = 0xA0;
    nes.ram[ADDR_FRAME_COUNTER] = 0x01;
    ending_typeA(&nes);
    assert(nes.ram[ADDR_ENDING_CUSTOM_VARS] == 0x9F);

    /* When Y is below $80: accelerates with double decrement */
    nes.ram[ADDR_ENDING_CUSTOM_VARS] = 0x7E;
    ending_typeA(&nes);
    assert(nes.ram[ADDR_ENDING_CUSTOM_VARS] == 0x7C);

    /* When Y reaches 0: no-op */
    nes.ram[ADDR_ENDING_CUSTOM_VARS] = 0x00;
    ending_typeA(&nes);
    assert(nes.ram[ADDR_ENDING_CUSTOM_VARS] == 0x00);

    printf("  [PASS] test_ending_type_a_ascent\n");
}

void test_ending_init_type_b_vars(void) {
    NES_State nes;
    nes_init(&nes);

    /* Test Level 9 initialization */
    nes.ram[ADDR_LEVEL_NUMBER] = 9;
    nes.ram[ADDR_P1_START_HEIGHT] = 3;
    ending_initTypeBVars(&nes);
    assert(nes.ram[ADDR_ENDING] == 0);
    assert(nes.ram[ADDR_ENDING_CUSTOM_VARS] == 0);
    assert(nes.ram[ADDR_ENDING_CUSTOM_VARS + 2] == 0);
    assert(nes.ram[ADDR_ENDING_CUSTOM_VARS + 3] == 0xBC);
    assert(nes.ram[ADDR_ENDING_CUSTOM_VARS + 4] == 0xB8);
    assert(nes.ram[ADDR_ENDING_CUSTOM_VARS + 5] == 0xBC);
    assert(nes.ram[ADDR_ENDING_CUSTOM_VARS + 6] == 0xB3);
    assert(nes.ram[ADDR_SPRITE_INDEX_OAM] == 0x02);
    assert(nes.ram[ADDR_ENDING_CATHEDRAL_FRAME_DELAY_COUNTER] == 0);

    /* Test Non-Level 9 initialization (e.g. Level 0) */
    nes.ram[ADDR_LEVEL_NUMBER] = 0;
    ending_initTypeBVars(&nes);
    assert(nes.ram[ADDR_ENDING_CUSTOM_VARS + 1] == cathedralStartDeltaTable[0]);
    for (int i = 2; i <= 6; i++) {
        assert(nes.ram[ADDR_ENDING_CUSTOM_VARS + i] == cathedralEndTargetTable[0]);
    }

    printf("  [PASS] test_ending_init_type_b_vars\n");
}

void test_ending_type_b_concert(void) {
    NES_State nes;
    nes_init(&nes);

    nes.ram[ADDR_LEVEL_NUMBER] = 9;
    nes.ram[ADDR_P1_START_HEIGHT] = 5; /* Full orchestra: Mario, Luigi, Peach, Bowser, DK, Samus, Link, Pit */
    nes.ram[ADDR_ENDING] = 0;
    nes.ram[ADDR_ENDING_CUSTOM_VARS] = 0;
    nes.ram[ADDR_FRAME_COUNTER] = 0;

    /* Initialize dome heights */
    nes.ram[ADDR_ENDING_CUSTOM_VARS + 3] = 0xBC;
    nes.ram[ADDR_ENDING_CUSTOM_VARS + 4] = 0xB8;
    nes.ram[ADDR_ENDING_CUSTOM_VARS + 5] = 0xBC;
    nes.ram[ADDR_ENDING_CUSTOM_VARS + 6] = 0xB3;

    ending_typeBConcert(&nes);
    /* Ending counter advanced for Luigi */
    assert(nes.ram[ADDR_ENDING] > 0);
    /* Ending sprite index staged */
    assert(nes.ram[ADDR_SPRITE_INDEX_OAM] != 0);

    printf("  [PASS] test_ending_type_b_concert\n");
}

void test_ending_type_b_cathedral(void) {
    NES_State nes;
    nes_init(&nes);

    nes.ram[ADDR_LEVEL_NUMBER] = 0;
    nes.ram[ADDR_START_HEIGHT] = 2;
    nes.ram[ADDR_ENDING] = 0;
    nes.ram[ADDR_ENDING_CUSTOM_VARS] = 0;
    nes.ram[ADDR_ENDING_CUSTOM_VARS + 1] = 0x10; /* Not at target (target is 0x00) */
    nes.ram[ADDR_ENDING_CATHEDRAL_FRAME_DELAY_COUNTER] = 0;

    ending_typeBCathedral(&nes);
    assert(nes.ram[ADDR_ENDING_CATHEDRAL_FRAME_DELAY_COUNTER] == 1);
    assert(nes.ram[ADDR_ENDING] == 1);

    printf("  [PASS] test_ending_type_b_cathedral\n");
}

void test_render_ending_dispatchers(void) {
    NES_State nes;
    nes_init(&nes);

    /* Game Type A dispatches to ending_typeA */
    nes.ram[ADDR_GAME_TYPE] = GAMETYPE_A;
    nes.ram[ADDR_ENDING] = 0;
    nes.ram[ADDR_ENDING_CUSTOM_VARS] = 0xBF;
    nes.ram[ADDR_ENDING_CUSTOM_VARS + 1] = 0x00;
    render_ending(&nes);
    assert(nes.ram[ADDR_ENDING_CUSTOM_VARS + 1] == 1);

    /* Game Type B Level 9 dispatches to ending_typeBConcert */
    nes.ram[ADDR_GAME_TYPE] = GAMETYPE_B;
    nes.ram[ADDR_LEVEL_NUMBER] = 9;
    nes.ram[ADDR_P1_START_HEIGHT] = 0; /* Pit only */
    render_ending(&nes);

    /* Unskippable sleep loop decrements sleep counter down to 0 */
    render_endingUnskippable(&nes, 3);
    assert(nes.ram[ADDR_SLEEP_COUNTER] == 0);
    assert(nes.ram[ADDR_OAM_STAGING_LENGTH] == 0);

    printf("  [PASS] test_render_ending_dispatchers\n");
}

void test_ending_animation_b_bonus_countdown(void) {
    NES_State nes;
    nes_init(&nes);

    nes.ram[ADDR_GAME_TYPE] = GAMETYPE_B;
    nes.ram[ADDR_P1_LEVEL_NUMBER] = 9;
    nes.ram[ADDR_P1_START_HEIGHT] = 2;
    nes.ram[ADDR_P1_SCORE] = 0x50;
    nes.ram[ADDR_P1_SCORE + 1] = 0x23;
    nes.ram[ADDR_P1_SCORE + 2] = 0x01; /* 12,350 */
    nes.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START; /* Exit START wait immediately */

    endingAnimationB(&nes);

    /* Verify bonuses were counted down to zero */
    assert(nes.ram[ADDR_B_TYPE_LEVEL_BONUS] == 0);
    assert(nes.ram[ADDR_B_TYPE_HEIGHT_BONUS] == 0);
    /* Verify final score restored */
    assert(nes.ram[ADDR_SCORE] == nes.ram[ADDR_TOTAL_SCORE]);
    assert(nes.ram[ADDR_SCORE + 1] == nes.ram[ADDR_TOTAL_SCORE + 1]);
    assert(nes.ram[ADDR_SCORE + 2] == nes.ram[ADDR_TOTAL_SCORE + 2]);

    printf("  [PASS] test_ending_animation_b_bonus_countdown\n");
}

void run_ending_subsystem_tests(void) {
    printf("--- Testing Ending Sequences & Cutscene Animations Subsystem ---\n");
    test_ending_data_tables();
    test_select_ending_screen();
    test_ending_type_a_ascent();
    test_ending_init_type_b_vars();
    test_ending_type_b_concert();
    test_ending_type_b_cathedral();
    test_render_ending_dispatchers();
    test_ending_animation_b_bonus_countdown();
    printf("--- All Ending Subsystem Tests Passed ---\n\n");
}

int test_ending_subsystem(void) {
    run_ending_subsystem_tests();
    return 0;
}
