#include "game/ending.h"
#include "game/render.h"
#include "math/bcd.h"

/*
 * Authoritative Ending ROM Data Tables
 */

/* Dome liftoff delay counter match table (4 bytes at $A735) */
const uint8_t domeLiftoffDelayTable[4] = {
    0x05, 0x07, 0x09, 0x0B
};

/* Dome X coordinate offsets (4 bytes at $A739) */
const uint8_t domeNumberToXOffsetTable[4] = {
    0x60, 0x90, 0x70, 0x7E
};

/* Dome starting Y heights (4 bytes at $A73D) */
const uint8_t domeStartHeightsTable[4] = {
    0xBC, 0xB8, 0xBC, 0xB3
};

/* Dome sprite IDs (4 bytes at $A741) */
const uint8_t domeNumberToSpriteTable[4] = {
    0x4D, 0x50, 0x4E, 0x4F
};

/* Dome thruster flame X offset table (4 bytes at $A745) */
const uint8_t domeFlameXOffsetTable[4] = {
    0x00, 0x00, 0x00, 0x02
};

/* Type B Cathedral sprite animation speeds per level 0..9 (10 bytes at $A749) */
const uint8_t ending_typeBCathedralAnimSpeed[10] = {
    0x02, 0x04, 0x06, 0x03, 0x10, 0x03, 0x05, 0x06, 0x02, 0x05
};

/* Type B Cathedral frame delay before movement per level 0..9 (10 bytes at $A753) */
const uint8_t ending_typeBCathedralFrameDelayTable[10] = {
    0x03, 0x01, 0x01, 0x01, 0x02, 0x05, 0x01, 0x02, 0x01, 0x01
};

/* Type B Cathedral initial X delta per level 0..9 (10 bytes at $A75D) */
const uint8_t cathedralStartDeltaTable[10] = {
    0x02, 0x02, 0xFE, 0xFE, 0x02, 0xFE, 0x02, 0x02, 0xFE, 0x02
};

/* Type B Cathedral ending X target per level 0..9 (10 bytes at $A767) */
const uint8_t cathedralEndTargetTable[10] = {
    0x00, 0x00, 0x00, 0x02, 0xF0, 0x10, 0xF0, 0xF0, 0x20, 0xF0
};

/* Type B Cathedral movement vector step per level 0..9 (10 bytes at $A771) */
const uint8_t ending_typeBCathedralVectorTable[10] = {
    0x01, 0x01, 0xFF, 0xFC, 0x01, 0xFF, 0x02, 0x02, 0xFE, 0x02
};

/* Type B Cathedral sprite trigger X coordinates (60 bytes at $A77B: 10 levels * 6 sprites) */
const uint8_t ending_typeBCathedralXTable[60] = {
    0x3A, 0x24, 0x0A, 0x4A, 0x3A, 0xFF,
    0x22, 0x44, 0x12, 0x32, 0x4A, 0xFF,
    0xAE, 0x6E, 0x8E, 0x6E, 0x1E, 0x02,
    0x42, 0x42, 0x42, 0x42, 0x42, 0x02,
    0x22, 0x0A, 0x1A, 0x04, 0x0A, 0xFF,
    0xEE, 0xDE, 0xFC, 0xFC, 0xF6, 0x02,
    0x80, 0x80, 0x80, 0x80, 0x80, 0xFF,
    0xE8, 0xE8, 0xE8, 0xE8, 0x48, 0xFF,
    0x80, 0xAE, 0x9E, 0x90, 0x80, 0x02,
    0x80, 0x80, 0x80, 0x80, 0x80, 0xFF
};

/* Type B Cathedral sprite Y positions (60 bytes at $A7B7: 10 levels * 6 sprites) */
const uint8_t ending_typeBCathedralYTable[60] = {
    0x98, 0xA8, 0xC0, 0xA8, 0x90, 0xB0,
    0xB0, 0xB8, 0xA0, 0xB8, 0xA8, 0xA0,
    0xC8, 0xC8, 0xC8, 0xC8, 0xC8, 0xC8,
    0x30, 0x20, 0x40, 0x28, 0xA0, 0x80,
    0xA8, 0x88, 0x68, 0xA8, 0x48, 0x78,
    0x58, 0x68, 0x18, 0x48, 0x78, 0x38,
    0xC8, 0xC8, 0xC8, 0xC8, 0xC8, 0xC8,
    0x90, 0x58, 0x70, 0xA8, 0x40, 0x38,
    0x68, 0x88, 0x78, 0x18, 0x48, 0xA8,
    0xC8, 0xC8, 0xC8, 0xC8, 0xC8, 0xC8
};

/* Type B Cathedral base sprite IDs per level 0..9 (10 bytes at $A7F3) */
const uint8_t ending_typeBCathedralSpriteTable[10] = {
    0x2C, 0x2E, 0x54, 0x32, 0x34, 0x36, 0x4B, 0x38, 0x3A, 0x4B
};

/* Mario frame to Y offset table (4 bytes at $A80A) */
const uint8_t marioFrameToYOffsetTable[4] = {
    0x97, 0x8F, 0x87, 0x8F
};

