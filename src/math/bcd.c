#include "math/bcd.h"

/* Point values: index 0 = 0, index 1 = 40, index 2 = 100, index 3 = 300, index 4 = 1200 */
/* Source: main.asm lines 3529-3531 */
const uint16_t pointsTable[5] = {
    0x0000,
    0x0040,
    0x0100,
    0x0300,
    0x1200
};

/* Music selection table: normal (indices 0..3) and allegro/fast (indices 4..7) */
/* Source: main.asm line 1076 */
const uint8_t musicSelectionTable[8] = {
    0x03, 0x04, 0x05, 0xFF,
    0x06, 0x07, 0x08, 0xFF
};

uint8_t bcd_add8(uint8_t a, uint8_t b, uint8_t *carry) {
    uint8_t cin = carry ? *carry : 0;
    uint16_t sum = (uint16_t)a + (uint16_t)b + cin;
    uint8_t lo = (uint8_t)((a & 0x0F) + (b & 0x0F) + cin);

    if (lo >= 0x0A) {
        sum += 0x06;
    }
    if (sum >= 0xA0) {
        sum += 0x60;
        if (carry) *carry = 1;
    } else {
        if (carry) *carry = 0;
    }
    return (uint8_t)(sum & 0xFF);
}

uint8_t bcd_sub8(uint8_t a, uint8_t b, uint8_t *borrow) {
    uint8_t bin = borrow ? *borrow : 0;
    int16_t diff = (int16_t)a - (int16_t)b - bin;

    if (diff < 0) {
        diff -= 0x06;
        if (borrow) *borrow = 1;
    } else {
        if ((a & 0x0F) < (b & 0x0F) + bin) {
            diff -= 0x06;
        }
        if (borrow) *borrow = 0;
    }
    return (uint8_t)(diff & 0xFF);
}

void setMusicTrack(NES_State *nes, uint8_t track) {
    if (!nes) return;
    nes->ram[ADDR_MUSIC_TRACK] = track;
    if (nes->ram[ADDR_GAME_MODE] == MODE_PLAY_ENDING_HIGH_SCORE) {
        nes->ram[ADDR_MUSIC_TRACK] = 0xFF;
    }
}

void updateMusicSpeed(NES_State *nes) {
    if (!nes) return;

    /* Base playfield address (default $0400) */
    uint16_t pf_base = nes_read16(nes, ADDR_PLAYFIELD_ADDR);
    if (pf_base < 0x0400 || pf_base >= 0x0700) {
        pf_base = ADDR_PLAYFIELD;
    }

    /* Check row 5: offset 5 * 10 = 50 */
    bool found_block = false;
    for (uint8_t col = 0; col < PLAYFIELD_WIDTH; col++) {
        uint8_t tile = nes_read8(nes, (uint16_t)(pf_base + 50 + col));
        if (tile != TILE_EMPTY) {
            found_block = true;
            break;
        }
    }

    uint8_t allegro = nes->ram[ADDR_ALLEGRO];
    uint8_t music_type = nes->ram[ADDR_MUSIC_TYPE];
    if (music_type >= 4) music_type = 3;

    if (found_block) {
        if (allegro == 0) {
            nes->ram[ADDR_ALLEGRO] = 0xFF;
            uint8_t track = musicSelectionTable[music_type + 4];
            setMusicTrack(nes, track);
        }
    } else {
        if (allegro != 0) {
            nes->ram[ADDR_ALLEGRO] = 0x00;
            uint8_t track = musicSelectionTable[music_type];
            setMusicTrack(nes, track);
        }
    }
}

void addHoldDownPoints(NES_State *nes) {
    if (!nes) return;

    uint8_t hold_points = nes->ram[ADDR_HOLD_DOWN_POINTS];
    if (hold_points >= 2) {
        /* Add (hold_points - 1) to score in BCD */
        uint8_t dec_score = (uint8_t)(nes->ram[ADDR_SCORE] - 1);
        uint16_t sum = (uint16_t)hold_points + (uint16_t)dec_score;

        if ((sum & 0x0F) >= 0x0A) {
            sum += 0x06;
        }
        if ((sum & 0xF0) >= 0xA0) {
            sum += 0x60;
            nes->ram[ADDR_SCORE + 1]++;
        }
        nes->ram[ADDR_SCORE] = (uint8_t)(sum & 0xFF);
        nes->ram[ADDR_OUT_OF_DATE_RENDER_FLAGS] |= RENDER_SCORE;
    }

    addLineClearPoints(nes);
}

