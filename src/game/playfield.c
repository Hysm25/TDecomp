#include "game/playfield.h"

/*
 * Authoritative Data Tables matching reference ROM byte-for-byte.
 */

/* multBy10Table: row index (0..19) * 10 ($96D6 in PRG ROM) */
const uint8_t multBy10Table[20] = {
    0x00, 0x0A, 0x14, 0x1E, 0x28, 0x32, 0x3C, 0x46,
    0x50, 0x5A, 0x64, 0x6E, 0x78, 0x82, 0x8C, 0x96,
    0xA0, 0xAA, 0xB4, 0xBE
};

/* garbageLinesTable: garbage lines sent to opponent in 2-player mode ($9B53 in PRG ROM) */
const uint8_t garbageLinesTable[5] = {
    0x00, 0x00, 0x01, 0x02, 0x04
};

/* leftColumnsTable: 5-step center-out left columns ($97FE in PRG ROM) */
const uint8_t leftColumnsTable[5] = {
    0x04, 0x03, 0x02, 0x01, 0x00
};

/* rightColumnsTable: 5-step center-out right columns ($9803 in PRG ROM) */
const uint8_t rightColumnsTable[5] = {
    0x05, 0x06, 0x07, 0x08, 0x09
};

/* typeBBlankInitCountByHeightTable: cells cleared by Type B start height ($8876 in PRG ROM) */
const uint8_t typeBBlankInitCountByHeightTable[6] = {
    0xC8, 0xAA, 0x96, 0x78, 0x64, 0x50
};

/* rngTable: random garbage tile distribution for Type B ($887C in PRG ROM) */
const uint8_t rngTable[8] = {
    TILE_EMPTY, TILE_1, TILE_EMPTY, TILE_2, TILE_3, TILE_3, TILE_EMPTY, TILE_EMPTY
};

void playState_lockTetrimino(NES_State *nes) {
    if (!nes) return;

    if (isPositionValid(nes) != 0) {
        /* Piece position invalid / out of bounds triggers game over sequence */
        nes->ram[ADDR_SOUND_EFFECT_SLOT0_INIT] = 0x02;
        nes->ram[ADDR_PLAY_STATE] = 0x0A;
        nes->ram[ADDR_CURTAIN_ROW] = 0xF0;
        return;
    }

    if (nes->ram[ADDR_VRAM_ROW] < 0x20) {
        /* Wait until playfield VRAM row copy is complete */
        return;
    }

    uint8_t tet_x = nes->ram[ADDR_TETRIMINO_X];
    uint8_t tet_y = nes->ram[ADDR_TETRIMINO_Y];
    uint8_t piece = nes->ram[ADDR_CURRENT_PIECE];
    if (piece >= ORIENTATION_COUNT) return;

    uint16_t pf_base = nes_read16(nes, ADDR_PLAYFIELD_ADDR);
    if (pf_base == 0) pf_base = ADDR_PLAYFIELD;

    const uint8_t *or_ptr = &orientationTable[piece * 12];
    for (int i = 0; i < 4; i++) {
        int8_t y_off = (int8_t)or_ptr[i * 3 + 0];
        uint8_t tile_id = or_ptr[i * 3 + 1];
        int8_t x_off = (int8_t)or_ptr[i * 3 + 2];

        int16_t row = (int16_t)tet_y + y_off;
        int16_t col = (int16_t)tet_x + x_off;
        if (row >= 0 && row < PLAYFIELD_HEIGHT && col >= 0 && col < PLAYFIELD_WIDTH) {
            uint16_t cell_idx = (uint16_t)(row * 10 + col);
            nes_write8(nes, pf_base + cell_idx, tile_id);
        }
    }

    nes->ram[ADDR_LINE_INDEX] = 0x00;
    updatePlayfield(nes);
    updateMusicSpeed(nes);
    nes->ram[ADDR_PLAY_STATE]++;
}