/* Luigi frame to Y offset table (10 bytes at $A80E) */
const uint8_t luigiFrameToYOffsetTable[10] = {
    0x97, 0x8F, 0x87, 0x87, 0x8F, 0x97, 0x8F, 0x87, 0x87, 0x8F
};

/* Luigi frame to sprite table (10 bytes at $A818) */
const uint8_t luigiFrameToSpriteTable[10] = {
    0x29, 0x29, 0x29, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x29, 0x29
};

/* PPU nametable patches for Type B Concert character masking */
const uint8_t ending_patchToPpu_typeBConcertHeightUnused[18] = {
    0x21, 0xA5, 0xFF, 0xFF, 0xFF, 0xFE,
    0x21, 0xC5, 0xFF, 0xFF, 0xFF, 0xFE,
    0x21, 0xE5, 0xFF, 0xFF, 0xFF, 0xFD
};

const uint8_t ending_patchToPpu_typeBConcertHeight0[22] = {
    0x23, 0x1A, 0xFF, 0xFE,
    0x23, 0x39, 0xFF, 0xFF, 0xFF, 0xFE,
    0x23, 0x59, 0xFF, 0xFF, 0xFF, 0xFE,
    0x23, 0x79, 0xFF, 0xFF, 0xFF, 0xFD
};

const uint8_t ending_patchToPpu_typeBConcertHeight1[24] = {
    0x23, 0x15, 0xFF, 0xFF, 0xFF, 0xFE,
    0x23, 0x35, 0xFF, 0xFF, 0xFF, 0xFE,
    0x23, 0x55, 0xFF, 0xFF, 0xFF, 0xFE,
    0x23, 0x75, 0xFF, 0xFF, 0xFF, 0xFD
};

const uint8_t ending_patchToPpu_typeBConcertHeight2[24] = {
    0x21, 0x88, 0xFF, 0xFF, 0xFF, 0xFE,
    0x21, 0xA8, 0xFF, 0xFF, 0xFF, 0xFE,
    0x21, 0xC8, 0xFF, 0xFF, 0xFF, 0xFE,
    0x21, 0xE8, 0xFF, 0xFF, 0xFF, 0xFD
};

const uint8_t ending_patchToPpu_typeBConcertHeight3[28] = {
    0x22, 0x46, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
    0x22, 0x66, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
    0x22, 0x86, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
    0x22, 0xA6, 0xFF, 0xFF, 0xFF, 0xFF, 0xFD
};

const uint8_t ending_patchToPpu_typeBConcertHeight4[18] = {
    0x20, 0xF9, 0xFF, 0xFF, 0xFF, 0xFE,
    0x21, 0x19, 0xFF, 0xFF, 0xFF, 0xFE,
    0x21, 0x39, 0xFF, 0xFF, 0xFF, 0xFD
};

/* PPU nametable patch for Type A 120,000+ score cathedral launch (89 bytes at $A8CC) */
const uint8_t ending_patchToPpu_typeAOver120k[89] = {
    0x22, 0x58, 0xFF, 0xFE,
    0x22, 0x75, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
    0x22, 0x94, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
    0x22, 0xB4, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
    0x22, 0xD4, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
    0x22, 0xF4, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
    0x23, 0x14, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
    0x23, 0x34, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
    0x22, 0xCA, 0x46, 0x47, 0xFE,
    0x22, 0xEA, 0x56, 0x57, 0xFD
};

/* Rocket body sprite ID table per ending tier 0..4 (5 bytes at $AA11) */
const uint8_t rocketToSpriteTable[5] = {
    0x3E, 0x41, 0x44, 0x47, 0x4A
};

/* Rocket thruster flame sprite table per ending tier 0..4 and frame 0..1 (10 bytes at $AA16) */
const uint8_t rocketToJetSpriteTable[10] = {
    0x3F, 0x40, 0x42, 0x43, 0x45, 0x46, 0x48, 0x49, 0x23, 0x24
};

/* Rocket thruster flame X offset table per ending tier 0..4 (5 bytes at $AA20) */
const uint8_t rocketToJetXOffsetTable[5] = {
    0x00, 0x00, 0x00, 0x00, 0x00
};

/* Rocket body X coordinate offset table per ending tier 0..4 (5 bytes at $AA25) */
const uint8_t rocketToXOffsetTable[5] = {
    0x54, 0x54, 0x50, 0x48, 0xA0
};

/* Rocket initial Y position table per ending tier 0..4 (5 bytes at $AA2A) */
const uint8_t rocketStartYTable[5] = {
    0xBF, 0xBF, 0xBF, 0xBF, 0xC7
};

static const uint8_t *get_patch_table(uint16_t addr) {
    switch (addr) {
        case 0xA822: return ending_patchToPpu_typeBConcertHeightUnused;
        case 0xA834: return ending_patchToPpu_typeBConcertHeight0;
        case 0xA84A: return ending_patchToPpu_typeBConcertHeight1;
        case 0xA862: return ending_patchToPpu_typeBConcertHeight2;
        case 0xA87A: return ending_patchToPpu_typeBConcertHeight3;
        case 0xA896: return ending_patchToPpu_typeBConcertHeight4;
        case 0xA8CC: return ending_patchToPpu_typeAOver120k;
        default: return NULL;
    }
}

