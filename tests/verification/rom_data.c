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
#include "game/ending.h"

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
#define ADDR_ROM_PIECE_TO_PPU_STAT_ADDR     0x96AA
#define ADDR_ROM_LEVEL_DISPLAY_TABLE        0x96B8
#define ADDR_ROM_VRAM_PLAYFIELD_ROWS        0x96EA
#define ADDR_ROM_DOME_LIFTOFF_DELAY_TABLE   0xA735
#define ADDR_ROM_DOME_NUMBER_TO_X_OFFSET    0xA739
#define ADDR_ROM_DOME_START_HEIGHTS         0xA73D
#define ADDR_ROM_DOME_NUMBER_TO_SPRITE      0xA741
#define ADDR_ROM_DOME_FLAME_X_OFFSET        0xA745
#define ADDR_ROM_ENDING_CATHEDRAL_ANIM_SPD  0xA749
#define ADDR_ROM_ENDING_CATHEDRAL_FRAME_DLY 0xA753
#define ADDR_ROM_CATHEDRAL_START_DELTA      0xA75D
#define ADDR_ROM_CATHEDRAL_END_TARGET       0xA767
#define ADDR_ROM_ENDING_CATHEDRAL_VECTOR    0xA771
#define ADDR_ROM_ENDING_CATHEDRAL_X_TABLE   0xA77B
#define ADDR_ROM_ENDING_CATHEDRAL_Y_TABLE   0xA7B7
#define ADDR_ROM_ENDING_CATHEDRAL_SPRITE    0xA7F3
#define ADDR_ROM_MARIO_FRAME_TO_Y_OFFSET    0xA80A
#define ADDR_ROM_LUIGI_FRAME_TO_Y_OFFSET    0xA80E
#define ADDR_ROM_LUIGI_FRAME_TO_SPRITE      0xA818
#define ADDR_ROM_PATCH_PPU_HEIGHT_UNUSED    0xA822
#define ADDR_ROM_PATCH_PPU_HEIGHT_0         0xA834
#define ADDR_ROM_PATCH_PPU_HEIGHT_1         0xA84A
#define ADDR_ROM_PATCH_PPU_HEIGHT_2         0xA862
#define ADDR_ROM_PATCH_PPU_HEIGHT_3         0xA87A
#define ADDR_ROM_PATCH_PPU_HEIGHT_4         0xA896
#define ADDR_ROM_PATCH_PPU_TYPE_A_OVER_120K 0xA8CC
#define ADDR_ROM_ROCKET_TO_SPRITE           0xAA11
#define ADDR_ROM_ROCKET_TO_JET_SPRITE       0xAA16
#define ADDR_ROM_ROCKET_TO_JET_X_OFFSET     0xAA20
#define ADDR_ROM_ROCKET_TO_X_OFFSET         0xAA25
#define ADDR_ROM_ROCKET_START_Y             0xAA2A

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

    /* Test 33: pieceToPpuStatAddr (14 bytes at $96AA) */
    for (int i = 0; i < 14; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_PIECE_TO_PPU_STAT_ADDR + i));
        assert(pieceToPpuStatAddr[i] == rom_b);
        (void)rom_b;
    }

    /* Test 34: levelDisplayTable (30 bytes at $96B8) */
    for (int i = 0; i < 30; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_LEVEL_DISPLAY_TABLE + i));
        assert(levelDisplayTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 35: vramPlayfieldRows (40 bytes at $96EA) */
    for (int i = 0; i < 40; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_VRAM_PLAYFIELD_ROWS + i));
        assert(vramPlayfieldRows[i] == rom_b);
        (void)rom_b;
    }

    /* Test 36: domeLiftoffDelayTable (4 bytes at $A735) */
    for (int i = 0; i < 4; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_DOME_LIFTOFF_DELAY_TABLE + i));
        assert(domeLiftoffDelayTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 37: domeNumberToXOffsetTable (4 bytes at $A739) */
    for (int i = 0; i < 4; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_DOME_NUMBER_TO_X_OFFSET + i));
        assert(domeNumberToXOffsetTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 38: domeStartHeightsTable (4 bytes at $A73D) */
    for (int i = 0; i < 4; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_DOME_START_HEIGHTS + i));
        assert(domeStartHeightsTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 39: domeNumberToSpriteTable (4 bytes at $A741) */
    for (int i = 0; i < 4; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_DOME_NUMBER_TO_SPRITE + i));
        assert(domeNumberToSpriteTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 40: domeFlameXOffsetTable (4 bytes at $A745) */
    for (int i = 0; i < 4; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_DOME_FLAME_X_OFFSET + i));
        assert(domeFlameXOffsetTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 41: ending_typeBCathedralAnimSpeed (10 bytes at $A749) */
    for (int i = 0; i < 10; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_ENDING_CATHEDRAL_ANIM_SPD + i));
        assert(ending_typeBCathedralAnimSpeed[i] == rom_b);
        (void)rom_b;
    }

    /* Test 42: ending_typeBCathedralFrameDelayTable (10 bytes at $A753) */
    for (int i = 0; i < 10; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_ENDING_CATHEDRAL_FRAME_DLY + i));
        assert(ending_typeBCathedralFrameDelayTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 43: cathedralStartDeltaTable (10 bytes at $A75D) */
    for (int i = 0; i < 10; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_CATHEDRAL_START_DELTA + i));
        assert(cathedralStartDeltaTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 44: cathedralEndTargetTable (10 bytes at $A767) */
    for (int i = 0; i < 10; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_CATHEDRAL_END_TARGET + i));
        assert(cathedralEndTargetTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 45: ending_typeBCathedralVectorTable (10 bytes at $A771) */
    for (int i = 0; i < 10; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_ENDING_CATHEDRAL_VECTOR + i));
        assert(ending_typeBCathedralVectorTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 46: ending_typeBCathedralXTable (60 bytes at $A77B) */
    for (int i = 0; i < 60; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_ENDING_CATHEDRAL_X_TABLE + i));
        assert(ending_typeBCathedralXTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 47: ending_typeBCathedralYTable (60 bytes at $A7B7) */
    for (int i = 0; i < 60; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_ENDING_CATHEDRAL_Y_TABLE + i));
        assert(ending_typeBCathedralYTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 48: ending_typeBCathedralSpriteTable (10 bytes at $A7F3) */
    for (int i = 0; i < 10; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_ENDING_CATHEDRAL_SPRITE + i));
        assert(ending_typeBCathedralSpriteTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 49: marioFrameToYOffsetTable (4 bytes at $A80A) */
    for (int i = 0; i < 4; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_MARIO_FRAME_TO_Y_OFFSET + i));
        assert(marioFrameToYOffsetTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 50: luigiFrameToYOffsetTable (10 bytes at $A80E) */
    for (int i = 0; i < 10; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_LUIGI_FRAME_TO_Y_OFFSET + i));
        assert(luigiFrameToYOffsetTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 51: luigiFrameToSpriteTable (10 bytes at $A818) */
    for (int i = 0; i < 10; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_LUIGI_FRAME_TO_SPRITE + i));
        assert(luigiFrameToSpriteTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 52: ending_patchToPpu_typeBConcertHeightUnused (18 bytes at $A822) */
    for (int i = 0; i < 18; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_PATCH_PPU_HEIGHT_UNUSED + i));
        assert(ending_patchToPpu_typeBConcertHeightUnused[i] == rom_b);
        (void)rom_b;
    }

    /* Test 53: ending_patchToPpu_typeBConcertHeight0 (22 bytes at $A834) */
    for (int i = 0; i < 22; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_PATCH_PPU_HEIGHT_0 + i));
        assert(ending_patchToPpu_typeBConcertHeight0[i] == rom_b);
        (void)rom_b;
    }

    /* Test 54: ending_patchToPpu_typeBConcertHeight1 (24 bytes at $A84A) */
    for (int i = 0; i < 24; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_PATCH_PPU_HEIGHT_1 + i));
        assert(ending_patchToPpu_typeBConcertHeight1[i] == rom_b);
        (void)rom_b;
    }

    /* Test 55: ending_patchToPpu_typeBConcertHeight2 (24 bytes at $A862) */
    for (int i = 0; i < 24; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_PATCH_PPU_HEIGHT_2 + i));
        assert(ending_patchToPpu_typeBConcertHeight2[i] == rom_b);
        (void)rom_b;
    }

    /* Test 56: ending_patchToPpu_typeBConcertHeight3 (28 bytes at $A87A) */
    for (int i = 0; i < 28; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_PATCH_PPU_HEIGHT_3 + i));
        assert(ending_patchToPpu_typeBConcertHeight3[i] == rom_b);
        (void)rom_b;
    }

    /* Test 57: ending_patchToPpu_typeBConcertHeight4 (18 bytes at $A896) */
    for (int i = 0; i < 18; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_PATCH_PPU_HEIGHT_4 + i));
        assert(ending_patchToPpu_typeBConcertHeight4[i] == rom_b);
        (void)rom_b;
    }

    /* Test 58: ending_patchToPpu_typeAOver120k (89 bytes at $A8CC) */
    for (int i = 0; i < 89; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_PATCH_PPU_TYPE_A_OVER_120K + i));
        assert(ending_patchToPpu_typeAOver120k[i] == rom_b);
        (void)rom_b;
    }

    /* Test 59: rocketToSpriteTable (5 bytes at $AA11) */
    for (int i = 0; i < 5; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_ROCKET_TO_SPRITE + i));
        assert(rocketToSpriteTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 60: rocketToJetSpriteTable (10 bytes at $AA16) */
    for (int i = 0; i < 10; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_ROCKET_TO_JET_SPRITE + i));
        assert(rocketToJetSpriteTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 61: rocketToJetXOffsetTable (5 bytes at $AA20) */
    for (int i = 0; i < 5; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_ROCKET_TO_JET_X_OFFSET + i));
        assert(rocketToJetXOffsetTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 62: rocketToXOffsetTable (5 bytes at $AA25) */
    for (int i = 0; i < 5; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_ROCKET_TO_X_OFFSET + i));
        assert(rocketToXOffsetTable[i] == rom_b);
        (void)rom_b;
    }

    /* Test 63: rocketStartYTable (5 bytes at $AA2A) */
    for (int i = 0; i < 5; i++) {
        uint8_t rom_b = nes_read8(nes, (uint16_t)(ADDR_ROM_ROCKET_START_Y + i));
        assert(rocketStartYTable[i] == rom_b);
        (void)rom_b;
    }

    printf("  [PASS] test_rom_tables_match (63 ROM data tables verified bit-for-bit)\n");
    return 0;
}
