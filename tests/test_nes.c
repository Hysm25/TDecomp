#include <stdio.h>
#include <assert.h>
#include "nes.h"

int test_nes_state(void) {
    NES_State nes;
    nes_init(&nes);

    /* Test 1: RAM initialization */
    for (int i = 0; i < NES_RAM_SIZE; i++) {
        assert(nes.ram[i] == 0);
    }

    /* Test 2: RAM write and read */
    nes_write8(&nes, ADDR_TETRIMINO_X, 5);
    assert(nes_read8(&nes, ADDR_TETRIMINO_X) == 5);
    assert(nes_read_zp(&nes, ADDR_TETRIMINO_X) == 5);

    /* Test 3: RAM mirroring ($0800 - $1FFF mirrors $0000 - $07FF) */
    nes_write8(&nes, 0x0040, 0x42);
    assert(nes_read8(&nes, 0x0840) == 0x42);
    assert(nes_read8(&nes, 0x1040) == 0x42);
    assert(nes_read8(&nes, 0x1840) == 0x42);

    /* Test 4: 16-bit little-endian read / write */
    nes_write16(&nes, ADDR_RNG_SEED, 0xABCD);
    assert(nes_read8(&nes, ADDR_RNG_SEED) == 0xCD);
    assert(nes_read8(&nes, ADDR_RNG_SEED + 1) == 0xAB);
    assert(nes_read16(&nes, ADDR_RNG_SEED) == 0xABCD);

    /* Test 5: RAM pointer accessor */
    uint8_t *playfield = nes_get_ram_ptr(&nes, ADDR_PLAYFIELD);
    assert(playfield != NULL);
    playfield[0] = TILE_1;
    assert(nes_read8(&nes, ADDR_PLAYFIELD) == TILE_1);

    printf("  [PASS] test_nes_state\n");
    return 0;
}