/*
 * Reads patch data from patchToPpuAddr ($14) and streams byte sequences to PPU.
 * Format: PPUADDR hi, lo, bytes..., $FE (next address), $FD (terminate).
 * Source address: $A507 (main.asm lines 4713-4732).
 */
void patchToPpu(NES_State *nes) {
    if (!nes) return;

    uint16_t addr = (uint16_t)(nes->ram[ADDR_PATCH_TO_PPU] | (nes->ram[ADDR_PATCH_TO_PPU + 1] << 8));
    const uint8_t *table = NULL;
    if (nes_read8(nes, addr) == 0) {
        table = get_patch_table(addr);
    }

    uint8_t y = 0;
    while (y < 255) {
        uint8_t hi = table ? table[y++] : nes_read8(nes, (uint16_t)(addr + y++));
        uint8_t lo = table ? table[y++] : nes_read8(nes, (uint16_t)(addr + y++));
        nes_write8(nes, REG_PPUADDR, hi);
        nes_write8(nes, REG_PPUADDR, lo);

        while (y < 255) {
            uint8_t val = table ? table[y++] : nes_read8(nes, (uint16_t)(addr + y++));
            if (val == 0xFE) {
                break;
            }
            if (val == 0xFD) {
                return;
            }
            nes_write8(nes, REG_PPUDATA, val);
        }
    }
}

/*
 * Dispatches PPU nametable masking patches for Type B Concert based on ending ($C4),
 * falling through across unearned height tiers to erase inactive characters.
 * Source address: $A4B1 (main.asm lines 4664-4712).
 */
void ending_typeBConcertPatchToPpuForHeight(NES_State *nes) {
    if (!nes) return;

    static const uint16_t concertPatchAddrs[6] = {
        0xA822, /* ending_patchToPpu_typeBConcertHeightUnused */
        0xA834, /* ending_patchToPpu_typeBConcertHeight0 */
        0xA84A, /* ending_patchToPpu_typeBConcertHeight1 */
        0xA862, /* ending_patchToPpu_typeBConcertHeight2 */
        0xA87A, /* ending_patchToPpu_typeBConcertHeight3 */
        0xA896  /* ending_patchToPpu_typeBConcertHeight4 */
    };

    uint8_t ending_val = nes->ram[ADDR_ENDING];
    if (ending_val > 5) {
        return;
    }

    for (int i = (int)ending_val; i < 6; i++) {
        uint16_t patch_addr = concertPatchAddrs[i];
        nes->ram[ADDR_PATCH_TO_PPU] = (uint8_t)(patch_addr & 0xFF);
        nes->ram[ADDR_PATCH_TO_PPU + 1] = (uint8_t)(patch_addr >> 8);
        patchToPpu(nes);
    }
}

/*
 * Alternate entry point for selectEndingScreen after tier is determined ($A9A5).
 * Sets ending_customVars = rocketStartYTable[ending] and ending_customVars+1 = 0.
 * Source address: $A9A5 (main.asm lines 5226-5232).
 */
void ending_selected(NES_State *nes) {
    if (!nes) return;

    uint8_t ending = nes->ram[ADDR_ENDING];
    if (ending > 4) ending = 4;

    nes->ram[ADDR_ENDING_CUSTOM_VARS] = rocketStartYTable[ending];
    nes->ram[ADDR_ENDING_CUSTOM_VARS + 1] = 0x00;
}

/*
 * Selects Type A ending tier (0..4) based on Player 1 score high byte ($75):
 *   < 50k: tier 0 (small rocket)
 *   < 70k: tier 1 (medium rocket)
 *   < 100k: tier 2 (large rocket)
 *   < 120k: tier 3 (huge rocket)
 *   >= 120k: tier 4 (cathedral launch + PPU nametable patch).
 * Initializes rocket starting Y coordinate and delay counter.
 * Source address: $A96E (main.asm lines 5197-5225).
 */
void selectEndingScreen(NES_State *nes) {
    if (!nes) return;

    nes->ram[ADDR_ENDING] = 0;
    uint8_t hi_score = nes->ram[ADDR_P1_SCORE + 2];

    if (hi_score < 0x05) {
        ending_selected(nes);
        return;
    }

    nes->ram[ADDR_ENDING] = 1;
    if (hi_score < 0x07) {
        ending_selected(nes);
        return;
    }

    nes->ram[ADDR_ENDING] = 2;
    if (hi_score < 0x10) {
        ending_selected(nes);
        return;
    }

    nes->ram[ADDR_ENDING] = 3;
    if (hi_score < 0x12) {
        ending_selected(nes);
        return;
    }

    nes->ram[ADDR_ENDING] = 4;
    nes->ram[ADDR_PATCH_TO_PPU] = (uint8_t)(0xA8CC & 0xFF);
    nes->ram[ADDR_PATCH_TO_PPU + 1] = (uint8_t)(0xA8CC >> 8);
    patchToPpu(nes);
    ending_selected(nes);
}

