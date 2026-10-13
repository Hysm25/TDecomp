#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "nes.h"
#include "game/system.h"
#include "game/highscore.h"
#include "constants/memory.h"
#include "constants/hardware.h"
#include "constants/gameplay.h"

static void test_mmc1_registers(void) {
    NES_State nes;
    nes_init(&nes);

    /* 1. Test setMMC1Control ($ACA3): write 0x10 */
    setMMC1Control(&nes, 0x10);
    assert(nes.mmc1_control == 0x10);
    assert(nes.mmc1_shift_count == 0);

    /* 2. Test changeCHRBank0 ($ACB7): write 0x01 */
    changeCHRBank0(&nes, 0x01);
    assert(nes.mmc1_chr0 == 0x01);
    assert(nes.mmc1_shift_count == 0);

    /* 3. Test changeCHRBank1 ($ACCB): write 0x02 */
    changeCHRBank1(&nes, 0x02);
    assert(nes.mmc1_chr1 == 0x02);
    assert(nes.mmc1_shift_count == 0);

    /* 4. Test changePRGBank ($ACDF): write 0x03 */
    changePRGBank(&nes, 0x03);
    assert(nes.mmc1_prg == 0x03);
    assert(nes.mmc1_shift_count == 0);

    /* 5. Test MMC1 reset write with bit 7 set */
    nes_write8(&nes, 0x8000, 0x01); /* write 1 bit into shift */
    assert(nes.mmc1_shift_count == 1);
    nes_write8(&nes, 0x8000, 0x80); /* reset write with bit 7 */
    assert(nes.mmc1_shift_count == 0);
    assert((nes.mmc1_control & 0x0C) == 0x0C);
}

static void test_ppu_memset_and_laa82(void) {
    NES_State nes;
    nes_init(&nes);

    /* Test memset_ppu_page_and_more with nametable 0 ($2000) */
    memset_ppu_page_and_more(&nes, 0x20, 0xAA, 0x55);
    assert(nes.ram[ADDR_CURRENT_PPU_CTRL] == (nes.ppu_ctrl & 0xFB));
    /* PPU address was set to attribute table start: $23C0 */
    assert(nes.ppu_addr == 0x23C0);

    /* Test LAA82: clears with tile $FF and attr $00 (attr table at $27C0) */
    LAA82(&nes, 0x24);
    assert(nes.ppu_addr == 0x27C0);

    /* Test memset_ppu_page_and_more with pattern table (< $20) */
    memset_ppu_page_and_more(&nes, 0x10, 0x11, 2);
    assert(nes.ppu_addr == 0x1000);
}

static void test_bulk_copy_to_ppu(void) {
    NES_State nes;
    nes_init(&nes);

    /* Stage test packet stream in CPU RAM:
     * Packet 1: PPU $2000, horiz, literal 2 bytes: 'A', 'B'
     * Packet 2: PPU $2010, horiz, RLE 4 bytes of 'X'
     * Terminator: $FF
     */
    uint16_t stream = 0x0300;
    nes.ram[0x0300] = 0x20; /* PPU HI */
    nes.ram[0x0301] = 0x00; /* PPU LO */
    nes.ram[0x0302] = 0x02; /* ctrl_len: horiz, literal, count 2 */
    nes.ram[0x0303] = 0x41; /* 'A' */
    nes.ram[0x0304] = 0x42; /* 'B' */

    nes.ram[0x0305] = 0x20; /* PPU HI */
    nes.ram[0x0306] = 0x10; /* PPU LO */
    nes.ram[0x0307] = 0x44; /* ctrl_len: horiz, RLE (bit 6), count 4 */
    nes.ram[0x0308] = 0x58; /* 'X' */

    nes.ram[0x0309] = 0xFF; /* terminator */

    bulkCopyToPpu(&nes, stream);
    /* Last PPUADDR set was packet 2 ($2010) */
    assert(nes.ppu_addr == 0x2010);

    /* Palette reset test: packet targeting $3F00 */
    nes.ram[0x0310] = 0x3F;
    nes.ram[0x0311] = 0x00;
    nes.ram[0x0312] = 0x01;
    nes.ram[0x0313] = 0x0F;
    nes.ram[0x0314] = 0xFF;

    bulkCopyToPpu(&nes, 0x0310);
    assert(nes.ppu_addr == 0x0000); /* cleared by 4x PPUADDR writes */
}

