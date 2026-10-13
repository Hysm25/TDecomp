#include "game/highscore.h"
#include "constants/memory.h"
#include "constants/hardware.h"
#include "constants/gameplay.h"
#include <string.h>

/*
 * Authoritative High Score Data Tables
 */

/*
 * PPU Base Addresses for High Score Table Rows ($A086, 6 bytes: 3 rows * 2 bytes big-endian)
 * Row 0: $2289, Row 1: $22C9, Row 2: $2309
 */
const uint8_t highScorePpuAddrTable[6] = {
    0x22, 0x89, 0x22, 0xC9, 0x23, 0x09
};

/*
 * High Score Character Code to Nametable Tile ID Lookup ($A08C, 44 bytes)
 */
const uint8_t highScoreCharToTile[44] = {
    0x24, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
    0x21, 0x22, 0x23, 0x00, 0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08, 0x09, 0x25, 0x4F, 0x5E,
    0x5F, 0x6E, 0x6F, 0xFF
};

/*
 * Byte (0..49) to BCD representation lookup table ($A0BC, 50 bytes)
 */
const uint8_t byteToBcdTable[50] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
    0x16, 0x17, 0x18, 0x19, 0x20, 0x21, 0x22, 0x23,
    0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x30, 0x31,
    0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
    0x48, 0x49
};

/*
 * High Score Entry Index to highScoreNames RAM Offset Lookup ($A1F1, 8 bytes)
 * Offsets: $00, $06, $0C, $12, $18, $1E, $24, $2A
 */
const uint8_t highScoreIndexToHighScoreNamesOffset[8] = {
    0x00, 0x06, 0x0C, 0x12, 0x18, 0x1E, 0x24, 0x2A
};

/*
 * High Score Entry Index to highScoreScores RAM Offset Lookup ($A1F9, 8 bytes)
 * Offsets: $00, $03, $06, $09, $0C, $0F, $12, $15
 */
const uint8_t highScoreIndexToHighScoreScoresOffset[8] = {
    0x00, 0x03, 0x06, 0x09, 0x0C, 0x0F, 0x12, 0x15
};

/*
 * High Score Cursor Row to Sprite Y Coordinate Lookup ($A33B, 3 bytes)
 * Rows 0..2: $9F, $AF, $BF
 */
const uint8_t highScorePosToY[3] = {
    0x9F, 0xAF, 0xBF
};

/*
 * High Score Cursor Letter to Sprite X Coordinate Lookup ($A33E, 6 bytes)
 * Letters 0..5: $48, $50, $58, $60, $68, $70
 */
const uint8_t highScoreNamePosToX[6] = {
    0x48, 0x50, 0x58, 0x60, 0x68, 0x70
};

/*
 * Default High Score Table initialized on cold boot ($AD67, 81 bytes)
 */
const uint8_t defaultHighScoresTable[81] = {
    /* Names (48 bytes: 8 entries * 6 chars) */
    0x08, 0x0F, 0x17, 0x01, 0x12, 0x04, /* HOWARD */
    0x0F, 0x14, 0x01, 0x13, 0x01, 0x0E, /* OTASAN */
    0x0C, 0x01, 0x0E, 0x03, 0x05, 0x2B, /* LANCE_ */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* unused 4th */
    0x01, 0x0C, 0x05, 0x18, 0x2B, 0x2B, /* ALEX__ */
    0x14, 0x0F, 0x0E, 0x19, 0x2B, 0x2B, /* TONY__ */
    0x0E, 0x09, 0x0E, 0x14, 0x05, 0x0E, /* NINTEN */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* unused 4th */

    /* Scores (24 bytes: 8 entries * 3 bytes BCD) */
    0x01, 0x00, 0x00,                   /* Type A 1st: 10,000 */
    0x00, 0x75, 0x00,                   /* Type A 2nd: 7,500 */
    0x00, 0x50, 0x00,                   /* Type A 3rd: 5,000 */
    0x00, 0x00, 0x00,                   /* unused 4th */
    0x00, 0x20, 0x00,                   /* Type B 1st: 2,000 */
    0x00, 0x10, 0x00,                   /* Type B 2nd: 1,000 */
    0x00, 0x05, 0x00,                   /* Type B 3rd: 500 */
    0x00, 0x00, 0x00,                   /* unused 4th */

    /* Levels (8 bytes: 8 entries * 1 byte) */
    0x09,                               /* Type A 1st: 9 */
    0x05,                               /* Type A 2nd: 5 */
    0x00,                               /* Type A 3rd: 0 */
    0x00,                               /* unused 4th */
    0x09,                               /* Type B 1st: 9 */
    0x05,                               /* Type B 2nd: 5 */
    0x00,                               /* Type B 3rd: 0 */
    0x00,                               /* unused 4th */

    /* Terminator */
    0xFF
};