/*
 * Stages Type A rocket and thruster sprites into OAM staging buffer,
 * handles 240-frame launch delay, and accelerates upward ascent physics.
 * Source address: $A9B1 (main.asm lines 5234-5282).
 */
void ending_typeA(NES_State *nes) {
    if (!nes) return;

    uint8_t y_pos = nes->ram[ADDR_ENDING_CUSTOM_VARS];
    if (y_pos == 0) {
        return;
    }

    uint8_t ending = nes->ram[ADDR_ENDING];
    if (ending > 4) ending = 4;

    /* Stage rocket body */
    nes->ram[ADDR_SPRITE_Y_OFFSET] = y_pos;
    nes->ram[ADDR_SPRITE_X_OFFSET] = rocketToXOffsetTable[ending];
    nes->ram[ADDR_SPRITE_INDEX_OAM] = rocketToSpriteTable[ending];
    loadSpriteIntoOamStaging(nes);

    /* Stage thruster jet flame */
    nes->ram[ADDR_GENERAL_COUNTER] = (uint8_t)(ending << 1);
    uint8_t jet_idx = (uint8_t)(((nes->ram[ADDR_FRAME_COUNTER] & 0x02) >> 1) + nes->ram[ADDR_GENERAL_COUNTER]);
    nes->ram[ADDR_SPRITE_INDEX_OAM] = rocketToJetSpriteTable[jet_idx];
    nes->ram[ADDR_SPRITE_X_OFFSET] += rocketToJetXOffsetTable[ending];
    loadSpriteIntoOamStaging(nes);

    /* Frame delay before launch (wait for counter to reach $F0) */
    if (nes->ram[ADDR_ENDING_CUSTOM_VARS + 1] != 0xF0) {
        nes->ram[ADDR_ENDING_CUSTOM_VARS + 1]++;
        return;
    }

    /* Ascent speed control */
    if (nes->ram[ADDR_ENDING_CUSTOM_VARS] >= 0xB0) {
        if (nes->ram[ADDR_FRAME_COUNTER] & 0x01) {
            return;
        }
    }

    nes->ram[ADDR_SOUND_EFFECT_SLOT0_INIT] = 0x03;
    nes->ram[ADDR_ENDING_CUSTOM_VARS]--;
    if (nes->ram[ADDR_ENDING_CUSTOM_VARS] < 0x80) {
        nes->ram[ADDR_ENDING_CUSTOM_VARS]--;
    }
}

/*
 * Initializes Type B ending variables: clears ending, ending_customVars,
 * and cathedral delay counter; initializes dome liftoff heights for level 9
 * or cathedral sprite positions for levels 0..8.
 * Source address: $A463 (main.asm lines 4623-4650).
 */
void ending_initTypeBVars(NES_State *nes) {
    if (!nes) return;

    nes->ram[ADDR_ENDING] = 0x00;
    nes->ram[ADDR_ENDING_CUSTOM_VARS] = 0x00;
    nes->ram[ADDR_ENDING_CATHEDRAL_FRAME_DELAY_COUNTER] = 0x00;
    nes->ram[ADDR_SPRITE_INDEX_OAM] = 0x02;

    uint8_t level = nes->ram[ADDR_LEVEL_NUMBER];
    if (level == 9) {
        nes->ram[ADDR_ENDING] = (uint8_t)(nes->ram[ADDR_P1_START_HEIGHT] + 1);
        ending_typeBConcertPatchToPpuForHeight(nes);
        nes->ram[ADDR_ENDING] = 0x00;
        nes->ram[ADDR_ENDING_CUSTOM_VARS + 2] = 0x00;
        nes->ram[ADDR_ENDING_CUSTOM_VARS + 3] = domeStartHeightsTable[0];
        nes->ram[ADDR_ENDING_CUSTOM_VARS + 4] = domeStartHeightsTable[1];
        nes->ram[ADDR_ENDING_CUSTOM_VARS + 5] = domeStartHeightsTable[2];
        nes->ram[ADDR_ENDING_CUSTOM_VARS + 6] = domeStartHeightsTable[3];
    } else {
        uint8_t end_target = cathedralEndTargetTable[level % 10];
        for (int i = 2; i <= 6; i++) {
            nes->ram[ADDR_ENDING_CUSTOM_VARS + i] = end_target;
        }
        nes->ram[ADDR_ENDING_CUSTOM_VARS + 1] = cathedralStartDeltaTable[level % 10];
    }
}

/*
 * Calculates dome liftoff acceleration, plays sound effect, updates dome heights,
 * and stages dome and flame sprites for Type B Level 9 concert finale.
 * Source address: $A6BC (main.asm lines 4992-5050).
 */
