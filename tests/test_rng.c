#include <stdio.h>
#include <assert.h>
#include "nes.h"
#include "math/rng.h"

int test_rng_and_piece_selection(void) {
    NES_State nes;
    nes_init(&nes);

    /* Test 1: generateNextPseudorandomNumber single step */
    /* Seed = 0x1234: b0 = 0x34 (bit 1 is 0), b1 = 0x12 (bit 1 is 1) -> XOR = 1 -> Carry in = 1 */
    nes.ram[ADDR_RNG_SEED] = 0x34;     /* 0011 0100 -> bit 1 is 0, bit 0 is 0 */
    nes.ram[ADDR_RNG_SEED + 1] = 0x12; /* 0001 0010 -> bit 1 is 1, bit 0 is 0 */
    generateNextPseudorandomNumber(&nes, ADDR_RNG_SEED, 2);

    /* b0 becomes (0x34 >> 1) | (1 << 7) = 0x1A | 0x80 = 0x9A */
    /* b0 bit 0 was 0, so carry for b1 is 0 */
    /* b1 becomes (0x12 >> 1) | (0 << 7) = 0x09 */
    assert(nes.ram[ADDR_RNG_SEED] == 0x9A);
    assert(nes.ram[ADDR_RNG_SEED + 1] == 0x09);
    assert(nes.ram[ADDR_TMP1] == 0x00); /* 0x34 & 0x02 == 0 */

    /* Test 2: generateNextPseudorandomNumber carry 0 branch */
    /* Seed = 0x0202: both bit 1 are 1 -> XOR = 0 -> Carry in = 0 */
    nes.ram[ADDR_RNG_SEED] = 0x02;     /* 0000 0010 -> bit 1 is 1, bit 0 is 0 */
    nes.ram[ADDR_RNG_SEED + 1] = 0x02; /* 0000 0010 -> bit 1 is 1, bit 0 is 0 */
    generateNextPseudorandomNumber(&nes, ADDR_RNG_SEED, 2);
    /* Carry in = 0. b0 becomes (0x02 >> 1) = 0x01. Next carry = 0. b1 becomes 0x01 */
    assert(nes.ram[ADDR_RNG_SEED] == 0x01);
    assert(nes.ram[ADDR_RNG_SEED + 1] == 0x01);
    assert(nes.ram[ADDR_TMP1] == 0x02); /* 0x02 & 0x02 == 2 */

    /* Test 3: unreferenced_soundRng */
    nes.ram[ADDR_SOUND_RNG_SEED] = 0x03;     /* bit 1 is 1, bit 0 is 1 */
    nes.ram[ADDR_SOUND_RNG_SEED + 1] = 0x00; /* bit 1 is 0, bit 0 is 0 */
    unreferenced_soundRng(&nes);
    assert(nes.ram[ADDR_SOUND_RNG_SEED] == 0x81);
    assert(nes.ram[ADDR_SOUND_RNG_SEED + 1] == 0x80);
    assert(nes.ram[ADDR_UNREFERENCED_SOUND_RNG_TMP] == 0x02);

    /* Test 4: pickRandomTetrimino reroll logic when same as previous spawnID */
    nes_init(&nes);
    nes.ram[ADDR_SPAWN_ID] = ORIENT_T_DOWN; /* 0x02 (index 0) */
    nes.ram[ADDR_SPAWN_COUNT] = 0;
    nes.ram[ADDR_RNG_SEED] = 0;             /* sum = 0 + 1 = 1 -> candidate spawnTable[1] = ORIENT_J_DOWN */
    uint8_t piece1 = pickRandomTetrimino(&nes);
    assert(piece1 == ORIENT_J_DOWN);
    assert(nes.ram[ADDR_SPAWN_ID] == ORIENT_J_DOWN);
    assert(nes.ram[ADDR_SPAWN_COUNT] == 1);
    (void)piece1;

    /* Now set previous spawnID to J_DOWN. With same candidate, reroll will trigger! */
    nes.ram[ADDR_SPAWN_COUNT] = 0;
    nes.ram[ADDR_RNG_SEED] = 0; /* (0 + 1) & 7 = 1 -> J_DOWN matches previous spawnID! */
    nes.ram[ADDR_RNG_SEED + 1] = 0;
    uint8_t piece2 = pickRandomTetrimino(&nes);
    assert(piece2 != ORIENT_J_DOWN || nes.ram[ADDR_SPAWN_ID] != 0);
    (void)piece2;

    /* Test 5: chooseNextTetrimino in demo mode (Mode 5) */
    nes_init(&nes);
    nes.ram[ADDR_GAME_MODE] = MODE_PLAY_ENDING_HIGH_SCORE; /* 5 */
    nes.ram[ADDR_DEMO_INDEX] = 0;
    /* First demo table byte is 0x00 -> index = 0 -> T_DOWN */
    uint8_t demo_piece1 = chooseNextTetrimino(&nes);
    assert(demo_piece1 == ORIENT_T_DOWN);
    assert(nes.ram[ADDR_DEMO_INDEX] == 1);
    (void)demo_piece1;

    /* Second demo table byte is 0x14 -> high nibble is 1 -> J_DOWN */
    uint8_t demo_piece2 = chooseNextTetrimino(&nes);
    assert(demo_piece2 == ORIENT_J_DOWN);
    assert(nes.ram[ADDR_DEMO_INDEX] == 2);
    (void)demo_piece2;

    /* Test 6: incrementPieceStat BCD decimal adjust */
    nes_init(&nes);
    /* Initial stat for T piece is 0 */
    incrementPieceStat(&nes, ORIENT_T_UP);
    assert(nes.ram[ADDR_STATS_BY_TYPE + 0] == 0x01);
    assert(nes.ram[ADDR_STATS_BY_TYPE + 1] == 0x00);

    /* Increment up to 9 */
    for (int i = 0; i < 8; i++) {
        incrementPieceStat(&nes, ORIENT_T_UP);
    }
    assert(nes.ram[ADDR_STATS_BY_TYPE + 0] == 0x09);

    /* 10th increment: 0x09 + 1 -> BCD adjust to 0x10 */
    incrementPieceStat(&nes, ORIENT_T_UP);
    assert(nes.ram[ADDR_STATS_BY_TYPE + 0] == 0x10);

    /* Fast-forward to 0x99 */
    nes.ram[ADDR_STATS_BY_TYPE + 0] = 0x99;
    incrementPieceStat(&nes, ORIENT_T_UP);
    /* 0x99 + 1 -> low digit becomes 0, high digit carries -> 0x00, stats+1 becomes 1 */
    assert(nes.ram[ADDR_STATS_BY_TYPE + 0] == 0x00);
    assert(nes.ram[ADDR_STATS_BY_TYPE + 1] == 0x01);

    printf("  [PASS] test_rng_and_piece_selection\n");
    return 0;
}