/*
 * Check if the NES RAM holds valid warm boot magic numbers ($0750 == $12, $34, $56, $78, $9A).
 */
bool isWarmBoot(const NES_State *nes) {
    if (!nes) return false;
    return (nes->ram[ADDR_INIT_MAGIC]     == 0x12 &&
            nes->ram[ADDR_INIT_MAGIC + 1] == 0x34 &&
            nes->ram[ADDR_INIT_MAGIC + 2] == 0x56 &&
            nes->ram[ADDR_INIT_MAGIC + 3] == 0x78 &&
            nes->ram[ADDR_INIT_MAGIC + 4] == 0x9A);
}

/*
 * Cold Boot High Score Table Initialization:
 * Source address: $8095 (main.asm lines 326-346).
 */
void initHighScoreTable(NES_State *nes) {
    if (!nes) return;

    for (uint16_t i = 0; i < sizeof(defaultHighScoresTable); i++) {
        uint8_t val = defaultHighScoresTable[i];
        if (val == 0xFF) break;
        nes->ram[ADDR_HIGH_SCORE_NAMES + i] = val;
    }

    nes->ram[ADDR_INIT_MAGIC]     = 0x12;
    nes->ram[ADDR_INIT_MAGIC + 1] = 0x34;
    nes->ram[ADDR_INIT_MAGIC + 2] = 0x56;
    nes->ram[ADDR_INIT_MAGIC + 3] = 0x78;
    nes->ram[ADDR_INIT_MAGIC + 4] = 0x9A;
}

/*
 * Outputs two BCD digits (high nibble then low nibble) to PPUDATA ($2007).
 * Sets generalCounter ($00A8) to val.
 * Source address: $9712 (main.asm lines 2691-2703).
 */
void twoDigsToPPU(NES_State *nes, uint8_t val) {
    if (!nes) return;
    nes->ram[ADDR_GENERAL_COUNTER] = val;
    nes_write8(nes, REG_PPUDATA, (uint8_t)((val & 0xF0) >> 4));
    nes_write8(nes, REG_PPUDATA, (uint8_t)(val & 0x0F));
}

/*
 * Copies a 6-byte player name from entry at startByte to the next entry index.
 * Adjusts for gameType (startByte + 0x18 if Type B).
 * Source address: $A192 (main.asm lines 4208-4237).
 */
void copyHighScoreNameToNextIndex(NES_State *nes, uint8_t startByte) {
    if (!nes) return;

    uint8_t offset = startByte;
    if (nes->ram[ADDR_GAME_TYPE] != 0) {
        offset = (uint8_t)(offset + 0x18);
    }
    nes->ram[ADDR_GENERAL_COUNTER] = offset;

    for (int8_t i = 5; i >= 0; i--) {
        nes->ram[ADDR_GENERAL_COUNTER2] = (uint8_t)i;
        uint8_t src_idx = (uint8_t)(offset + (uint8_t)i);
        uint8_t val = nes->ram[ADDR_HIGH_SCORE_NAMES + src_idx];
        nes->ram[ADDR_GENERAL_COUNTER3] = val;
        nes->ram[ADDR_HIGH_SCORE_NAMES + src_idx + 6] = val;
    }

    nes->ram[ADDR_GENERAL_COUNTER2] = 0xFF;
}