void LA6BC(NES_State *nes) {
    if (!nes) return;

    /* Check liftoff trigger for domes */
    for (int x = 0; x < 4; x++) {
        if (domeLiftoffDelayTable[x] == nes->ram[ADDR_ENDING_CUSTOM_VARS]) {
            if (nes->ram[ADDR_ENDING_CUSTOM_VARS + 3 + x] != 0) {
                nes->ram[ADDR_ENDING_CUSTOM_VARS + 3 + x]--;
                nes->ram[ADDR_ENDING_CUSTOM_VARS]++;
            }
        }
    }

    /* Stage dome sprites and flames */
    nes->ram[ADDR_ENDING_CURRENT_SPRITE] = 0;
    for (int sprite = 0; sprite < 4; sprite++) {
        nes->ram[ADDR_ENDING_CURRENT_SPRITE] = (uint8_t)sprite;
        uint8_t cur_height = nes->ram[ADDR_ENDING_CUSTOM_VARS + 3 + sprite];
        if (cur_height == 0) {
            continue;
        }

        uint8_t gen = cur_height;
        if (domeStartHeightsTable[sprite] != gen) {
            nes->ram[ADDR_SOUND_EFFECT_SLOT0_INIT] = 0x03;
            gen--;
            if (gen < 0xA0) {
                gen--;
            }
        }

        nes->ram[ADDR_ENDING_CUSTOM_VARS + 3 + sprite] = gen;
        nes->ram[ADDR_SPRITE_Y_OFFSET] = gen;
        nes->ram[ADDR_SPRITE_X_OFFSET] = domeNumberToXOffsetTable[sprite];
        nes->ram[ADDR_SPRITE_INDEX_OAM] = domeNumberToSpriteTable[sprite];
        loadSpriteIntoOamStaging(nes);

        /* Re-check if dome moved to stage rocket flame */
        if (domeStartHeightsTable[sprite] != nes->ram[ADDR_ENDING_CUSTOM_VARS + 3 + sprite]) {
            nes->ram[ADDR_SPRITE_X_OFFSET] += domeFlameXOffsetTable[sprite];
            nes->ram[ADDR_SPRITE_INDEX_OAM] = (uint8_t)(((nes->ram[ADDR_FRAME_COUNTER] & 0x02) >> 1) + 0x51);
            loadSpriteIntoOamStaging(nes);
        }
    }
}

/*
 * Stages guest Nintendo character sprites (Mario, Luigi, Peach, Bowser, Donkey Kong,
 * Samus, Link, Pit) according to player1_startHeight with cascading fallthrough,
 * and executes dome liftoff staging via LA6BC.
 * Source address: $A53B (main.asm lines 4750-4906).
 */
void ending_typeBConcert(NES_State *nes) {
    if (!nes) return;

    uint8_t height = nes->ram[ADDR_P1_START_HEIGHT];

    switch (height) {
        default:
        case 5:
            /* Peach */
            nes->ram[ADDR_SPRITE_X_OFFSET] = 0xC8;
            nes->ram[ADDR_SPRITE_Y_OFFSET] = 0x47;
            nes->ram[ADDR_SPRITE_INDEX_OAM] = (uint8_t)(((nes->ram[ADDR_FRAME_COUNTER] & 0x08) >> 3) + 0x21);
            loadSpriteIntoOamStaging(nes);

            /* Mario */
            nes->ram[ADDR_SPRITE_X_OFFSET] = 0xA0;
            nes->ram[ADDR_SPRITE_INDEX_OAM] = 0x27;
            uint8_t mario_idx = (uint8_t)((nes->ram[ADDR_FRAME_COUNTER] & 0x18) >> 3);
            nes->ram[ADDR_SPRITE_Y_OFFSET] = marioFrameToYOffsetTable[mario_idx];
            if (nes->ram[ADDR_SPRITE_Y_OFFSET] != 0x97) {
                nes->ram[ADDR_SPRITE_INDEX_OAM] = 0x28;
            }
            loadSpriteIntoOamStaging(nes);

            /* Luigi */
            while (1) {
                nes->ram[ADDR_SPRITE_X_OFFSET] = 0xC0;
                uint8_t luigi_idx = (uint8_t)(nes->ram[ADDR_ENDING] >> 3);
                if (luigi_idx != 0x0A) {
                    nes->ram[ADDR_SPRITE_Y_OFFSET] = luigiFrameToYOffsetTable[luigi_idx];
                    nes->ram[ADDR_SPRITE_INDEX_OAM] = luigiFrameToSpriteTable[luigi_idx];
                    loadSpriteIntoOamStaging(nes);
                    nes->ram[ADDR_ENDING]++;
                    break;
                }
                nes->ram[ADDR_ENDING] = 0x00;
                nes->ram[ADDR_ENDING_CUSTOM_VARS]++;
            }
            /* fallthrough */

        case 4:
            /* Bowser */
            nes->ram[ADDR_SPRITE_X_OFFSET] = 0x30;
            nes->ram[ADDR_SPRITE_Y_OFFSET] = 0xA7;
            nes->ram[ADDR_SPRITE_INDEX_OAM] = (uint8_t)(((nes->ram[ADDR_FRAME_COUNTER] & 0x10) >> 4) + 0x1F);
            loadSpriteIntoOamStaging(nes);
            /* fallthrough */

        case 3:
            /* Donkey Kong */
            nes->ram[ADDR_SPRITE_X_OFFSET] = 0x40;
            nes->ram[ADDR_SPRITE_Y_OFFSET] = 0x77;
            nes->ram[ADDR_SPRITE_INDEX_OAM] = (uint8_t)(((nes->ram[ADDR_FRAME_COUNTER] & 0x10) >> 4) + 0x1D);
            loadSpriteIntoOamStaging(nes);
            /* fallthrough */

        case 2:
            /* Samus */
            nes->ram[ADDR_SPRITE_X_OFFSET] = 0xA8;
            nes->ram[ADDR_SPRITE_Y_OFFSET] = 0xD7;
            nes->ram[ADDR_SPRITE_INDEX_OAM] = (uint8_t)(((nes->ram[ADDR_FRAME_COUNTER] & 0x10) >> 4) + 0x1A);
            loadSpriteIntoOamStaging(nes);
            /* fallthrough */

        case 1:
            /* Link */
            nes->ram[ADDR_SPRITE_X_OFFSET] = 0xC8;
            nes->ram[ADDR_SPRITE_Y_OFFSET] = 0xD7;
            nes->ram[ADDR_SPRITE_INDEX_OAM] = (uint8_t)(((nes->ram[ADDR_FRAME_COUNTER] & 0x10) >> 4) + 0x18);
            loadSpriteIntoOamStaging(nes);
            /* fallthrough */

        case 0:
            /* Pit (Kid Icarus) */
            nes->ram[ADDR_SPRITE_X_OFFSET] = 0x28;
            nes->ram[ADDR_SPRITE_Y_OFFSET] = 0x77;
            nes->ram[ADDR_SPRITE_INDEX_OAM] = (uint8_t)(((nes->ram[ADDR_FRAME_COUNTER] & 0x10) >> 4) + 0x16);
            loadSpriteIntoOamStaging(nes);
            LA6BC(nes);
            break;
    }
}

