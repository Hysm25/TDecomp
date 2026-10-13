#include "game/tetrimino.h"
#include "game/render.h"

/*
 * Rotation lookup table: 19 orientations * 2 directions (B counter-clockwise, A clockwise).
 * Source: main.asm lines 1503-1528
 */
const uint8_t rotationTable[19 * 2] = {
    ORIENT_T_LEFT,  ORIENT_T_RIGHT,  /* 0x00: from tUp */
    ORIENT_T_UP,    ORIENT_T_DOWN,   /* 0x01: from tRight */
    ORIENT_T_RIGHT, ORIENT_T_LEFT,   /* 0x02: from tDown */
    ORIENT_T_DOWN,  ORIENT_T_UP,     /* 0x03: from tLeft */

    ORIENT_J_DOWN,  ORIENT_J_UP,     /* 0x04: from jLeft */
    ORIENT_J_LEFT,  ORIENT_J_RIGHT,  /* 0x05: from jUp */
    ORIENT_J_UP,    ORIENT_J_DOWN,   /* 0x06: from jRight */
    ORIENT_J_RIGHT, ORIENT_J_LEFT,   /* 0x07: from jDown */

    ORIENT_Z_VERT,  ORIENT_Z_VERT,   /* 0x08: from zHoriz */
    ORIENT_Z_HORIZ, ORIENT_Z_HORIZ,  /* 0x09: from zVert */

    ORIENT_O_FIXED, ORIENT_O_FIXED,  /* 0x0A: from oFixed */

    ORIENT_S_VERT,  ORIENT_S_VERT,   /* 0x0B: from sHoriz */
    ORIENT_S_HORIZ, ORIENT_S_HORIZ,  /* 0x0C: from sVert */

    ORIENT_L_UP,    ORIENT_L_DOWN,   /* 0x0D: from lRight */
    ORIENT_L_RIGHT, ORIENT_L_LEFT,   /* 0x0E: from lDown */
    ORIENT_L_DOWN,  ORIENT_L_UP,     /* 0x0F: from lLeft */
    ORIENT_L_LEFT,  ORIENT_L_RIGHT,  /* 0x10: from lUp */

    ORIENT_I_HORIZ, ORIENT_I_HORIZ,  /* 0x11: from iVert */
    ORIENT_I_VERT,  ORIENT_I_VERT    /* 0x12: from iHoriz */
};

/*
 * Orientation geometry table: 19 orientations * 4 minos * (y_offset, tile_id, x_offset).
 * Source: main.asm lines 1759-1785
 */