void addLineClearPoints(NES_State *nes) {
    if (!nes) return;

    nes->ram[ADDR_HOLD_DOWN_POINTS] = 0;
    uint8_t completed = nes->ram[ADDR_COMPLETED_LINES];
    if (completed > 4) completed = 4;

    uint8_t loops = (uint8_t)(nes->ram[ADDR_LEVEL_NUMBER] + 1);
    uint16_t pts = pointsTable[completed];
    uint8_t pts_lo = LOW_BYTE(pts);
    uint8_t pts_hi = HIGH_BYTE(pts);

    for (uint8_t i = 0; i < loops; i++) {
        /* Byte 0 */
        uint16_t s0 = (uint16_t)nes->ram[ADDR_SCORE] + pts_lo;
        if (s0 >= 0xA0) {
            s0 += 0x60;
            nes->ram[ADDR_SCORE + 1]++;
        }
        nes->ram[ADDR_SCORE] = (uint8_t)(s0 & 0xFF);

        /* Byte 1 */
        uint16_t s1 = (uint16_t)nes->ram[ADDR_SCORE + 1] + pts_hi;
        if ((s1 & 0x0F) >= 0x0A) {
            s1 += 0x06;
        }
        if ((s1 & 0xF0) >= 0xA0) {
            s1 += 0x60;
            nes->ram[ADDR_SCORE + 2]++;
        }
        nes->ram[ADDR_SCORE + 1] = (uint8_t)(s1 & 0xFF);

        /* Byte 2 */
        uint8_t s2 = nes->ram[ADDR_SCORE + 2];
        if ((s2 & 0x0F) >= 0x0A) {
            s2 += 0x06;
        }
        if ((s2 & 0xF0) >= 0xA0) {
            /* Max-out score at 999,999 */
            nes->ram[ADDR_SCORE] = 0x99;
            nes->ram[ADDR_SCORE + 1] = 0x99;
            nes->ram[ADDR_SCORE + 2] = 0x99;
            break;
        }
        nes->ram[ADDR_SCORE + 2] = s2;
    }

    nes->ram[ADDR_OUT_OF_DATE_RENDER_FLAGS] |= RENDER_SCORE;
    nes->ram[ADDR_COMPLETED_LINES] = 0;
    nes->ram[ADDR_PLAY_STATE]++;
}

void updateLinesAndLevel(NES_State *nes) {
    if (!nes) return;

    uint8_t completed = nes->ram[ADDR_COMPLETED_LINES];
    if (completed == 0) {
        addHoldDownPoints(nes);
        return;
    }

    /* Update line clear statistics by type (single, double, triple, tetris) */
    if (completed <= 4) {
        uint8_t idx = (uint8_t)(completed - 1);
        uint8_t stat = (uint8_t)(nes->ram[ADDR_LINE_CLEAR_STATS_BY_TYPE + idx] + 1);
        if ((stat & 0x0F) >= 0x0A) {
            stat += 0x06;
        }
        nes->ram[ADDR_LINE_CLEAR_STATS_BY_TYPE + idx] = stat;
    }
    nes->ram[ADDR_OUT_OF_DATE_RENDER_FLAGS] |= RENDER_LINES;

    if (nes->ram[ADDR_GAME_TYPE] == GAMETYPE_A) {
        for (uint8_t i = 0; i < completed; i++) {
            uint8_t l0 = (uint8_t)(nes->ram[ADDR_LINES] + 1);
            if ((l0 & 0x0F) >= 0x0A) {
                l0 += 0x06;
            }
            if ((l0 & 0xF0) >= 0xA0) {
                l0 &= 0x0F;
                nes->ram[ADDR_LINES + 1]++;
            }
            nes->ram[ADDR_LINES] = l0;

            /* Check level increment on tens boundary */
            if ((l0 & 0x0F) == 0) {
                uint8_t total_tens = (uint8_t)((nes->ram[ADDR_LINES + 1] << 4) | (l0 >> 4));
                if (nes->ram[ADDR_LEVEL_NUMBER] < total_tens) {
                    nes->ram[ADDR_LEVEL_NUMBER]++;
                    nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = 0x06; /* Level up SFX */
                    nes->ram[ADDR_OUT_OF_DATE_RENDER_FLAGS] |= RENDER_LEVEL;
                }
            }
        }
    } else {
        /* Game Type B: lines decrements towards 0 */
        int16_t rem = (int16_t)nes->ram[ADDR_LINES] - (int16_t)completed;
        if (rem <= 0) {
            nes->ram[ADDR_LINES] = 0;
        } else {
            uint8_t l0 = (uint8_t)rem;
            if ((l0 & 0x0F) >= 0x0A) {
                l0 -= 0x06;
            }
            nes->ram[ADDR_LINES] = l0;
        }
    }

    addHoldDownPoints(nes);
}

void playState_updateLinesAndStatistics(NES_State *nes) {
    if (!nes) return;
    updateMusicSpeed(nes);
    updateLinesAndLevel(nes);
}

/*
 * Adds 100 points in BCD to totalScore ($00DD / $00DE) with carry handling.
 * Used for Type B ending score calculation before high score evaluation.
 * Source address: $9F62 (main.asm lines 3945-3973).
 */
void add100Points(NES_State *nes) {
    if (!nes) return;

    uint8_t mid = (uint8_t)(nes->ram[ADDR_TOTAL_SCORE + 1] + 1);
    nes->ram[ADDR_TOTAL_SCORE + 1] = mid;

    if ((mid & 0x0F) >= 0x0A) {
        mid = (uint8_t)(mid + 0x06);
        nes->ram[ADDR_TOTAL_SCORE + 1] = mid;
    }

    if ((mid & 0xF0) >= 0xA0) {
        mid = (uint8_t)(mid + 0x60);
        nes->ram[ADDR_TOTAL_SCORE + 1] = mid;
        nes->ram[ADDR_TOTAL_SCORE + 2]++;
    }

    uint8_t hi = nes->ram[ADDR_TOTAL_SCORE + 2];
    if ((hi & 0x0F) >= 0x0A) {
        hi = (uint8_t)(hi + 0x06);
        nes->ram[ADDR_TOTAL_SCORE + 2] = hi;
    }
}