/*
 * Alternates cathedral character animation sprite index based on speed table.
 * Source address: $A690 (main.asm lines 4961-4978).
 */
void ending_typeBCathedralSetSprite(NES_State *nes) {
    if (!nes) return;

    nes->ram[ADDR_ENDING]++;
    uint8_t level = (uint8_t)(nes->ram[ADDR_LEVEL_NUMBER] % 10);

    if (ending_typeBCathedralAnimSpeed[level] == nes->ram[ADDR_ENDING]) {
        nes->ram[ADDR_ENDING_CUSTOM_VARS] ^= 0x01;
        nes->ram[ADDR_ENDING] = 0x00;
    }

    nes->ram[ADDR_SPRITE_INDEX_OAM] = (uint8_t)(ending_typeBCathedralSpriteTable[level] + nes->ram[ADDR_ENDING_CUSTOM_VARS]);
}

/*
 * Computes table index: levelNumber * 6 + ending_currentSprite.
 * Also sets generalCounter ($A8) = levelNumber * 2.
 * Source address: $A6AE (main.asm lines 4979-4991).
 */
uint8_t ending_computeTypeBCathedralYTableIndex(NES_State *nes) {
    if (!nes) return 0;

    uint8_t level = (uint8_t)(nes->ram[ADDR_LEVEL_NUMBER] % 10);
    nes->ram[ADDR_GENERAL_COUNTER] = (uint8_t)(level << 1);
    return (uint8_t)(level * 6 + nes->ram[ADDR_ENDING_CURRENT_SPRITE]);
}

/*
 * Animates Type B cathedral characters for levels 0..8 across startHeight sprites.
 * Source address: $A625 (main.asm lines 4907-4960).
 */
void ending_typeBCathedral(NES_State *nes) {
    if (!nes) return;

    ending_typeBCathedralSetSprite(nes);
    nes->ram[ADDR_ENDING_CATHEDRAL_FRAME_DELAY_COUNTER]++;
    nes->ram[ADDR_ENDING_CURRENT_SPRITE] = 0;

    uint8_t level = (uint8_t)(nes->ram[ADDR_LEVEL_NUMBER] % 10);
    uint8_t end_target = cathedralEndTargetTable[level];

    while (1) {
        nes->ram[ADDR_GENERAL_COUNTER] = end_target;
        uint8_t cur_sprite = nes->ram[ADDR_ENDING_CURRENT_SPRITE];
        uint8_t cur_val = nes->ram[ADDR_ENDING_CUSTOM_VARS + 1 + cur_sprite];

        if (cur_val != end_target) {
            nes->ram[ADDR_SPRITE_X_OFFSET] = cur_val;
            uint8_t table_idx = ending_computeTypeBCathedralYTableIndex(nes);
            nes->ram[ADDR_SPRITE_Y_OFFSET] = ending_typeBCathedralYTable[table_idx];
            loadSpriteIntoOamStaging(nes);

            if (ending_typeBCathedralFrameDelayTable[level] == nes->ram[ADDR_ENDING_CATHEDRAL_FRAME_DELAY_COUNTER]) {
                uint8_t new_x = (uint8_t)(ending_typeBCathedralVectorTable[level] + nes->ram[ADDR_SPRITE_X_OFFSET]);
                nes->ram[ADDR_SPRITE_X_OFFSET] = new_x;
                nes->ram[ADDR_ENDING_CUSTOM_VARS + 1 + cur_sprite] = new_x;

                table_idx = ending_computeTypeBCathedralYTableIndex(nes);
                if (ending_typeBCathedralXTable[table_idx] == new_x) {
                    nes->ram[ADDR_ENDING_CUSTOM_VARS + 1 + cur_sprite + 1] = cathedralStartDeltaTable[level];
                }
            }
        }

        nes->ram[ADDR_GENERAL_COUNTER] = nes->ram[ADDR_ENDING_CURRENT_SPRITE];
        if (nes->ram[ADDR_ENDING_CURRENT_SPRITE] == nes->ram[ADDR_START_HEIGHT]) {
            break;
        }
        nes->ram[ADDR_ENDING_CURRENT_SPRITE]++;
    }

    if (ending_typeBCathedralFrameDelayTable[level] == nes->ram[ADDR_ENDING_CATHEDRAL_FRAME_DELAY_COUNTER]) {
        nes->ram[ADDR_ENDING_CATHEDRAL_FRAME_DELAY_COUNTER] = 0x00;
    }
}