/*
 * Copies a 3-byte BCD score from entry at startByte to the next entry index.
 * Adjusts for gameType (startByte + 0x0C if Type B).
 * Source address: $A1C1 (main.asm lines 4239-4257).
 */
void copyHighScoreScoreToNextIndex(NES_State *nes, uint8_t startByte) {
    if (!nes) return;

    uint8_t x = startByte;
    if (nes->ram[ADDR_GAME_TYPE] != 0) {
        x = (uint8_t)(x + 0x0C);
    }

    nes->ram[ADDR_HIGH_SCORE_A + x + 3] = nes->ram[ADDR_HIGH_SCORE_A + x];
    x++;
    nes->ram[ADDR_HIGH_SCORE_A + x + 3] = nes->ram[ADDR_HIGH_SCORE_A + x];
    x++;
    nes->ram[ADDR_HIGH_SCORE_A + x + 3] = nes->ram[ADDR_HIGH_SCORE_A + x];
}

/*
 * Copies a 1-byte level number from entry at startByte to the next entry index.
 * Adjusts for gameType (startByte + 0x04 if Type B).
 * Source address: $A1E0 (main.asm lines 4259-4271).
 */
void copyHighScoreLevelToNextIndex(NES_State *nes, uint8_t startByte) {
    if (!nes) return;

    uint8_t x = startByte;
    if (nes->ram[ADDR_GAME_TYPE] != 0) {
        x = (uint8_t)(x + 0x04);
    }

    nes->ram[ADDR_HIGH_SCORE_LEVELS + x + 1] = nes->ram[ADDR_HIGH_SCORE_LEVELS + x];
}

/*
 * Adjusts high score table positions by shifting lower entries down,
 * clears the 6-character name slot for the new high score, writes the player's
 * score and level into the table, and initiates the high score entry screen.
 * Source address: $A134 (main.asm lines 4160-4206).
 */
void adjustHighScores(NES_State *nes) {
    if (!nes) return;

    uint8_t rawPos = nes->ram[ADDR_HIGH_SCORE_RAW_POS];
    uint8_t pos = rawPos & 0x03;

    if (pos < 2) {
        copyHighScoreNameToNextIndex(nes, 0x06);
        copyHighScoreScoreToNextIndex(nes, 0x03);
        copyHighScoreLevelToNextIndex(nes, 0x01);

        if (pos == 0) {
            copyHighScoreNameToNextIndex(nes, 0x00);
            copyHighScoreScoreToNextIndex(nes, 0x00);
            copyHighScoreLevelToNextIndex(nes, 0x00);
        }
    }

    uint8_t nameOffset = highScoreIndexToHighScoreNamesOffset[rawPos & 7];
    memset(&nes->ram[ADDR_HIGH_SCORE_NAMES + nameOffset], 0, 6);

    uint8_t scoreOffset = highScoreIndexToHighScoreScoresOffset[rawPos & 7];
    nes->ram[ADDR_HIGH_SCORE_A + scoreOffset]     = nes->ram[ADDR_P1_SCORE + 2];
    nes->ram[ADDR_HIGH_SCORE_A + scoreOffset + 1] = nes->ram[ADDR_P1_SCORE + 1];
    nes->ram[ADDR_HIGH_SCORE_A + scoreOffset + 2] = nes->ram[ADDR_P1_SCORE];

    nes->ram[ADDR_HIGH_SCORE_LEVELS + (rawPos & 7)] = nes->ram[ADDR_P1_LEVEL_NUMBER];

    highScoreEntryScreen(nes);
}

/*
 * Checks if Player 1's score qualifies for the high score table for the current
 * gameType (Type A: entries 0..2, Type B: entries 4..6). Breaks ties in favor
 * of the new score. If qualified, sets highScoreEntryRawPos and invokes adjustHighScores.
 * Source address: $A0EE (main.asm lines 4116-4159).
 */
