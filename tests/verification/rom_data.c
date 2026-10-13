#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "verification/rom_data.h"
#include "game/tetrimino.h"
#include "math/rng.h"
#include "math/bcd.h"
#include "game/playfield.h"
#include "game/menu.h"
#include "game/highscore.h"
#include "game/state.h"
#include "game/render.h"

/* Addresses from tetris.lbl */
#define ADDR_ROM_GAME_TYPEB_NAMETABLE_PATCH 0x86B0
#define ADDR_ROM_LEVEL_TO_SPRITE_Y_OFFSET   0x85B2
#define ADDR_ROM_LEVEL_TO_SPRITE_X_OFFSET   0x85BC
#define ADDR_ROM_HEIGHT_TO_PPU_HIGH_ADDR    0x85C6
#define ADDR_ROM_HEIGHT_TO_PPU_LOW_ADDR     0x85CC
#define ADDR_ROM_MUSIC_SELECTION_TABLE      0x85D2
#define ADDR_ROM_ROTATION_TABLE             0x88EE
#define ADDR_ROM_FRAMES_PER_DROP_TABLE      0x898E
#define ADDR_ROM_ORIENTATION_TABLE          0x8A9C
#define ADDR_ROM_ORIENTATION_TO_SPRITE_TABLE 0x8BE5
#define ADDR_ROM_COLOR_TABLE                0x984C
#define ADDR_ROM_OAM_CONTENT_LOOKUP_SPRITES 0x8C6C
#define ADDR_ROM_TETRIMINO_TYPE_TABLE       0x993B
#define ADDR_ROM_SPAWN_TABLE                0x994E
#define ADDR_ROM_SPAWN_ORIENT_TABLE         0x9956
#define ADDR_ROM_POINTS_TABLE               0x9CA5
#define ADDR_ROM_DEMO_TETRIMINO_TABLE       0xDF00
#define ADDR_ROM_MULT_BY_10_TABLE           0x96D6
#define ADDR_ROM_LEFT_COLUMNS_TABLE         0x97FE
#define ADDR_ROM_RIGHT_COLUMNS_TABLE        0x9803
#define ADDR_ROM_TYPEB_BLANK_COUNT_TABLE    0x8876
#define ADDR_ROM_RNG_TABLE                  0x887C
#define ADDR_ROM_GARBAGE_LINES_TABLE        0x9B53
#define ADDR_ROM_HIGH_SCORE_PPU_ADDR_TABLE  0xA086
#define ADDR_ROM_HIGH_SCORE_CHAR_TO_TILE    0xA08C
#define ADDR_ROM_BYTE_TO_BCD_TABLE          0xA0BC
#define ADDR_ROM_HIGH_SCORE_NAMES_OFFSET    0xA1F1
#define ADDR_ROM_HIGH_SCORE_SCORES_OFFSET   0xA1F9
#define ADDR_ROM_HIGH_SCORE_POS_TO_Y        0xA33B
#define ADDR_ROM_HIGH_SCORE_NAME_POS_TO_X   0xA33E
#define ADDR_ROM_TYPEB_SUCCESS_GRAPHIC      0xA42E
#define ADDR_ROM_DEFAULT_HIGH_SCORES_TABLE  0xAD67

bool load_reference_rom(NES_State *nes, const char *custom_path) {
    if (!nes) return false;

    if (custom_path) {
        FILE *f = fopen(custom_path, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long sz = ftell(f);
            fseek(f, 0, SEEK_SET);
            if (sz >= 16 + NES_PRG_SIZE) {
                uint8_t *buf = (uint8_t *)malloc(sz);
                if (buf) {
                    size_t read_bytes = fread(buf, 1, sz, f);
                    fclose(f);
                    if (read_bytes == (size_t)sz) {
                        bool ok = nes_load_rom(nes, buf, sz);
                        free(buf);
                        if (ok) return true;
                    } else {
                        free(buf);
                    }
                }
            } else {
                fclose(f);
            }
        }
    }

    const char *paths[] = {
        "Tetris (USA).nes",
        "../Tetris (USA).nes",
        "../../Tetris (USA).nes",
        "/root/TDecomp/Tetris (USA).nes",
        "TetrisNESDisasm/tetris.nes",
        "../TetrisNESDisasm/tetris.nes",
        "../../TetrisNESDisasm/tetris.nes",
        "/root/TDecomp/TetrisNESDisasm/tetris.nes",
        NULL
    };

    for (int i = 0; paths[i] != NULL; i++) {
        if (!paths[i]) continue;
        FILE *f = fopen(paths[i], "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long sz = ftell(f);
            fseek(f, 0, SEEK_SET);
            if (sz >= 16 + NES_PRG_SIZE) {
                uint8_t *buf = (uint8_t *)malloc(sz);
                if (buf) {
                    size_t read_bytes = fread(buf, 1, sz, f);
                    fclose(f);
                    if (read_bytes == (size_t)sz) {
                        bool ok = nes_load_rom(nes, buf, sz);
                        free(buf);
                        if (ok) return true;
                    } else {
                        free(buf);
                    }
                }
            } else {
                fclose(f);
            }
        }
    }

    return false;
}