void playState_checkForCompletedRows(NES_State *nes) {
    if (!nes) return;

    if (nes->ram[ADDR_VRAM_ROW] < 0x20) {
        return;
    }

    int8_t tet_y = (int8_t)nes->ram[ADDR_TETRIMINO_Y];
    int16_t base_y = tet_y - 2;
    if (base_y < 0) {
        base_y = 0;
    }

    uint8_t l_idx = nes->ram[ADDR_LINE_INDEX];
    int16_t check_row = base_y + l_idx;

    uint16_t pf_base = nes_read16(nes, ADDR_PLAYFIELD_ADDR);
    if (pf_base == 0) pf_base = ADDR_PLAYFIELD;

    bool row_complete = false;
    if (check_row < PLAYFIELD_HEIGHT) {
        row_complete = true;
        uint16_t row_start = (uint16_t)(check_row * 10);
        for (int c = 0; c < 10; c++) {
            if (nes_read8(nes, pf_base + row_start + c) == TILE_EMPTY) {
                row_complete = false;
                break;
            }
        }
    }

    if (row_complete) {
        nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = 0x0A;
        nes->ram[ADDR_COMPLETED_LINES]++;
        if (l_idx < 4) {
            nes->ram[ADDR_COMPLETED_ROW + l_idx] = (uint8_t)check_row;
        }

        /* Shift all playfield rows above down by 1 row (+10) */
        int16_t y = (int16_t)(check_row * 10 - 1);
        for (; y >= 0; y--) {
            uint8_t val = nes_read8(nes, pf_base + y);
            nes_write8(nes, pf_base + y + 10, val);
        }

        /* Clear top row with TILE_EMPTY */
        for (int c = 0; c < 10; c++) {
            nes_write8(nes, pf_base + c, TILE_EMPTY);
        }

        nes->ram[ADDR_CURRENT_PIECE] = ORIENT_HIDDEN;
    } else {
        if (l_idx < 4) {
            nes->ram[ADDR_COMPLETED_ROW + l_idx] = 0x00;
        }
    }

    nes->ram[ADDR_LINE_INDEX]++;
    if (nes->ram[ADDR_LINE_INDEX] < 4) {
        return;
    }

    /* All 4 candidate rows checked */
    uint8_t comp_lines = nes->ram[ADDR_COMPLETED_LINES];
    uint8_t garbage_idx = (comp_lines > 4) ? 4 : comp_lines;
    nes->ram[ADDR_PENDING_GARBAGE_INACTIVE] += garbageLinesTable[garbage_idx];
    nes->ram[ADDR_VRAM_ROW] = 0x00;
    nes->ram[ADDR_ROW_Y] = 0x00;

    if (comp_lines == 4) {
        nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = 0x04; /* Tetris sound effect */
    }

    nes->ram[ADDR_PLAY_STATE]++;
    if (comp_lines == 0) {
        /* When no lines are cleared, skip PlayState 4 animation straight to PlayState 5 */
        nes->ram[ADDR_PLAY_STATE]++;
        nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = 0x07; /* Piece lock sound */
    }
}

void updateLineClearingAnimation(NES_State *nes) {
    if (!nes) return;

    if ((nes->ram[ADDR_FRAME_COUNTER] & 0x03) != 0) {
        return;
    }

    nes->ram[ADDR_ROW_Y]++;
    if (nes->ram[ADDR_ROW_Y] >= 5) {
        nes->ram[ADDR_PLAY_STATE]++;
    }
}

void playState_updateGameOverCurtain(NES_State *nes) {
    if (!nes) return;

    uint8_t c_row = nes->ram[ADDR_CURTAIN_ROW];
    if (c_row == 20) {
        return;
    }

    if ((nes->ram[ADDR_FRAME_COUNTER] & 0x03) != 0) {
        return;
    }

    if ((c_row & 0x80) == 0) {
        /* Non-negative curtainRow: draw curtain row on playfield */
        if (c_row < 20) {
            uint16_t pf_base = nes_read16(nes, ADDR_PLAYFIELD_ADDR);
            if (pf_base == 0) pf_base = ADDR_PLAYFIELD;
            uint16_t row_start = (uint16_t)(c_row * 10);
            for (int col = 0; col < 10; col++) {
                nes_write8(nes, pf_base + row_start + col, TILE_CURTAIN);
            }
            nes->ram[ADDR_CURRENT_PIECE] = ORIENT_HIDDEN;
            nes->ram[ADDR_VRAM_ROW] = c_row;
        }
    }

    nes->ram[ADDR_CURTAIN_ROW]++;
}