void handleHighScoreIfNecessary(NES_State *nes) {
    if (!nes) return;

    uint8_t rawPos = (nes->ram[ADDR_GAME_TYPE] == 0) ? 0 : 4;
    nes->ram[ADDR_HIGH_SCORE_RAW_POS] = rawPos;

    while (true) {
        nes->ram[ADDR_GENERAL_COUNTER2] = rawPos;
        uint8_t y = (uint8_t)(rawPos * 3);

        uint8_t score_hi  = nes->ram[ADDR_HIGH_SCORE_A + y];
        uint8_t player_hi = nes->ram[ADDR_P1_SCORE + 2];

        if (score_hi == player_hi) {
            uint8_t score_mid  = nes->ram[ADDR_HIGH_SCORE_A + y + 1];
            uint8_t player_mid = nes->ram[ADDR_P1_SCORE + 1];

            if (score_mid == player_mid) {
                uint8_t score_lo  = nes->ram[ADDR_HIGH_SCORE_A + y + 2];
                uint8_t player_lo = nes->ram[ADDR_P1_SCORE];

                if (player_lo >= score_lo) {
                    adjustHighScores(nes);
                    return;
                }
            } else if (player_mid > score_mid) {
                adjustHighScores(nes);
                return;
            }
        } else if (player_hi > score_hi) {
            adjustHighScores(nes);
            return;
        }

        /* Score is too small for current slot */
        rawPos++;
        nes->ram[ADDR_HIGH_SCORE_RAW_POS] = rawPos;
        if (rawPos == 3 || rawPos == 7) {
            return;
        }
    }
}

/*
 * Return stub for high score display routine.
 * Source address: $A085 (main.asm lines 4093-4094).
 */
void showHighScores_ret(NES_State *nes) {
    (void)nes;
}

/*
 * Real high score display routine rendering 3 entries for the active gameType
 * (names, 6-digit BCD scores, and 2-digit BCD levels) to PPU nametable.
 * Source address: $9FFB (main.asm lines 4021-4092).
 */
void showHighScores_real(NES_State *nes) {
    if (!nes) return;

    uint8_t gc2 = (nes->ram[ADDR_GAME_TYPE] == 0) ? 0 : 4;
    nes->ram[ADDR_GENERAL_COUNTER2] = gc2;

    while (true) {
        uint8_t entryIdx = nes->ram[ADDR_GENERAL_COUNTER2] & 0x03;
        uint8_t addr_hi = highScorePpuAddrTable[entryIdx * 2];
        uint8_t addr_lo = highScorePpuAddrTable[entryIdx * 2 + 1];
        nes_write8(nes, REG_PPUADDR, addr_hi);
        nes_write8(nes, REG_PPUADDR, addr_lo);

        uint8_t nameOffset = (uint8_t)(nes->ram[ADDR_GENERAL_COUNTER2] * 6);
        nes->ram[ADDR_GENERAL_COUNTER] = nameOffset;
        for (uint8_t i = 0; i < 6; i++) {
            uint8_t ch = nes->ram[ADDR_HIGH_SCORE_NAMES + nameOffset + i];
            uint8_t tile = (ch < sizeof(highScoreCharToTile)) ? highScoreCharToTile[ch] : 0xFF;
            nes_write8(nes, REG_PPUDATA, tile);
        }
        nes_write8(nes, REG_PPUDATA, 0xFF);

        uint8_t scoreOffset = (uint8_t)(nes->ram[ADDR_GENERAL_COUNTER2] * 3);
        twoDigsToPPU(nes, nes->ram[ADDR_HIGH_SCORE_A + scoreOffset]);
        twoDigsToPPU(nes, nes->ram[ADDR_HIGH_SCORE_A + scoreOffset + 1]);
        twoDigsToPPU(nes, nes->ram[ADDR_HIGH_SCORE_A + scoreOffset + 2]);
        nes_write8(nes, REG_PPUDATA, 0xFF);

        uint8_t lvl = nes->ram[ADDR_HIGH_SCORE_LEVELS + nes->ram[ADDR_GENERAL_COUNTER2]];
        uint8_t bcd_lvl = (lvl < sizeof(byteToBcdTable)) ? byteToBcdTable[lvl] : 0;
        twoDigsToPPU(nes, bcd_lvl);

        nes->ram[ADDR_GENERAL_COUNTER2]++;
        if (nes->ram[ADDR_GENERAL_COUNTER2] == 3 || nes->ram[ADDR_GENERAL_COUNTER2] == 7) {
            break;
        }
    }
}