const uint8_t orientationTable[19 * 12] = {
    /* 0x00: tUp */
    (uint8_t) 0, TILE_1, (uint8_t)-1,   (uint8_t) 0, TILE_1, (uint8_t) 0,   (uint8_t) 0, TILE_1, (uint8_t) 1,  (uint8_t)-1, TILE_1, (uint8_t) 0,
    /* 0x01: tRight */
    (uint8_t)-1, TILE_1, (uint8_t) 0,   (uint8_t) 0, TILE_1, (uint8_t) 0,   (uint8_t) 0, TILE_1, (uint8_t) 1,  (uint8_t) 1, TILE_1, (uint8_t) 0,
    /* 0x02: tDown (spawn) */
    (uint8_t) 0, TILE_1, (uint8_t)-1,   (uint8_t) 0, TILE_1, (uint8_t) 0,   (uint8_t) 0, TILE_1, (uint8_t) 1,  (uint8_t) 1, TILE_1, (uint8_t) 0,
    /* 0x03: tLeft */
    (uint8_t)-1, TILE_1, (uint8_t) 0,   (uint8_t) 0, TILE_1, (uint8_t)-1,   (uint8_t) 0, TILE_1, (uint8_t) 0,  (uint8_t) 1, TILE_1, (uint8_t) 0,

    /* 0x04: jLeft */
    (uint8_t)-1, TILE_3, (uint8_t) 0,   (uint8_t) 0, TILE_3, (uint8_t) 0,   (uint8_t) 1, TILE_3, (uint8_t)-1,  (uint8_t) 1, TILE_3, (uint8_t) 0,
    /* 0x05: jUp */
    (uint8_t)-1, TILE_3, (uint8_t)-1,   (uint8_t) 0, TILE_3, (uint8_t)-1,   (uint8_t) 0, TILE_3, (uint8_t) 0,  (uint8_t) 0, TILE_3, (uint8_t) 1,
    /* 0x06: jRight */
    (uint8_t)-1, TILE_3, (uint8_t) 0,   (uint8_t)-1, TILE_3, (uint8_t) 1,   (uint8_t) 0, TILE_3, (uint8_t) 0,  (uint8_t) 1, TILE_3, (uint8_t) 0,
    /* 0x07: jDown (spawn) */
    (uint8_t) 0, TILE_3, (uint8_t)-1,   (uint8_t) 0, TILE_3, (uint8_t) 0,   (uint8_t) 0, TILE_3, (uint8_t) 1,  (uint8_t) 1, TILE_3, (uint8_t) 1,

    /* 0x08: zHoriz (spawn) */
    (uint8_t) 0, TILE_2, (uint8_t)-1,   (uint8_t) 0, TILE_2, (uint8_t) 0,   (uint8_t) 1, TILE_2, (uint8_t) 0,  (uint8_t) 1, TILE_2, (uint8_t) 1,
    /* 0x09: zVert */
    (uint8_t)-1, TILE_2, (uint8_t) 1,   (uint8_t) 0, TILE_2, (uint8_t) 0,   (uint8_t) 0, TILE_2, (uint8_t) 1,  (uint8_t) 1, TILE_2, (uint8_t) 0,

    /* 0x0A: oFixed (spawn) */
    (uint8_t) 0, TILE_1, (uint8_t)-1,   (uint8_t) 0, TILE_1, (uint8_t) 0,   (uint8_t) 1, TILE_1, (uint8_t)-1,  (uint8_t) 1, TILE_1, (uint8_t) 0,

    /* 0x0B: sHoriz (spawn) */
    (uint8_t) 0, TILE_3, (uint8_t) 0,   (uint8_t) 0, TILE_3, (uint8_t) 1,   (uint8_t) 1, TILE_3, (uint8_t)-1,  (uint8_t) 1, TILE_3, (uint8_t) 0,
    /* 0x0C: sVert */
    (uint8_t)-1, TILE_3, (uint8_t) 0,   (uint8_t) 0, TILE_3, (uint8_t) 0,   (uint8_t) 0, TILE_3, (uint8_t) 1,  (uint8_t) 1, TILE_3, (uint8_t) 1,

    /* 0x0D: lRight */
    (uint8_t)-1, TILE_2, (uint8_t) 0,   (uint8_t) 0, TILE_2, (uint8_t) 0,   (uint8_t) 1, TILE_2, (uint8_t) 0,  (uint8_t) 1, TILE_2, (uint8_t) 1,
    /* 0x0E: lDown (spawn) */
    (uint8_t) 0, TILE_2, (uint8_t)-1,   (uint8_t) 0, TILE_2, (uint8_t) 0,   (uint8_t) 0, TILE_2, (uint8_t) 1,  (uint8_t) 1, TILE_2, (uint8_t)-1,
    /* 0x0F: lLeft */
    (uint8_t)-1, TILE_2, (uint8_t)-1,  (uint8_t)-1, TILE_2, (uint8_t) 0,   (uint8_t) 0, TILE_2, (uint8_t) 0,  (uint8_t) 1, TILE_2, (uint8_t) 0,
    /* 0x10: lUp */
    (uint8_t)-1, TILE_2, (uint8_t) 1,   (uint8_t) 0, TILE_2, (uint8_t)-1,   (uint8_t) 0, TILE_2, (uint8_t) 0,  (uint8_t) 0, TILE_2, (uint8_t) 1,

    /* 0x11: iVert */
    (uint8_t)-2, TILE_1, (uint8_t) 0,  (uint8_t)-1, TILE_1, (uint8_t) 0,   (uint8_t) 0, TILE_1, (uint8_t) 0,  (uint8_t) 1, TILE_1, (uint8_t) 0,
    /* 0x12: iHoriz (spawn) */
    (uint8_t) 0, TILE_1, (uint8_t)-2,   (uint8_t) 0, TILE_1, (uint8_t)-1,   (uint8_t) 0, TILE_1, (uint8_t) 0,  (uint8_t) 0, TILE_1, (uint8_t) 1
};

/*
 * Frames per drop table (NTSC): levels 0 to 28.
 * Source: main.asm lines 1608-1611
 */
const uint8_t framesPerDropTable[29] = {
    0x30, 0x2B, 0x26, 0x21, 0x1C, 0x17, 0x12, 0x0D,
    0x08, 0x06, 0x05, 0x05, 0x05, 0x04, 0x04, 0x04,
    0x03, 0x03, 0x03, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02
};

uint8_t getDropSpeed(uint8_t level) {
    if (level >= 29) {
        return 0x01;
    }
    return framesPerDropTable[level];
}