/*
 * Master Type B cutscene sprite staging routine: branches to ending_typeBConcert
 * for level 9, or ending_typeBCathedral for levels 0..8.
 * Source address: $A52E (main.asm lines 4739-4748).
 */
void ending_typeB(NES_State *nes) {
    if (!nes) return;

    if (nes->ram[ADDR_LEVEL_NUMBER] == 9) {
        ending_typeBConcert(nes);
    } else {
        ending_typeBCathedral(nes);
    }
}

/*
 * Master ending sprite staging dispatcher: calls ending_typeB if Game Type B,
 * or ending_typeA if Game Type A.
 * Source address: $A527 (main.asm lines 4734-4738).
 */
void render_ending(NES_State *nes) {
    if (!nes) return;

    if (nes->ram[ADDR_GAME_TYPE] != GAMETYPE_A) {
        ending_typeB(nes);
    } else {
        ending_typeA(nes);
    }
}

/*
 * Executes unskippable ending frame sleep loop, calling render_ending and
 * waiting for NMI frame sync while decrementing sleepCounter to 0.
 * Source address: $A7FD (main.asm lines 5098-5105).
 */
void render_endingUnskippable(NES_State *nes, uint8_t count) {
    if (!nes) return;

    nes->ram[ADDR_SLEEP_COUNTER] = count;
    do {
        render_ending(nes);
        nes->ram[ADDR_OAM_STAGING_LENGTH] = 0;
        if (nes->ram[ADDR_SLEEP_COUNTER] > 0) {
            nes->ram[ADDR_SLEEP_COUNTER]--;
        }
    } while (nes->ram[ADDR_SLEEP_COUNTER] > 0);
}

/*
 * Runs full Type A ending cutscene sequence: sets CHR bank 2, loads nametable
 * and palette, selects ending tier, plays music, and waits for launch completion
 * and START button press.
 * Source address: $A926 (main.asm lines 5165-5195).
 */
void endingAnimationA(NES_State *nes) {
    if (!nes) return;

    nes->ram[ADDR_SPRITE_INDEX_OAM] = 0x02;
    nes->ram[ADDR_RENDER_MODE] = RENDER_MODE_ENDING;
    nes_write8(nes, REG_MMC1_CHR0, CHR_TYPEA_ENDING);
    nes_write8(nes, REG_MMC1_CHR1, CHR_TYPEA_ENDING);

    selectEndingScreen(nes);
    setMusicTrack(nes, 0x0A);
    render_endingUnskippable(nes, ENDING_SLEEP_TIME_1);

    while (nes->ram[ADDR_ENDING_CUSTOM_VARS] != 0 ||
           nes->ram[ADDR_NEWLY_PRESSED_P1] != BUTTON_START) {
        render_ending(nes);
        nes->ram[ADDR_OAM_STAGING_LENGTH] = 0;
        if (nes->ram[ADDR_ENDING_CUSTOM_VARS] == 0) {
            break;
        }
    }
}

/*
 * Runs full Type B ending cutscene sequence: sets CHR banks according to level,
 * initializes variables, plays music, counts down level and height bonus points
 * with software BCD decrements and sound effects, and waits for START button press.
 * Source address: $9E49 (main.asm lines 3802-3944).
 */