/*
 * Dispatches high score display if numberOfPlayers == 1, otherwise returns.
 * Source address: $9FF2 (main.asm lines 4015-4020).
 */
void showHighScores(NES_State *nes) {
    if (!nes) return;
    if (nes->ram[ADDR_NUMBER_OF_PLAYERS] == 1) {
        showHighScores_real(nes);
    } else {
        showHighScores_ret(nes);
    }
}

/*
 * Single-frame step handler for the high score entry screen.
 * Handles cursor horizontal navigation (A / Right / B / Left),
 * letter cycling (Down / Up with 44-character alphabet wrap),
 * and confirmation (Start button).
 * Returns true if name entry is finished (Start pressed), false if ongoing.
 */
bool highScoreEntryScreen_step(NES_State *nes) {
    if (!nes) return true;

    nes->ram[ADDR_OAM_STAGING] = 0;
    uint8_t letterIdx = nes->ram[ADDR_HIGH_SCORE_NAME_OFFSET_LETTER];
    if (letterIdx >= 6) letterIdx = 0;
    nes->ram[ADDR_SPRITE_X_OFFSET] = highScoreNamePosToX[letterIdx];
    nes->ram[ADDR_SPRITE_INDEX_OAM] = ((nes->ram[ADDR_FRAME_COUNTER] & MENU_CURSOR_MASK) != 0) ? 0x0E : 0x02;

    uint8_t newlyPressed = nes->ram[ADDR_NEWLY_PRESSED_P1];

    if (newlyPressed & BUTTON_START) {
        nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = SFX_MENU_SELECT;
        return true; /* Finished name entry */
    }

    if (newlyPressed & (BUTTON_A | BUTTON_RIGHT)) {
        nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = SFX_MENU_MOVE;
        nes->ram[ADDR_HIGH_SCORE_NAME_OFFSET_LETTER]++;
        if (nes->ram[ADDR_HIGH_SCORE_NAME_OFFSET_LETTER] >= 6) {
            nes->ram[ADDR_HIGH_SCORE_NAME_OFFSET_LETTER] = 0;
        }
    }

    if (newlyPressed & (BUTTON_B | BUTTON_LEFT)) {
        nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = SFX_MENU_MOVE;
        if (nes->ram[ADDR_HIGH_SCORE_NAME_OFFSET_LETTER] == 0) {
            nes->ram[ADDR_HIGH_SCORE_NAME_OFFSET_LETTER] = 5;
        } else {
            nes->ram[ADDR_HIGH_SCORE_NAME_OFFSET_LETTER]--;
        }
    }

    uint8_t held = nes->ram[ADDR_HELD_BUTTONS_P1];
    uint8_t rowOffset = nes->ram[ADDR_HIGH_SCORE_NAME_OFFSET_ROW];
    letterIdx = nes->ram[ADDR_HIGH_SCORE_NAME_OFFSET_LETTER];
    uint8_t charIdx = (uint8_t)(rowOffset + letterIdx);

    if ((held & BUTTON_DOWN) && ((nes->ram[ADDR_FRAME_COUNTER] & 0x07) == 0)) {
        nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = SFX_MENU_MOVE;
        int8_t letter = (int8_t)(nes->ram[ADDR_HIGH_SCORE_NAMES + charIdx] - 1);
        if (letter < 0) {
            letter = (int8_t)(letter + 0x2C);
        }
        nes->ram[ADDR_HIGH_SCORE_NAMES + charIdx] = (uint8_t)letter;
    }

    if ((held & BUTTON_UP) && ((nes->ram[ADDR_FRAME_COUNTER] & 0x07) == 0)) {
        nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = SFX_MENU_MOVE;
        uint8_t letter = (uint8_t)(nes->ram[ADDR_HIGH_SCORE_NAMES + charIdx] + 1);
        if (letter >= 0x2C) {
            letter = (uint8_t)(letter - 0x2C);
        }
        nes->ram[ADDR_HIGH_SCORE_NAMES + charIdx] = letter;
    }

    nes->ram[ADDR_HIGH_SCORE_CURRENT_LETTER] = nes->ram[ADDR_HIGH_SCORE_NAMES + charIdx];
    nes->ram[ADDR_OUT_OF_DATE_RENDER_FLAGS] = RENDER_HIGH_SCORE_LETTER;

    return false; /* Still in name entry */
}