uint8_t isPositionValid(NES_State *nes) {
    if (!nes) return 0xFF;

    uint8_t tet_y = nes->ram[ADDR_TETRIMINO_Y];
    uint8_t tet_x = nes->ram[ADDR_TETRIMINO_X];
    uint8_t piece = nes->ram[ADDR_CURRENT_PIECE];
    if (piece >= ORIENTATION_COUNT) {
        nes->ram[ADDR_GENERAL_COUNTER] = 0xFF;
        return 0xFF;
    }

    uint16_t pf_base = nes_read16(nes, ADDR_PLAYFIELD_ADDR);
    if (pf_base < 0x0400 || pf_base >= 0x0700) {
        pf_base = ADDR_PLAYFIELD;
    }

    const uint8_t *entry = &orientationTable[piece * 12];
    for (uint8_t mino = 0; mino < 4; mino++) {
        int8_t y_off = (int8_t)entry[mino * 3 + 0];
        int8_t x_off = (int8_t)entry[mino * 3 + 2];

        /* Vertical bounds check */
        int16_t row = (int16_t)(tet_y + y_off);
        uint8_t check_row = (uint8_t)(row + 2);
        if (check_row >= 0x16) {
            nes->ram[ADDR_GENERAL_COUNTER] = 0xFF;
            return 0xFF;
        }

        /* Horizontal bounds check */
        int16_t col = (int16_t)(tet_x + x_off);
        if (col < 0 || col >= PLAYFIELD_WIDTH) {
            nes->ram[ADDR_GENERAL_COUNTER] = 0xFF;
            return 0xFF;
        }

        /* Occupied playfield cell check */
        if (row >= 0 && row < PLAYFIELD_HEIGHT) {
            uint16_t cell_offset = (uint16_t)(row * 10 + col);
            uint8_t tile = nes_read8(nes, (uint16_t)(pf_base + cell_offset));
            if (tile < TILE_EMPTY) {
                nes->ram[ADDR_GENERAL_COUNTER] = 0xFF;
                return 0xFF;
            }
        }
    }

    nes->ram[ADDR_GENERAL_COUNTER] = 0x00;
    return 0x00;
}

void rotate_tetrimino(NES_State *nes) {
    if (!nes) return;

    uint8_t piece = nes->ram[ADDR_CURRENT_PIECE];
    if (piece >= ORIENTATION_COUNT) return;
    nes->ram[ADDR_ORIGINAL_Y] = piece;

    uint8_t pressed = nes->ram[ADDR_NEWLY_PRESSED_BUTTONS];
    if (pressed & BUTTON_A) {
        /* Clockwise rotation */
        nes->ram[ADDR_CURRENT_PIECE] = rotationTable[piece * 2 + 1];
        if (isPositionValid(nes) == 0) {
            nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = 0x05;
            return;
        }
        nes->ram[ADDR_CURRENT_PIECE] = nes->ram[ADDR_ORIGINAL_Y];
        return;
    }

    if (pressed & BUTTON_B) {
        /* Counter-clockwise rotation */
        nes->ram[ADDR_CURRENT_PIECE] = rotationTable[piece * 2];
        if (isPositionValid(nes) == 0) {
            nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = 0x05;
            return;
        }
        nes->ram[ADDR_CURRENT_PIECE] = nes->ram[ADDR_ORIGINAL_Y];
        return;
    }
}

void shift_tetrimino(NES_State *nes) {
    if (!nes) return;

    uint8_t old_x = nes->ram[ADDR_TETRIMINO_X];
    nes->ram[ADDR_ORIGINAL_Y] = old_x;

    uint8_t held = nes->ram[ADDR_HELD_BUTTONS];
    if (held & BUTTON_DOWN) {
        return;
    }

    uint8_t pressed = nes->ram[ADDR_NEWLY_PRESSED_BUTTONS];
    if (pressed & (BUTTON_LEFT | BUTTON_RIGHT)) {
        nes->ram[ADDR_AUTOREPEAT_X] = 0;
    } else {
        if (!(held & (BUTTON_LEFT | BUTTON_RIGHT))) {
            return;
        }
        uint8_t auto_x = (uint8_t)(nes->ram[ADDR_AUTOREPEAT_X] + 1);
        nes->ram[ADDR_AUTOREPEAT_X] = auto_x;
        if (auto_x < DAS_RESET_NTSC) {
            return;
        }
        nes->ram[ADDR_AUTOREPEAT_X] = DAS_DELAY_NTSC;
    }

    /* Move right */
    if (held & BUTTON_RIGHT) {
        nes->ram[ADDR_TETRIMINO_X] = (uint8_t)(old_x + 1);
        if (isPositionValid(nes) != 0) {
            nes->ram[ADDR_TETRIMINO_X] = nes->ram[ADDR_ORIGINAL_Y];
            nes->ram[ADDR_AUTOREPEAT_X] = DAS_RESET_NTSC;
            return;
        }
        nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = 0x03;
        return;
    }

    /* Move left */
    if (held & BUTTON_LEFT) {
        nes->ram[ADDR_TETRIMINO_X] = (uint8_t)(old_x - 1);
        if (isPositionValid(nes) != 0) {
            nes->ram[ADDR_TETRIMINO_X] = nes->ram[ADDR_ORIGINAL_Y];
            nes->ram[ADDR_AUTOREPEAT_X] = DAS_RESET_NTSC;
            return;
        }
        nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = 0x03;
        return;
    }
}