void playState_receiveGarbage(NES_State *nes) {
    if (!nes) return;

    if (nes->ram[ADDR_NUMBER_OF_PLAYERS] == 1 || nes->ram[ADDR_PENDING_GARBAGE] == 0) {
        nes->ram[ADDR_PLAY_STATE]++;
        return;
    }

    if (nes->ram[ADDR_VRAM_ROW] < 0x20) {
        return;
    }

    uint8_t garbage_count = nes->ram[ADDR_PENDING_GARBAGE];
    if (garbage_count > 20) garbage_count = 20;

    uint16_t pf_base = nes_read16(nes, ADDR_PLAYFIELD_ADDR);
    if (pf_base == 0) pf_base = ADDR_PLAYFIELD;

    uint16_t shift_cells = (uint16_t)(garbage_count * 10);
    uint16_t dst = 0;
    uint16_t src = shift_cells;
    while (src < PLAYFIELD_SIZE) {
        nes_write8(nes, pf_base + dst, nes_read8(nes, pf_base + src));
        dst++;
        src++;
    }

    uint8_t hole = nes->ram[ADDR_GARBAGE_HOLE];
    uint8_t col = 0;
    while (dst < PLAYFIELD_SIZE) {
        uint8_t tile = (col == hole) ? TILE_HIDDEN : TILE_GARBAGE;
        nes_write8(nes, pf_base + dst, tile);
        col++;
        if (col >= 10) col = 0;
        dst++;
    }

    nes->ram[ADDR_PENDING_GARBAGE] = 0;
    nes->ram[ADDR_VRAM_ROW] = 0;
    nes->ram[ADDR_PLAY_STATE]++;
}

void initPlayfieldIfTypeB(NES_State *nes) {
    if (!nes) return;

    if (nes->ram[ADDR_GAME_TYPE] != GAMETYPE_B) {
        return;
    }
    initPlayfieldForTypeB(nes);
}

void initPlayfieldForTypeB(NES_State *nes) {
    if (!nes) return;

    nes->ram[ADDR_P1_VRAM_ROW] = 0;
    nes->ram[ADDR_P2_VRAM_ROW] = 0;

    /* Generate garbage in bottom 12 rows (rows 8 through 19) */
    for (uint8_t r = 8; r < 20; r++) {
        for (int8_t c = 9; c >= 0; c--) {
            generateNextPseudorandomNumber(nes, ADDR_RNG_SEED, 2);
            uint8_t seed_lo = nes->ram[ADDR_RNG_SEED];
            uint8_t tile = rngTable[seed_lo & 0x07];
            nes_write8(nes, ADDR_PLAYFIELD + r * 10 + c, tile);
        }

        /* Guarantee at least one empty square in the row */
        uint8_t blank_col;
        do {
            generateNextPseudorandomNumber(nes, ADDR_RNG_SEED, 2);
            blank_col = nes->ram[ADDR_RNG_SEED] & 0x0F;
        } while (blank_col >= 10);

        nes_write8(nes, ADDR_PLAYFIELD + r * 10 + blank_col, TILE_EMPTY);
    }

    /* Copy generated playfield to Player 2 */
    for (int i = 0; i < PLAYFIELD_SIZE; i++) {
        nes_write8(nes, ADDR_PLAYFIELD_P2 + i, nes_read8(nes, ADDR_PLAYFIELD + i));
    }

    /* Clear upper rows based on Player 1 startHeight (0..5) */
    uint8_t h1 = nes->ram[ADDR_P1_START_HEIGHT];
    if (h1 > 5) h1 = 5;
    uint8_t limit1 = typeBBlankInitCountByHeightTable[h1];
    for (int i = 0; i < limit1 && i < PLAYFIELD_SIZE; i++) {
        nes_write8(nes, ADDR_PLAYFIELD + i, TILE_EMPTY);
    }

    /* Clear upper rows based on Player 2 startHeight (0..5) */
    uint8_t h2 = nes->ram[ADDR_P2_START_HEIGHT];
    if (h2 > 5) h2 = 5;
    uint8_t limit2 = typeBBlankInitCountByHeightTable[h2];
    for (int i = 0; i < limit2 && i < PLAYFIELD_SIZE; i++) {
        nes_write8(nes, ADDR_PLAYFIELD_P2 + i, TILE_EMPTY);
    }
}
