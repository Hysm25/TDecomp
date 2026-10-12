#include <stdio.h>
#include <assert.h>
#include "nes.h"
#include "math/bcd.h"

int test_bcd_and_scoring(void) {
    NES_State nes;
    nes_init(&nes);

    /* Test 1: bcd_add8 and bcd_sub8 basic */
    uint8_t carry = 0;
    uint8_t res = bcd_add8(0x05, 0x07, &carry);
    assert(res == 0x12);
    assert(carry == 0);
    (void)res;

    res = bcd_add8(0x55, 0x48, &carry);
    assert(res == 0x03);
    assert(carry == 1);
    (void)res;

    uint8_t borrow = 0;
    res = bcd_sub8(0x23, 0x05, &borrow);
    assert(res == 0x18);
    assert(borrow == 0);
    (void)res;

    /* Test 2: addHoldDownPoints */
    nes_init(&nes);
    nes.ram[ADDR_SCORE] = 0x10;
    nes.ram[ADDR_HOLD_DOWN_POINTS] = 1;
    addHoldDownPoints(&nes);
    /* holdDownPoints < 2 -> no drop points added */
    assert(nes.ram[ADDR_SCORE] == 0x10);

    nes.ram[ADDR_HOLD_DOWN_POINTS] = 5; /* 5 - 1 = 4 points */
    addHoldDownPoints(&nes);
    assert(nes.ram[ADDR_SCORE] == 0x14);
    assert((nes.ram[ADDR_OUT_OF_DATE_RENDER_FLAGS] & RENDER_SCORE) != 0);

    /* Test 3: addLineClearPoints */
    /* Single line clear on Level 0: 40 points */
    nes_init(&nes);
    nes.ram[ADDR_LEVEL_NUMBER] = 0;
    nes.ram[ADDR_COMPLETED_LINES] = 1;
    addLineClearPoints(&nes);
    assert(nes.ram[ADDR_SCORE] == 0x40);
    assert(nes.ram[ADDR_SCORE + 1] == 0x00);
    assert(nes.ram[ADDR_SCORE + 2] == 0x00);
    assert(nes.ram[ADDR_COMPLETED_LINES] == 0);

    /* Tetris (4 lines) on Level 9: 1200 * 10 = 12000 points */
    nes_init(&nes);
    nes.ram[ADDR_LEVEL_NUMBER] = 9;
    nes.ram[ADDR_COMPLETED_LINES] = 4;
    addLineClearPoints(&nes);
    /* 12,000 points in 3-byte BCD: byte 0 = 0x00, byte 1 = 0x20, byte 2 = 0x01 */
    assert(nes.ram[ADDR_SCORE] == 0x00);
    assert(nes.ram[ADDR_SCORE + 1] == 0x20);
    assert(nes.ram[ADDR_SCORE + 2] == 0x01);

    /* Max-out condition: score clamps at 999,999 */
    nes.ram[ADDR_SCORE] = 0x00;
    nes.ram[ADDR_SCORE + 1] = 0x90;
    nes.ram[ADDR_SCORE + 2] = 0x99;
    nes.ram[ADDR_LEVEL_NUMBER] = 9;
    nes.ram[ADDR_COMPLETED_LINES] = 4;
    addLineClearPoints(&nes);
    assert(nes.ram[ADDR_SCORE] == 0x99);
    assert(nes.ram[ADDR_SCORE + 1] == 0x99);
    assert(nes.ram[ADDR_SCORE + 2] == 0x99);

    /* Test 4: updateLinesAndLevel in Game Type A */
    nes_init(&nes);
    nes.ram[ADDR_GAME_TYPE] = GAMETYPE_A;
    nes.ram[ADDR_LINES] = 0x09;
    nes.ram[ADDR_LEVEL_NUMBER] = 0;
    nes.ram[ADDR_COMPLETED_LINES] = 1;
    updateLinesAndLevel(&nes);
    /* Lines should advance from 9 to 10 (tens boundary!) */
    assert(nes.ram[ADDR_LINES] == 0x10);
    /* Level should advance from 0 to 1! */
    assert(nes.ram[ADDR_LEVEL_NUMBER] == 1);
    assert(nes.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == 0x06);
    assert(nes.ram[ADDR_LINE_CLEAR_STATS_BY_TYPE + 0] == 0x01);

    /* Test 5: updateMusicSpeed and allegro detection */
    nes_init(&nes);
    nes_write16(&nes, ADDR_PLAYFIELD_ADDR, ADDR_PLAYFIELD);
    nes.ram[ADDR_MUSIC_TYPE] = 0; /* Music 1 */
    /* Empty playfield: music remains normal */
    for (int i = 0; i < PLAYFIELD_SIZE; i++) {
        nes.ram[ADDR_PLAYFIELD + i] = TILE_EMPTY;
    }
    updateMusicSpeed(&nes);
    assert(nes.ram[ADDR_ALLEGRO] == 0x00);

    /* Place block at row 5, col 3: offset 5 * 10 + 3 = 53 */
    nes.ram[ADDR_PLAYFIELD + 53] = TILE_1;
    updateMusicSpeed(&nes);
    assert(nes.ram[ADDR_ALLEGRO] == 0xFF);
    /* musicType 0 + 4 = 4 -> musicSelectionTable[4] = 0x06 */
    assert(nes.ram[ADDR_MUSIC_TRACK] == 0x06);

    /* Remove block: allegro resets */
    nes.ram[ADDR_PLAYFIELD + 53] = TILE_EMPTY;
    updateMusicSpeed(&nes);
    assert(nes.ram[ADDR_ALLEGRO] == 0x00);
    /* musicSelectionTable[0] = 0x03 */
    assert(nes.ram[ADDR_MUSIC_TRACK] == 0x03);

    printf("  [PASS] test_bcd_and_scoring\n");
    return 0;
}