void updatePlayfield(NES_State *nes) {
    if (!nes) return;

    int16_t row = (int16_t)nes->ram[ADDR_TETRIMINO_Y] - 2;
    if (row < 0) {
        row = 0;
    }
    if ((uint8_t)row < nes->ram[ADDR_VRAM_ROW]) {
        nes->ram[ADDR_VRAM_ROW] = (uint8_t)row;
    }
}

void drop_tetrimino(NES_State *nes) {
    if (!nes) return;

    int8_t auto_y = (int8_t)nes->ram[ADDR_AUTOREPEAT_Y];
    uint8_t pressed = nes->ram[ADDR_NEWLY_PRESSED_BUTTONS];
    uint8_t held = nes->ram[ADDR_HELD_BUTTONS];

    if (auto_y < 0) {
        if (pressed & BUTTON_DOWN) {
            nes->ram[ADDR_AUTOREPEAT_Y] = 0;
            auto_y = 0;
        } else {
            nes->ram[ADDR_AUTOREPEAT_Y]++;
            return;
        }
    }

    if (auto_y == 0) {
        if (!(held & (BUTTON_LEFT | BUTTON_RIGHT))) {
            if ((pressed & BUTTON_DPAD) == BUTTON_DOWN) {
                nes->ram[ADDR_AUTOREPEAT_Y] = 1;
            }
        }
    } else {
        if ((held & BUTTON_DPAD) != BUTTON_DOWN) {
            nes->ram[ADDR_AUTOREPEAT_Y] = 0;
            nes->ram[ADDR_HOLD_DOWN_POINTS] = 0;
        } else {
            uint8_t ay = (uint8_t)(nes->ram[ADDR_AUTOREPEAT_Y] + 1);
            nes->ram[ADDR_AUTOREPEAT_Y] = ay;
            if (ay >= 3) {
                nes->ram[ADDR_AUTOREPEAT_Y] = 1;
                nes->ram[ADDR_HOLD_DOWN_POINTS]++;

                /* Force drop 1 cell */
                nes->ram[ADDR_FALL_TIMER] = 0;
                uint8_t old_y = nes->ram[ADDR_TETRIMINO_Y];
                nes->ram[ADDR_ORIGINAL_Y] = old_y;
                nes->ram[ADDR_TETRIMINO_Y] = (uint8_t)(old_y + 1);
                if (isPositionValid(nes) == 0) {
                    return;
                }
                /* Piece locked */
                nes->ram[ADDR_TETRIMINO_Y] = nes->ram[ADDR_ORIGINAL_Y];
                nes->ram[ADDR_PLAY_STATE] = PLAYSTATE_LOCK_TETRIMINO;
                updatePlayfield(nes);
                return;
            }
        }
    }

    /* Fall timer lookup */
    uint8_t drop_spd = getDropSpeed(nes->ram[ADDR_LEVEL_NUMBER]);
    nes->ram[ADDR_DROP_SPEED] = drop_spd;
    if (nes->ram[ADDR_FALL_TIMER] >= drop_spd) {
        nes->ram[ADDR_FALL_TIMER] = 0;
        uint8_t old_y = nes->ram[ADDR_TETRIMINO_Y];
        nes->ram[ADDR_ORIGINAL_Y] = old_y;
        nes->ram[ADDR_TETRIMINO_Y] = (uint8_t)(old_y + 1);
        if (isPositionValid(nes) == 0) {
            return;
        }
        /* Piece locked */
        nes->ram[ADDR_TETRIMINO_Y] = nes->ram[ADDR_ORIGINAL_Y];
        nes->ram[ADDR_PLAY_STATE] = PLAYSTATE_LOCK_TETRIMINO;
        updatePlayfield(nes);
    }
}

