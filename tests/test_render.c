#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "game/render.h"
#include "game/highscore.h"
#include "constants/memory.h"
#include "constants/gameplay.h"
#include "constants/hardware.h"

int test_render_subsystem(void) {
    int failures = 0;
    printf("\n--- Testing Render and Sprite Staging Subsystem ---\n");

    /* Test 1: copyOamStagingToOam ($AB5E) */
    {
        NES_State nes;
        nes_init(&nes);
        copyOamStagingToOam(&nes);
        assert(nes.oam_addr == 0x00);
        printf("  [PASS] copyOamStagingToOam resets OAMADDR and triggers OAMDMA\n");
    }

    /* Test 2: copyCurrentScrollAndCtrlToPPU ($AA8A) */
    {
        NES_State nes;
        nes_init(&nes);
        nes.ram[ADDR_CURRENT_PPU_CTRL] = 0x98;
        copyCurrentScrollAndCtrlToPPU(&nes);
        assert(nes.ppu_ctrl == 0x98);
        printf("  [PASS] copyCurrentScrollAndCtrlToPPU flushes scroll and writes PPUCTRL\n");
    }

    /* Test 3: loadSpriteIntoOamStaging ($8C27) */
    {
        NES_State nes;
        nes_init(&nes);
        /* Test cursor sprite 0 (sprite00LevelSelectCursor): 4 tiles = 16 bytes */
        nes.ram[ADDR_SPRITE_INDEX_OAM] = 0x00;
        nes.ram[ADDR_SPRITE_X_OFFSET] = 0x20;
        nes.ram[ADDR_SPRITE_Y_OFFSET] = 0x30;
        nes.ram[ADDR_OAM_STAGING_LENGTH] = 0x00;

        loadSpriteIntoOamStaging(&nes);

        assert(nes.ram[ADDR_OAM_STAGING_LENGTH] == 16);
        assert(nes.ram[ADDR_GENERAL_COUNTER] == 0x20);
        assert(nes.ram[ADDR_GENERAL_COUNTER2] == 0x8D);
        /* Check first staged sprite tile */
        assert(nes.ram[ADDR_OAM_STAGING + 0] == (uint8_t)(0x00 + 0x30)); /* y */
        assert(nes.ram[ADDR_OAM_STAGING + 1] == 0xFC);                  /* tile */
        assert(nes.ram[ADDR_OAM_STAGING + 2] == 0x20);                  /* attr */
        assert(nes.ram[ADDR_OAM_STAGING + 3] == (uint8_t)(0x00 + 0x20)); /* x */

        /* Second tile in cursor */
        assert(nes.ram[ADDR_OAM_STAGING + 4] == (uint8_t)(0x00 + 0x30)); /* y */
        assert(nes.ram[ADDR_OAM_STAGING + 5] == 0xFC);                  /* tile */
        assert(nes.ram[ADDR_OAM_STAGING + 6] == 0x20);                  /* attr */
        assert(nes.ram[ADDR_OAM_STAGING + 7] == (uint8_t)(0x08 + 0x20)); /* x */

        printf("  [PASS] loadSpriteIntoOamStaging correctly stages table-driven sprite\n");
    }

    /* Test 4: updatePaletteForLevel ($9808) */
    {
        NES_State nes;
        nes_init(&nes);
        /* Level 9 */
        nes.ram[ADDR_P1_LEVEL_NUMBER] = 9;
        updatePaletteForLevel(&nes);
        assert(nes.ram[ADDR_GENERAL_COUNTER] == 0x20);

        /* Level 19 should wrap to 9 (level % 10) */
        nes.ram[ADDR_P1_LEVEL_NUMBER] = 19;
        updatePaletteForLevel(&nes);
        assert(nes.ram[ADDR_GENERAL_COUNTER] == 0x20);
        printf("  [PASS] updatePaletteForLevel modulo-10 palette update\n");
    }

    /* Test 5: render_mode_legal_and_title_screens ($82B1) */
    {
        NES_State nes;
        nes_init(&nes);
        nes.ram[ADDR_CURRENT_PPU_CTRL] = 0x9B;
        nes.ram[ADDR_PPU_SCROLL_X] = 10;
        nes.ram[ADDR_PPU_SCROLL_Y] = 20;

        render_mode_legal_and_title_screens(&nes);

        assert(nes.ram[ADDR_CURRENT_PPU_CTRL] == (0x9B & 0xFC));
        assert(nes.ram[ADDR_PPU_SCROLL_X] == 0);
        assert(nes.ram[ADDR_PPU_SCROLL_Y] == 0);
        printf("  [PASS] render_mode_legal_and_title_screens masks PPUCTRL and clears scroll\n");
    }

    /* Test 6: render_mode_menu_screens ($85DA) */
    {
        NES_State nes;
        nes_init(&nes);
        nes.ram[ADDR_CURRENT_PPU_CTRL] = 0x9B;
        nes.ram[ADDR_PPU_SCROLL_X] = 10;
        nes.ram[ADDR_PPU_SCROLL_Y] = 20;

        render_mode_menu_screens(&nes);

        assert(nes.ram[ADDR_CURRENT_PPU_CTRL] == (0x9B & 0xFC));
        assert(nes.ppu_ctrl == (0x9B & 0xFC));
        assert(nes.ram[ADDR_PPU_SCROLL_X] == 0);
        assert(nes.ram[ADDR_PPU_SCROLL_Y] == 0);
        printf("  [PASS] render_mode_menu_screens updates PPUCTRL and clears scroll\n");
    }

    /* Test 7: render_mode_ending_animation ($9F95) */
    {
        NES_State nes;
        nes_init(&nes);
        /* Type A */
        nes.ram[ADDR_GAME_TYPE] = GAMETYPE_A;
        nes.ram[ADDR_P1_SCORE] = 0x12;
        nes.ram[ADDR_P1_SCORE + 1] = 0x34;
        nes.ram[ADDR_P1_SCORE + 2] = 0x56;

        render_mode_ending_animation(&nes);
        assert(nes.ram[ADDR_GENERAL_COUNTER] == 0x12);

        /* Type B */
        nes.ram[ADDR_GAME_TYPE] = GAMETYPE_B;
        nes.ram[ADDR_GENERAL_COUNTER4] = 0x09;
        nes.ram[ADDR_GENERAL_COUNTER5] = 0x05;
        nes.ram[ADDR_TOTAL_SCORE] = 0x99;
        nes.ram[ADDR_TOTAL_SCORE + 1] = 0x88;
        nes.ram[ADDR_TOTAL_SCORE + 2] = 0x07;

        render_mode_ending_animation(&nes);
        assert(nes.ram[ADDR_GENERAL_COUNTER] == 0x99);
        printf("  [PASS] render_mode_ending_animation nametable score rendering\n");
    }

    /* Test 8: render ($804B) state dispatcher */
    {
        NES_State nes;
        nes_init(&nes);

        /* Mode 0: Legal and Title */
        nes.ram[ADDR_RENDER_MODE] = RENDER_MODE_LEGAL_AND_TITLE;
        nes.ram[ADDR_CURRENT_PPU_CTRL] = 0xFF;
        render(&nes);
        assert(nes.ram[ADDR_CURRENT_PPU_CTRL] == 0xFC);

        /* Mode 1: Menus */
        nes.ram[ADDR_RENDER_MODE] = RENDER_MODE_MENUS;
        nes.ram[ADDR_CURRENT_PPU_CTRL] = 0xFF;
        render(&nes);
        assert(nes.ppu_ctrl == 0xFC);

        /* Mode 4: Ending */
        nes.ram[ADDR_RENDER_MODE] = RENDER_MODE_ENDING;
        nes.ram[ADDR_GAME_TYPE] = GAMETYPE_A;
        nes.ram[ADDR_P1_SCORE] = 0x42;
        render(&nes);
        assert(nes.ram[ADDR_GENERAL_COUNTER] == 0x42);

        printf("  [PASS] render master dispatcher for modes 0..4\n");
    }

    return failures;
}
