#include "math/rng.h"

/*
 * Table mapping piece orientation (0..18) to canonical piece ID (0..6).
 * Source: main.asm lines 3005-3012
 */
const uint8_t tetriminoTypeFromOrientation[19] = {
    PIECE_T, PIECE_T, PIECE_T, PIECE_T, /* 0..3: tUp, tRight, tDown, tLeft */
    PIECE_J, PIECE_J, PIECE_J, PIECE_J, /* 4..7: jLeft, jUp, jRight, jDown */
    PIECE_Z, PIECE_Z,                   /* 8..9: zHoriz, zVert */
    PIECE_O,                            /* 10: oFixed */
    PIECE_S, PIECE_S,                   /* 11..12: sHoriz, sVert */
    PIECE_L, PIECE_L, PIECE_L, PIECE_L, /* 13..16: lRight, lDown, lLeft, lUp */
    PIECE_I, PIECE_I                    /* 17..18: iVert, iHoriz */
};

/*
 * Spawn table with initial orientations for each of the 7 piece types.
 * Source: main.asm lines 3013-3014
 */
const uint8_t spawnTable[7] = {
    ORIENT_T_DOWN,  /* 0x02 */
    ORIENT_J_DOWN,  /* 0x07 */
    ORIENT_Z_HORIZ, /* 0x08 */
    ORIENT_O_FIXED, /* 0x0A */
    ORIENT_S_HORIZ, /* 0x0B */
    ORIENT_L_DOWN,  /* 0x0E */
    ORIENT_I_HORIZ   /* 0x12 */
};

/*
 * Table mapping any orientation back to its initial spawn orientation.
 * Source: main.asm lines 3021-3028
 */
const uint8_t spawnOrientationFromOrientation[19] = {
    ORIENT_T_DOWN,  ORIENT_T_DOWN,  ORIENT_T_DOWN,  ORIENT_T_DOWN,
    ORIENT_J_DOWN,  ORIENT_J_DOWN,  ORIENT_J_DOWN,  ORIENT_J_DOWN,
    ORIENT_Z_HORIZ, ORIENT_Z_HORIZ,
    ORIENT_O_FIXED,
    ORIENT_S_HORIZ, ORIENT_S_HORIZ,
    ORIENT_L_DOWN,  ORIENT_L_DOWN,  ORIENT_L_DOWN,  ORIENT_L_DOWN,
    ORIENT_I_HORIZ, ORIENT_I_HORIZ
};

/*
 * Demo mode piece stream table.
 * Source: data/demo_data.asm lines 136-168
 */
static const uint8_t demoTetriminoTypeTable[256] = {
    0x00, 0x14, 0x8A, 0x45, 0x22, 0x11, 0x88, 0x44,
    0x22, 0x91, 0x48, 0xA4, 0x52, 0x29, 0x14, 0x0A,
    0x85, 0xC2, 0xE1, 0x70, 0x38, 0x9C, 0x4E, 0xA7,
    0x53, 0xA9, 0xD4, 0x6A, 0xB5, 0x5A, 0xAD, 0xD6,
    0x6B, 0x35, 0x1A, 0x8D, 0xC6, 0xE3, 0x71, 0x38,
    0x9C, 0xCE, 0xE7, 0x73, 0xB9, 0xDC, 0xEE, 0xF7,
    0xFB, 0xFD, 0xFE, 0x7F, 0x3F, 0x9F, 0xCF, 0x67,
    0x33, 0x19, 0x0C, 0x86, 0x43, 0x21, 0x90, 0xC8,
    0xE4, 0xF2, 0xF9, 0x7C, 0xBE, 0x5F, 0xAF, 0xD7,
    0xEB, 0xF5, 0xFA, 0xFD, 0x7E, 0x3F, 0x1F, 0x0F,
    0x07, 0x03, 0x81, 0xC0, 0x60, 0xB0, 0xD8, 0xEC,
    0xF6, 0x7B, 0x3D, 0x1E, 0x8F, 0xC7, 0xE3, 0xF1,
    0x78, 0xBC, 0xDE, 0xEF, 0x77, 0x3B, 0x1D, 0x8E,
    0xC7, 0xE3, 0xF1, 0xF8, 0xFC, 0xFE, 0x7F, 0xBF,
    0x5F, 0x2F, 0x17, 0x8B, 0xC5, 0x62, 0x31, 0x98,
    0xCC, 0xE6, 0x73, 0x39, 0x9C, 0x4E, 0x27, 0x93,
    0xC9, 0x64, 0xB2, 0x59, 0x2C, 0x16, 0x0B, 0x05,
    0x82, 0xC1, 0x60, 0xB0, 0x58, 0x2C, 0x96, 0x4B,
    0xA5, 0xD2, 0xE9, 0x74, 0x3A, 0x9D, 0x4E, 0x27,
    0x13, 0x89, 0xC4, 0x62, 0xB1, 0xD8, 0x6C, 0xB6,
    0x5B, 0x2D, 0x16, 0x8B, 0x45, 0x22, 0x91, 0x48,
    0xA4, 0xD2, 0xE9, 0xF4, 0xFA, 0xFD, 0xFE, 0xFF,
    0xFF, 0xFF, 0x7F, 0xBF, 0xDF, 0x6F, 0xB7, 0x5B,
    0x2D, 0x96, 0x4B, 0x25, 0x92, 0x49, 0xA4, 0xD2,
    0x69, 0x34, 0x9A, 0x4D, 0x26, 0x13, 0x89, 0x44,
    0xA2, 0xD1, 0x68, 0xB4, 0x5A, 0x2D, 0x96, 0xCB,
    0xE5, 0xF2, 0xF9, 0x7C, 0x3E, 0x1F, 0x8F, 0x47,
    0x23, 0x91, 0xC8, 0x64, 0x32, 0x19, 0x8C, 0xC6,
    0x63, 0x31, 0x18, 0x0C, 0x06, 0x03, 0x81, 0x40,
    0xA0, 0xD0, 0x68, 0x34, 0x1A, 0x0D, 0x86, 0xC3,
    0x78, 0xBC, 0xDE, 0xEF, 0x77, 0x3B, 0x1D, 0x8E,
    0xC7, 0xE3, 0xF1, 0xF8, 0xFC, 0xFE, 0x7F, 0xBF
};