/*
 * Interactive high score entry screen: sets music track 9, renderMode 2,
 * initializes cursor and row offsets, and displays high score screen.
 * Source address: $A201 (main.asm lines 4276-4438).
 */
void highScoreEntryScreen(NES_State *nes) {
    if (!nes) return;

    nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = 0;
    nes->ram[ADDR_MUSIC_TRACK] = 0x09;
    nes->ram[ADDR_RENDER_MODE] = 2;

    nes_write8(nes, REG_PPUADDR, 0x20);
    nes_write8(nes, REG_PPUADDR, 0x6D);
    nes_write8(nes, REG_PPUDATA, (uint8_t)(0x0A + nes->ram[ADDR_GAME_TYPE]));

    showHighScores(nes);
    nes->ram[ADDR_RENDER_MODE] = 2;

    uint8_t rawPos = nes->ram[ADDR_HIGH_SCORE_RAW_POS];
    nes->ram[ADDR_HIGH_SCORE_NAME_OFFSET_ROW] = (uint8_t)(rawPos * 6);
    nes->ram[ADDR_HIGH_SCORE_NAME_OFFSET_LETTER] = 0;
    nes->ram[ADDR_OAM_STAGING] = 0;

    uint8_t pos = rawPos & 0x03;
    if (pos < 3) {
        nes->ram[ADDR_SPRITE_Y_OFFSET] = highScorePosToY[pos];
    }

    highScoreEntryScreen_step(nes);
}

/*
 * Renders the updated high score letter to PPU nametable during VBlank (renderMode 2).
 * Writes the tile at the cursor PPU address, resets PPU scroll to (0, 0),
 * and clears outOfDateRenderFlags.
 * Source address: $A344 (main.asm lines 4443-4473).
 */
void render_mode_congratulations_screen(NES_State *nes) {
    if (!nes) return;

    if (!(nes->ram[ADDR_OUT_OF_DATE_RENDER_FLAGS] & RENDER_HIGH_SCORE_LETTER)) {
        return;
    }

    uint8_t pos = nes->ram[ADDR_HIGH_SCORE_RAW_POS] & 0x03;
    uint8_t addr_hi = highScorePpuAddrTable[pos * 2];
    uint8_t addr_lo_base = highScorePpuAddrTable[pos * 2 + 1];
    nes->ram[ADDR_GENERAL_COUNTER] = addr_lo_base;
    uint8_t addr_lo = (uint8_t)(addr_lo_base + nes->ram[ADDR_HIGH_SCORE_NAME_OFFSET_LETTER]);

    nes_write8(nes, REG_PPUADDR, addr_hi);
    nes_write8(nes, REG_PPUADDR, addr_lo);

    uint8_t currentLetter = nes->ram[ADDR_HIGH_SCORE_CURRENT_LETTER];
    uint8_t tile = (currentLetter < sizeof(highScoreCharToTile)) ? highScoreCharToTile[currentLetter] : 0xFF;
    nes_write8(nes, REG_PPUDATA, tile);

    nes->ram[ADDR_PPU_SCROLL_X] = 0;
    nes_write8(nes, REG_PPUSCROLL, 0);
    nes->ram[ADDR_PPU_SCROLL_Y] = 0;
    nes_write8(nes, REG_PPUSCROLL, 0);

    nes->ram[ADDR_OUT_OF_DATE_RENDER_FLAGS] = 0;
}