static void test_nmi_and_irq(void) {
    NES_State nes;
    nes_init(&nes);

    nes.ram[ADDR_OAM_STAGING_LENGTH] = 10;
    nes.ram[ADDR_SLEEP_COUNTER] = 5;
    nes.ram[ADDR_FRAME_COUNTER] = 0xFE;
    nes.ram[ADDR_FRAME_COUNTER + 1] = 0x00;
    nes.ram[ADDR_RNG_SEED] = 0x89;
    nes.ram[ADDR_RNG_SEED + 1] = 0x88;
    nes.ram[ADDR_RENDER_MODE] = 0; /* render_mode_legal_and_title */

    nmi(&nes);

    assert(nes.ram[ADDR_OAM_STAGING_LENGTH] == 0);
    assert(nes.ram[ADDR_SLEEP_COUNTER] == 4);
    assert(nes.ram[ADDR_FRAME_COUNTER] == 0xFF);
    assert(nes.ram[ADDR_FRAME_COUNTER + 1] == 0x00);
    assert(nes.ram[ADDR_VERTICAL_BLANK_INTERVAL] == 1);
    assert(nes.ram[ADDR_PPU_SCROLL_X] == 0);
    assert(nes.ram[ADDR_PPU_SCROLL_Y] == 0);

    /* Sleep counter clamping at 0 */
    nes.ram[ADDR_SLEEP_COUNTER] = 0;
    nmi(&nes);
    assert(nes.ram[ADDR_SLEEP_COUNTER] == 0); /* 0 - 1 = 0xFF -> restored to 0 */

    /* IRQ test */
    nes.irq_pending = true;
    irq(&nes);
    assert(!nes.irq_pending);
}

static void test_sleep_vblanks(void) {
    NES_State nes;
    nes_init(&nes);

    sleep_for_a_vblanks(&nes, 3);
    assert(nes.ram[ADDR_SLEEP_COUNTER] == 0);

    sleep_for_14_vblanks(&nes);
    assert(nes.ram[ADDR_SLEEP_COUNTER] == 0);
}

static void test_init_ram_cold_and_warm(void) {
    NES_State nes;
    nes_init(&nes);

    /* Cold boot test: initMagic not set */
    initRam(&nes);
    assert(nes.ram[ADDR_INIT_MAGIC] == 0x12);
    assert(nes.ram[ADDR_INIT_MAGIC + 1] == 0x34);
    assert(nes.ram[ADDR_INIT_MAGIC + 2] == 0x56);
    assert(nes.ram[ADDR_INIT_MAGIC + 3] == 0x78);
    assert(nes.ram[ADDR_INIT_MAGIC + 4] == 0x9A);
    assert(nes.ram[ADDR_RNG_SEED] == 0x89);
    assert(nes.ram[ADDR_RNG_SEED + 1] == 0x88);
    assert(nes.ram[ADDR_NUMBER_OF_PLAYERS] == 1);
    assert(nes.ram[ADDR_CURRENT_PPU_CTRL] == 0x90);

    /* Warm boot test: corrupt high scores, ensure they are NOT reset if magic present */
    nes.ram[ADDR_HIGH_SCORE_NAMES] = 'Z';
    initRam(&nes);
    assert(nes.ram[ADDR_HIGH_SCORE_NAMES] == 'Z');
}

static void test_reset_bootstrap(void) {
    NES_State nes;
    nes_init(&nes);

    reset(&nes);
    assert(nes.mmc1_control == 0x10);
    assert(nes.mmc1_chr0 == 0x00);
    assert(nes.mmc1_chr1 == 0x00);
    assert(nes.mmc1_prg == 0x00);
    assert(nes.ram[ADDR_INIT_MAGIC] == 0x12);
}

static void test_pal_select_debug(void) {
    NES_State nes;
    nes_init(&nes);

    nes.ram[ADDR_PLAY_STATE] = 1;
    nes.ram[ADDR_SCORE + 2] = 0x05;
    nes.ram[ADDR_P1_SCORE + 2] = 0x05;
    nes.ram[ADDR_HELD_BUTTONS_P1] = BUTTON_SELECT;

    checkSelectHeldToAddPoints(&nes);

    assert(nes.ram[ADDR_SCORE + 2] == 0x06);
    assert(nes.ram[ADDR_P1_SCORE + 2] == 0x06);
    assert((nes.ram[ADDR_OUT_OF_DATE_RENDER_FLAGS] & RENDER_SCORE) != 0);
    assert(nes.ram[ADDR_PLAY_STATE] == 2);
}

int test_system_subsystem(void) {
    test_mmc1_registers();
    test_ppu_memset_and_laa82();
    test_bulk_copy_to_ppu();
    test_nmi_and_irq();
    test_sleep_vblanks();
    test_init_ram_cold_and_warm();
    test_reset_bootstrap();
    test_pal_select_debug();
    printf("--- All System Boot, MMC1 Mapper & PPU Utility Unit Tests Passed ---\n");
    return 0;
}