void generateNextPseudorandomNumber(NES_State *nes, uint8_t seed_addr, uint8_t seed_size) {
    if (!nes || seed_size == 0) return;

    uint8_t b0 = nes->ram[seed_addr];
    uint8_t b1 = nes->ram[(uint8_t)(seed_addr + 1)];

    /* Bit 1 of byte X and Bit 1 of byte X+1 */
    uint8_t bit0 = (b0 & 0x02) >> 1;
    uint8_t bit1 = (b1 & 0x02) >> 1;
    uint8_t carry = bit0 ^ bit1;

    nes->ram[ADDR_TMP1] = (uint8_t)(b0 & 0x02);

    uint8_t curr = seed_addr;
    for (uint8_t i = 0; i < seed_size; i++) {
        uint8_t val = nes->ram[curr];
        uint8_t next_carry = val & 0x01;
        nes->ram[curr] = (uint8_t)((val >> 1) | (carry << 7));
        carry = next_carry;
        curr++;
    }
}

void unreferenced_soundRng(NES_State *nes) {
    if (!nes) return;

    uint8_t b0 = nes->ram[ADDR_SOUND_RNG_SEED];
    uint8_t b1 = nes->ram[ADDR_SOUND_RNG_SEED + 1];

    nes->ram[ADDR_UNREFERENCED_SOUND_RNG_TMP] = (uint8_t)(b0 & 0x02);
    uint8_t bit0 = (b0 & 0x02) >> 1;
    uint8_t bit1 = (b1 & 0x02) >> 1;
    uint8_t carry = bit0 ^ bit1;

    uint8_t next_carry = b0 & 0x01;
    nes->ram[ADDR_SOUND_RNG_SEED] = (uint8_t)((b0 >> 1) | (carry << 7));
    nes->ram[ADDR_SOUND_RNG_SEED + 1] = (uint8_t)((b1 >> 1) | (next_carry << 7));
}

uint8_t pickRandomTetrimino(NES_State *nes) {
    if (!nes) return ORIENT_T_DOWN;

    /* Increment spawn count */
    uint8_t spawn_count = (uint8_t)(nes->ram[ADDR_SPAWN_COUNT] + 1);
    nes->ram[ADDR_SPAWN_COUNT] = spawn_count;

    uint8_t seed_low = nes->ram[ADDR_RNG_SEED];
    uint8_t idx = (uint8_t)((seed_low + spawn_count) & 0x07);
    uint8_t spawn_id = nes->ram[ADDR_SPAWN_ID];

    if (idx < 7) {
        uint8_t candidate = spawnTable[idx];
        if (candidate != spawn_id) {
            nes->ram[ADDR_SPAWN_ID] = candidate;
            return candidate;
        }
    }

    /* Reroll branch (@invalidIndex) */
    generateNextPseudorandomNumber(nes, ADDR_RNG_SEED, 2);
    seed_low = nes->ram[ADDR_RNG_SEED];

    uint8_t sum = (uint8_t)((seed_low & 0x07) + spawn_id);
    while (sum >= 7) {
        sum -= 7;
    }

    uint8_t candidate = spawnTable[sum];
    nes->ram[ADDR_SPAWN_ID] = candidate;
    return candidate;
}

uint8_t chooseNextTetrimino(NES_State *nes) {
    if (!nes) return ORIENT_T_DOWN;

    uint8_t mode = nes->ram[ADDR_GAME_MODE];
    if (mode == MODE_PLAY_ENDING_HIGH_SCORE) {
        uint8_t demo_idx = nes->ram[ADDR_DEMO_INDEX];
        nes->ram[ADDR_DEMO_INDEX] = (uint8_t)(demo_idx + 1);

        uint8_t entry = demoTetriminoTypeTable[demo_idx];
        uint8_t idx = (uint8_t)((entry >> 4) & 0x07);
        if (idx >= 7) {
            idx = 0;
        }
        return spawnTable[idx];
    }

    return pickRandomTetrimino(nes);
}

void incrementPieceStat(NES_State *nes, uint8_t orientation) {
    if (!nes || orientation >= ORIENTATION_COUNT) return;

    uint8_t piece_type = tetriminoTypeFromOrientation[orientation];
    uint8_t offset = (uint8_t)(piece_type * 2);

    uint8_t low_val = (uint8_t)(nes->ram[ADDR_STATS_BY_TYPE + offset] + 1);
    uint8_t counter = low_val;

    if ((counter & 0x0F) >= 0x0A) {
        counter += 0x06;
        if (counter >= 0xA0) {
            counter += 0x60;
            nes->ram[ADDR_STATS_BY_TYPE + offset + 1]++;
        }
    }

    nes->ram[ADDR_STATS_BY_TYPE + offset] = counter;
    nes->ram[ADDR_GENERAL_COUNTER] = counter;
}