int test_rom_tables_match(const NES_State *nes) {
    assert(nes != NULL);

    /* Test 1: musicSelectionTable (8 bytes at $85D2) */
    for (int i = 0; i < 8; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_MUSIC_SELECTION_TABLE + i));
        assert(musicSelectionTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 2: rotationTable (38 bytes at $88EE) */
    for (int i = 0; i < 38; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_ROTATION_TABLE + i));
        assert(rotationTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 3: framesPerDropTable (29 bytes at $898E) */
    for (int i = 0; i < 29; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_FRAMES_PER_DROP_TABLE + i));
        assert(framesPerDropTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 4: orientationTable (228 bytes at $8A9C) */
    for (int i = 0; i < 228; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_ORIENTATION_TABLE + i));
        assert(orientationTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 5: tetriminoTypeFromOrientation (19 bytes at $993B) */
    for (int i = 0; i < 19; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_TETRIMINO_TYPE_TABLE + i));
        assert(tetriminoTypeFromOrientation[i] == rom_b);
        (void)rom_b;
    }

    /* Test 6: spawnTable (7 bytes at $994E) */
    for (int i = 0; i < 7; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_SPAWN_TABLE + i));
        assert(spawnTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 7: spawnOrientationFromOrientation (19 bytes at $9956) */
    for (int i = 0; i < 19; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_SPAWN_ORIENT_TABLE + i));
        assert(spawnOrientationFromOrientation[i] == rom_b);
        (void)rom_b;
    }

    /* Test 8: pointsTable (10 bytes / 5 words at $9CA5) */
    for (int i = 0; i < 5; i++) {
        uint16_t rom_w = nes_read16(nes, (uint16_t)(ADDR_ROM_POINTS_TABLE + i * 2));
        assert(pointsTable[i] == rom_w);
        (void)rom_w;
    }

    /* Test 9: demoTetriminoTypeTable (256 bytes at $DF00) */
    for (int i = 0; i < 256; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_DEMO_TETRIMINO_TABLE + i));
        assert(demoTetriminoTypeTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 10: multBy10Table (20 bytes at $96D6) */
    for (int i = 0; i < 20; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_MULT_BY_10_TABLE + i));
        assert(multBy10Table[i] == rom_b);
        (void)rom_b;
    }

    /* Test 11: leftColumnsTable (5 bytes at $97FE) */
    for (int i = 0; i < 5; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_LEFT_COLUMNS_TABLE + i));
        assert(leftColumnsTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 12: rightColumnsTable (5 bytes at $9803) */
    for (int i = 0; i < 5; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_RIGHT_COLUMNS_TABLE + i));
        assert(rightColumnsTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 13: typeBBlankInitCountByHeightTable (6 bytes at $8876) */
    for (int i = 0; i < 6; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_TYPEB_BLANK_COUNT_TABLE + i));
        assert(typeBBlankInitCountByHeightTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 14: rngTable (8 bytes at $887C) */
    for (int i = 0; i < 8; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_RNG_TABLE + i));
        assert(rngTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 15: garbageLinesTable (5 bytes at $9B53) */
    for (int i = 0; i < 5; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_GARBAGE_LINES_TABLE + i));
        assert(garbageLinesTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 16: levelToSpriteYOffset (10 bytes at $85B2) */
    for (int i = 0; i < 10; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_LEVEL_TO_SPRITE_Y_OFFSET + i));
        assert(levelToSpriteYOffset[i] == rom_b);
        (void)rom_b;
    }

    /* Test 17: levelToSpriteXOffset (10 bytes at $85BC) */
    for (int i = 0; i < 10; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_LEVEL_TO_SPRITE_X_OFFSET + i));
        assert(levelToSpriteXOffset[i] == rom_b);
        (void)rom_b;
    }

    /* Test 18: heightToPpuHighAddr (6 bytes at $85C6) */
    for (int i = 0; i < 6; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_HEIGHT_TO_PPU_HIGH_ADDR + i));
        assert(heightToPpuHighAddr[i] == rom_b);
        (void)rom_b;
    }

    /* Test 19: heightToPpuLowAddr (6 bytes at $85CC) */
    for (int i = 0; i < 6; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_HEIGHT_TO_PPU_LOW_ADDR + i));
        assert(heightToPpuLowAddr[i] == rom_b);
        (void)rom_b;
    }

    /* Test 20: typebSuccessGraphic (31 bytes at $A42E) */
    for (int i = 0; i < 31; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_TYPEB_SUCCESS_GRAPHIC + i));
        assert(typebSuccessGraphic[i] == rom_b);
        (void)rom_b;
    }

    /* Test 21: highScorePpuAddrTable (6 bytes at $A086) */
    for (int i = 0; i < 6; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_HIGH_SCORE_PPU_ADDR_TABLE + i));
        assert(highScorePpuAddrTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 22: highScoreCharToTile (44 bytes at $A08C) */
    for (int i = 0; i < 44; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_HIGH_SCORE_CHAR_TO_TILE + i));
        assert(highScoreCharToTile[i] == rom_b);
        (void)rom_b;
    }

    /* Test 23: byteToBcdTable (50 bytes at $A0BC) */
    for (int i = 0; i < 50; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_BYTE_TO_BCD_TABLE + i));
        assert(byteToBcdTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 24: highScoreIndexToHighScoreNamesOffset (8 bytes at $A1F1) */
    for (int i = 0; i < 8; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_HIGH_SCORE_NAMES_OFFSET + i));
        assert(highScoreIndexToHighScoreNamesOffset[i] == rom_b);
        (void)rom_b;
    }

    /* Test 25: highScoreIndexToHighScoreScoresOffset (8 bytes at $A1F9) */
    for (int i = 0; i < 8; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_HIGH_SCORE_SCORES_OFFSET + i));
        assert(highScoreIndexToHighScoreScoresOffset[i] == rom_b);
        (void)rom_b;
    }

    /* Test 26: highScorePosToY (3 bytes at $A33B) */
    for (int i = 0; i < 3; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_HIGH_SCORE_POS_TO_Y + i));
        assert(highScorePosToY[i] == rom_b);
        (void)rom_b;
    }

    /* Test 27: highScoreNamePosToX (6 bytes at $A33E) */
    for (int i = 0; i < 6; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_HIGH_SCORE_NAME_POS_TO_X + i));
        assert(highScoreNamePosToX[i] == rom_b);
        (void)rom_b;
    }

    /* Test 28: defaultHighScoresTable (81 bytes at $AD67) */
    for (int i = 0; i < 81; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_DEFAULT_HIGH_SCORES_TABLE + i));
        assert(defaultHighScoresTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 29: game_typeb_nametable_patch (44 bytes at $86B0) */
    for (int i = 0; i < 44; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_GAME_TYPEB_NAMETABLE_PATCH + i));
        assert(game_typeb_nametable_patch[i] == rom_b);
        (void)rom_b;
    }

    /* Test 30: orientationToSpriteTable (19 bytes at $8BE5) */
    for (int i = 0; i < 19; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_ORIENTATION_TO_SPRITE_TABLE + i));
        assert(orientationToSpriteTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 31: colorTable (40 bytes at $984C) */
    for (int i = 0; i < 40; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_COLOR_TABLE + i));
        assert(colorTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 32: oamContentLookupAndSprites (2008 bytes at $8C6C) */
    for (int i = 0; i < 2008; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_OAM_CONTENT_LOOKUP_SPRITES + i));
        assert(oamContentLookupAndSprites[i] == rom_b);
        (void)rom_b;
    }

    printf("  [PASS] test_rom_tables_match (32 ROM data tables verified bit-for-bit)\n");
    return 0;
}