void endingAnimationB(NES_State *nes) {
    if (!nes) return;

    uint8_t p1_lvl = nes->ram[ADDR_P1_LEVEL_NUMBER];
    uint8_t level = (uint8_t)(levelDisplayTable[p1_lvl % 30] & 0x0F);
    nes->ram[ADDR_LEVEL_NUMBER] = level;

    nes->ram[ADDR_TOTAL_SCORE] = 0x00;
    nes->ram[ADDR_TOTAL_SCORE + 1] = 0x00;
    nes->ram[ADDR_TOTAL_SCORE + 2] = 0x00;

    nes->ram[ADDR_B_TYPE_LEVEL_BONUS] = (uint8_t)(level << 4);
    nes->ram[ADDR_B_TYPE_HEIGHT_BONUS] = (uint8_t)(nes->ram[ADDR_P1_START_HEIGHT] << 4);

    if (level == 9) {
        nes_write8(nes, REG_MMC1_CHR0, CHR_TYPEB_ENDING);
        nes_write8(nes, REG_MMC1_CHR1, CHR_TYPEB_ENDING);
    } else if (level == 2 || level == 6) {
        nes_write8(nes, REG_MMC1_CHR0, CHR_GAME);
        nes_write8(nes, REG_MMC1_CHR1, CHR_TYPEA_ENDING);
    } else {
        nes_write8(nes, REG_MMC1_CHR0, CHR_TYPEA_ENDING);
        nes_write8(nes, REG_MMC1_CHR1, CHR_TYPEA_ENDING);
    }

    ending_initTypeBVars(nes);
    nes->ram[ADDR_RENDER_MODE] = RENDER_MODE_ENDING;
    setMusicTrack(nes, 0x0A);
    render_endingUnskippable(nes, ENDING_SLEEP_TIME_1);

    /* Transfer Player 1 score to totalScore and clear player1_score */
    nes->ram[ADDR_TOTAL_SCORE] = nes->ram[ADDR_P1_SCORE];
    nes->ram[ADDR_TOTAL_SCORE + 1] = nes->ram[ADDR_P1_SCORE + 1];
    nes->ram[ADDR_TOTAL_SCORE + 2] = nes->ram[ADDR_P1_SCORE + 2];
    nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = 0x02;
    nes->ram[ADDR_P1_SCORE] = 0x00;
    nes->ram[ADDR_P1_SCORE + 1] = 0x00;
    nes->ram[ADDR_P1_SCORE + 2] = 0x00;
    render_endingUnskippable(nes, ENDING_SLEEP_TIME_2);

    /* Software BCD countdown of level bonus */
    while (nes->ram[ADDR_B_TYPE_LEVEL_BONUS] != 0) {
        nes->ram[ADDR_B_TYPE_LEVEL_BONUS]--;
        if ((nes->ram[ADDR_B_TYPE_LEVEL_BONUS] & 0x0F) == 0x0F) {
            nes->ram[ADDR_B_TYPE_LEVEL_BONUS] = (uint8_t)((nes->ram[ADDR_B_TYPE_LEVEL_BONUS] & 0xF0) | 0x09);
        }
        add100Points(nes);
        nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = 0x01;
        render_endingUnskippable(nes, ENDING_SLEEP_TIME_3);
    }
    render_endingUnskippable(nes, ENDING_SLEEP_TIME_2);

    /* Software BCD countdown of height bonus */
    while (nes->ram[ADDR_B_TYPE_HEIGHT_BONUS] != 0) {
        nes->ram[ADDR_B_TYPE_HEIGHT_BONUS]--;
        if ((nes->ram[ADDR_B_TYPE_HEIGHT_BONUS] & 0x0F) == 0x0F) {
            nes->ram[ADDR_B_TYPE_HEIGHT_BONUS] = (uint8_t)((nes->ram[ADDR_B_TYPE_HEIGHT_BONUS] & 0xF0) | 0x09);
        }
        add100Points(nes);
        nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = 0x01;
        render_endingUnskippable(nes, ENDING_SLEEP_TIME_3);
    }
    nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = 0x02;
    render_endingUnskippable(nes, ENDING_SLEEP_TIME_2);

    /* Wait for START button */
    while (nes->ram[ADDR_NEWLY_PRESSED_P1] != BUTTON_START) {
        render_ending(nes);
        nes->ram[ADDR_OAM_STAGING_LENGTH] = 0;
        break;
    }

    nes->ram[ADDR_LEVEL_NUMBER] = nes->ram[ADDR_P1_LEVEL_NUMBER];
    nes->ram[ADDR_SCORE] = nes->ram[ADDR_TOTAL_SCORE];
    nes->ram[ADDR_SCORE + 1] = nes->ram[ADDR_TOTAL_SCORE + 1];
    nes->ram[ADDR_SCORE + 2] = nes->ram[ADDR_TOTAL_SCORE + 2];
}

/*
 * Master ending cutscene entry point: sets sprite index 2, render mode 4,
 * and dispatches to endingAnimationA (Type A) or endingAnimationB (Type B).
 * Source address: $9E3A (main.asm lines 3790-3798).
 */
void endingAnimation(NES_State *nes) {
    if (!nes) return;

    nes->ram[ADDR_SPRITE_INDEX_OAM] = 0x02;
    nes->ram[ADDR_RENDER_MODE] = RENDER_MODE_ENDING;

    if (nes->ram[ADDR_GAME_TYPE] != GAMETYPE_A) {
        endingAnimationB(nes);
    } else {
        endingAnimationA(nes);
    }
}