/*
 * Orientation to sprite index table for next piece preview (19 bytes at $8BE5).
 * Maps spawn orientations to sprite indices in oamContentLookup.
 * Source address: $8BE5 (main.asm lines 1847-1850).
 */
const uint8_t orientationToSpriteTable[19] = {
    0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x09,
    0x08, 0x00, 0x0B, 0x07, 0x00, 0x00, 0x0A, 0x00,
    0x00, 0x00, 0x0C
};

/*
 * Stages the 4 minos of the current active tetromino into OAM staging ($0200).
 * Calculates screen coordinates from tetriminoX and tetriminoY.
 * Applies 1P vs 2P offsets and clips minos with Y < $2F (transparent tile 0xFF).
 * Source address: $8A0A (main.asm lines 1668-1758).
 */
void stageSpriteForCurrentPiece(NES_State *nes) {
    if (!nes) return;

    uint8_t x = nes->ram[ADDR_TETRIMINO_X];
    uint8_t x_center = (uint8_t)((x << 3) + 0x60);
    nes->ram[ADDR_GENERAL_COUNTER3] = x_center;

    if (nes->ram[ADDR_NUMBER_OF_PLAYERS] != 1) {
        x_center = (uint8_t)(x_center - 0x40);
        nes->ram[ADDR_GENERAL_COUNTER3] = x_center;
        if (nes->ram[ADDR_ACTIVE_PLAYER] != 1) {
            x_center = (uint8_t)(x_center + 0x6F);
            nes->ram[ADDR_GENERAL_COUNTER3] = x_center;
        }
    }

    uint8_t y = nes->ram[ADDR_TETRIMINO_Y];
    uint8_t y_center = (uint8_t)((y << 3) + 0x2F);
    nes->ram[ADDR_GENERAL_COUNTER4] = y_center;

    uint8_t piece = nes->ram[ADDR_CURRENT_PIECE];
    nes->ram[ADDR_GENERAL_COUNTER5] = piece;
    nes->ram[ADDR_GENERAL_COUNTER] = (uint8_t)(piece << 2);

    uint8_t tbl_idx = (uint8_t)(piece * 12);
    uint8_t oam_len = nes->ram[ADDR_OAM_STAGING_LENGTH];
    nes->ram[ADDR_GENERAL_COUNTER2] = 4;

    for (uint8_t i = 0; i < 4; i++) {
        uint8_t y_offset = orientationTable[tbl_idx++];
        uint8_t mino_y = (uint8_t)((y_offset << 3) + y_center);
        nes->ram[ADDR_OAM_STAGING + oam_len] = mino_y;
        nes->ram[ADDR_ORIGINAL_Y] = mino_y;
        oam_len++;

        uint8_t tile_id = orientationTable[tbl_idx++];
        nes->ram[ADDR_OAM_STAGING + oam_len] = tile_id;
        oam_len++;

        nes->ram[ADDR_OAM_STAGING + oam_len] = 0x02;

        if (mino_y < 0x2F) {
            oam_len++;
            nes->ram[ADDR_OAM_STAGING + oam_len - 2] = 0xFF;
            nes->ram[ADDR_OAM_STAGING + oam_len] = 0x00;
            oam_len++;
            tbl_idx++;
        } else {
            oam_len++;
            uint8_t x_offset = orientationTable[tbl_idx++];
            uint8_t mino_x = (uint8_t)((x_offset << 3) + x_center);
            nes->ram[ADDR_OAM_STAGING + oam_len] = mino_x;
            oam_len++;
        }

        nes->ram[ADDR_OAM_STAGING_LENGTH] = oam_len;
        nes->ram[ADDR_GENERAL_COUNTER2]--;
    }
}

/*
 * Stages the next piece preview sprite into OAM staging.
 * If displayNextPiece ($DF) != 0, returns early.
 * Sets sprite offsets (X = $C8, Y = $77) and loads sprite via loadSpriteIntoOamStaging.
 * Source address: $8BCE (main.asm lines 1832-1846).
 */
void stageSpriteForNextPiece(NES_State *nes) {
    if (!nes) return;

    if (nes->ram[ADDR_DISPLAY_NEXT_PIECE] != 0) {
        return;
    }

    nes->ram[ADDR_SPRITE_X_OFFSET] = 0xC8;
    nes->ram[ADDR_SPRITE_Y_OFFSET] = 0x77;

    uint8_t next = nes->ram[ADDR_NEXT_PIECE];
    uint8_t sprite_idx = (next < 19) ? orientationToSpriteTable[next] : 0;
    nes->ram[ADDR_SPRITE_INDEX_OAM] = sprite_idx;

    loadSpriteIntoOamStaging(nes);
}
