#include "game/system.h"
#include "game/render.h"
#include "game/audio.h"
#include "game/input.h"
#include "game/state.h"
#include "game/menu.h"
#include "game/highscore.h"
#include "math/rng.h"
#include "math/bcd.h"
#include <string.h>

/*
 * Authoritative ROM Data Tables in System / Mapper Area
 */
const uint8_t game_palette[36] = {                        /* $ACF3 */
    0x3F, 0x00, 0x20, 0x0F, 0x30, 0x12, 0x16, 0x0F,
    0x20, 0x12, 0x18, 0x0F, 0x2C, 0x16, 0x29, 0x0F,
    0x3C, 0x00, 0x30, 0x0F, 0x35, 0x15, 0x22, 0x0F,
    0x35, 0x29, 0x26, 0x0F, 0x2C, 0x16, 0x29, 0x0F,
    0x3C, 0x00, 0x30, 0xFF
};

const uint8_t legal_screen_palette[20] = {                /* $AD17 */
    0x3F, 0x00, 0x10, 0x0F, 0x27, 0x2A, 0x2B, 0x0F,
    0x3C, 0x2A, 0x22, 0x0F, 0x27, 0x2C, 0x29, 0x0F,
    0x30, 0x3A, 0x15, 0xFF
};

const uint8_t menu_palette[24] = {                        /* $AD2B */
    0x3F, 0x00, 0x14, 0x0F, 0x30, 0x38, 0x00, 0x0F,
    0x30, 0x16, 0x00, 0x0F, 0x30, 0x21, 0x00, 0x0F,
    0x16, 0x2A, 0x28, 0x0F, 0x30, 0x29, 0x27, 0xFF
};

const uint8_t ending_palette[36] = {                      /* $AD43 */
    0x3F, 0x00, 0x20, 0x12, 0x0F, 0x29, 0x37, 0x12,
    0x0F, 0x30, 0x27, 0x12, 0x0F, 0x17, 0x27, 0x12,
    0x0F, 0x15, 0x37, 0x12, 0x0F, 0x29, 0x37, 0x12,
    0x0F, 0x30, 0x27, 0x12, 0x0F, 0x17, 0x27, 0x12,
    0x0F, 0x15, 0x37, 0xFF
};

/*
 * MMC1 Mapper Drivers ($ACA3 - $ACDF)
 */
void setMMC1Control(NES_State *nes, uint8_t val) {
    if (!nes) return;
    for (int i = 0; i < 5; i++) {
        nes_write8(nes, REG_MMC1_CTRL, val);
        val >>= 1;
    }
}

void changeCHRBank0(NES_State *nes, uint8_t val) {
    if (!nes) return;
    for (int i = 0; i < 5; i++) {
        nes_write8(nes, REG_MMC1_CHR0, val);
        val >>= 1;
    }
}

void changeCHRBank1(NES_State *nes, uint8_t val) {
    if (!nes) return;
    for (int i = 0; i < 5; i++) {
        nes_write8(nes, REG_MMC1_CHR1, val);
        val >>= 1;
    }
}

void changePRGBank(NES_State *nes, uint8_t val) {
    if (!nes) return;
    for (int i = 0; i < 5; i++) {
        nes_write8(nes, REG_MMC1_PRG, val);
        val >>= 1;
    }
}

/*
 * PPU Control Helpers ($AA6B - $AA7C)
 */
void _updatePpuCtrl(NES_State *nes, uint8_t ctrl) {
    if (!nes) return;
    nes_write8(nes, REG_PPUCTRL, ctrl);
    nes->ram[ADDR_CURRENT_PPU_CTRL] = ctrl;
}

void waitForVBlankAndEnableNmi(NES_State *nes) {
    if (!nes) return;
    uint8_t ctrl = (uint8_t)(nes->ram[ADDR_CURRENT_PPU_CTRL] | 0x80);
    _updatePpuCtrl(nes, ctrl);
}

void disableNmi(NES_State *nes) {
    if (!nes) return;
    uint8_t ctrl = (uint8_t)(nes->ram[ADDR_CURRENT_PPU_CTRL] & 0x7F);
    _updatePpuCtrl(nes, ctrl);
}

/*
 * PPU Memory Utilities ($AA82, $AC1C)
 */
void memset_ppu_page_and_more(NES_State *nes, uint8_t ppu_hi, uint8_t tile, uint8_t attr_or_count) {
    if (!nes) return;
    nes->ram[ADDR_TMP1] = ppu_hi;
    nes->ram[ADDR_TMP2] = tile;
    nes->ram[ADDR_TMP3] = attr_or_count;

    (void)nes_read8(nes, REG_PPUSTATUS);
    uint8_t ctrl = (uint8_t)(nes->ram[ADDR_CURRENT_PPU_CTRL] & 0xFB);
    nes_write8(nes, REG_PPUCTRL, ctrl);
    nes->ram[ADDR_CURRENT_PPU_CTRL] = ctrl;

    nes_write8(nes, REG_PPUADDR, ppu_hi);
    nes_write8(nes, REG_PPUADDR, 0x00);

    uint8_t pages = (ppu_hi >= 0x20) ? 4 : attr_or_count;
    for (uint16_t i = 0; i < (uint16_t)(pages * 256); i++) {
        nes_write8(nes, REG_PPUDATA, tile);
    }

    if (ppu_hi >= 0x20) {
        /* With carry set from cmp #$20, adc #$02 adds 3 to ppu_hi */
        uint8_t attr_hi = (uint8_t)(ppu_hi + 3);
        nes_write8(nes, REG_PPUADDR, attr_hi);
        nes_write8(nes, REG_PPUADDR, 0xC0);
        for (uint8_t i = 0; i < 0x40; i++) {
            nes_write8(nes, REG_PPUDATA, attr_or_count);
        }
    }
}

void LAA82(NES_State *nes, uint8_t ppu_hi) {
    memset_ppu_page_and_more(nes, ppu_hi, 0xFF, 0x00);
}

/*
 * Bulk PPU Stream Copying ($AA98, $AAF2)
 */
void copyToPpu(NES_State *nes, uint16_t stream_addr) {
    if (!nes) return;
    (void)nes_read8(nes, REG_PPUSTATUS);

    uint16_t curr_addr = stream_addr;
    uint16_t call_stack[8];
    int sp = 0;

    while (1) {
        uint8_t b = nes_read8(nes, curr_addr);
        if (b & 0x80) {
            /* Bit 7 set -> stream terminator */
            return;
        }
        if (b == 0x60) {
            /* Return from sub-stream */
            if (sp > 0) {
                curr_addr = call_stack[--sp];
                curr_addr += 3;
                continue;
            } else {
                return;
            }
        }
        if (b == 0x4C) {
            /* Sub-stream call */
            uint16_t target = (uint16_t)(nes_read8(nes, (uint16_t)(curr_addr + 1)) |
                                        (nes_read8(nes, (uint16_t)(curr_addr + 2)) << 8));
            if (sp < 8) {
                call_stack[sp++] = curr_addr;
            }
            curr_addr = target;
            continue;
        }

        /* Normal packet */
        uint8_t ppu_hi = b;
        uint8_t ppu_lo = nes_read8(nes, (uint16_t)(curr_addr + 1));
        nes_write8(nes, REG_PPUADDR, ppu_hi);
        nes_write8(nes, REG_PPUADDR, ppu_lo);

        uint8_t ctrl_len = nes_read8(nes, (uint16_t)(curr_addr + 2));
        uint8_t ppuctrl = nes->ram[ADDR_CURRENT_PPU_CTRL];
        if (ctrl_len & 0x80) {
            ppuctrl |= 0x04;
        } else {
            ppuctrl &= ~0x04;
        }
        nes_write8(nes, REG_PPUCTRL, ppuctrl);
        nes->ram[ADDR_CURRENT_PPU_CTRL] = ppuctrl;

        bool is_rle = (ctrl_len & 0x40) != 0;
        uint8_t count = (uint8_t)(ctrl_len & 0x3F);

        uint16_t data_ptr = (uint16_t)(curr_addr + 3);
        if (is_rle) {
            uint8_t fill_byte = nes_read8(nes, data_ptr);
            for (uint8_t i = 0; i < count; i++) {
                nes_write8(nes, REG_PPUDATA, fill_byte);
            }
            curr_addr = (uint16_t)(data_ptr + 1);
        } else {
            for (uint8_t i = 0; i < count; i++) {
                uint8_t byte_val = nes_read8(nes, (uint16_t)(data_ptr + i));
                nes_write8(nes, REG_PPUDATA, byte_val);
            }
            curr_addr = (uint16_t)(data_ptr + count);
        }

        if (ppu_hi == 0x3F) {
            nes_write8(nes, REG_PPUADDR, 0x3F);
            nes_write8(nes, REG_PPUADDR, 0x00);
            nes_write8(nes, REG_PPUADDR, 0x00);
            nes_write8(nes, REG_PPUADDR, 0x00);
        }
    }
}

void bulkCopyToPpu(NES_State *nes, uint16_t stream_addr) {
    copyToPpu(nes, stream_addr);
}

void copyAddrAtReturnAddressToTmp_incrReturnAddrBy2(NES_State *nes) {
    (void)nes;
}

/*
 * VBlank Synchronization ($A44D, $A459)
 */
void sleep_for_a_vblanks(NES_State *nes, uint8_t count) {
    if (!nes) return;
    nes->ram[ADDR_SLEEP_COUNTER] = count;
    while (nes->ram[ADDR_SLEEP_COUNTER] != 0) {
        updateAudioWaitForNmiAndResetOamStaging(nes);
        if (nes->ram[ADDR_SLEEP_COUNTER] > 0) {
            nes->ram[ADDR_SLEEP_COUNTER]--;
        }
    }
}

void sleep_for_14_vblanks(NES_State *nes) {
    sleep_for_a_vblanks(nes, 0x14);
}

/*
 * Interrupt Service Routines ($8005, $804A)
 */
void nmi(NES_State *nes) {
    if (!nes) return;
    nes->ram[ADDR_OAM_STAGING_LENGTH] = 0;
    render(nes);

    uint8_t sc = nes->ram[ADDR_SLEEP_COUNTER];
    sc--;
    if (sc == 0xFF) {
        sc++;
    }
    nes->ram[ADDR_SLEEP_COUNTER] = sc;

    copyOamStagingToOam(nes);

    uint16_t fc = (uint16_t)(nes->ram[ADDR_FRAME_COUNTER] | (nes->ram[ADDR_FRAME_COUNTER + 1] << 8));
    fc++;
    nes->ram[ADDR_FRAME_COUNTER] = (uint8_t)(fc & 0xFF);
    nes->ram[ADDR_FRAME_COUNTER + 1] = (uint8_t)(fc >> 8);

    generateNextPseudorandomNumber(nes, ADDR_RNG_SEED, 2);

    nes->ram[ADDR_PPU_SCROLL_X] = 0;
    nes_write8(nes, REG_PPUSCROLL, 0);
    nes->ram[ADDR_PPU_SCROLL_Y] = 0;
    nes_write8(nes, REG_PPUSCROLL, 0);

    nes->ram[ADDR_VERTICAL_BLANK_INTERVAL] = 1;
    pollControllerButtons(nes);

    nes->nmi_pending = false;
}

void irq(NES_State *nes) {
    if (!nes) return;
    nes->irq_pending = false;
}

/*
 * System Boot and RAM Initialization ($FF00, $8000, $805A)
 */
void initRamContinued(NES_State *nes) {
    if (!nes) return;

    /* Zero out CPU RAM pages 0..6 ($0000 - $06FF) */
    memset(nes->ram, 0, 0x0700);

    /* Check initMagic ($0700..$0704) for warm boot */
    bool warm_boot = (nes->ram[ADDR_INIT_MAGIC] == 0x12 &&
                      nes->ram[ADDR_INIT_MAGIC + 1] == 0x34 &&
                      nes->ram[ADDR_INIT_MAGIC + 2] == 0x56 &&
                      nes->ram[ADDR_INIT_MAGIC + 3] == 0x78 &&
                      nes->ram[ADDR_INIT_MAGIC + 4] == 0x9A);
    if (!warm_boot) {
        initHighScoreTable(nes);
    }

    nes->ram[ADDR_RNG_SEED] = 0x89;
    nes->ram[ADDR_RNG_SEED + 1] = 0x88;

    nes->ram[ADDR_PPU_SCROLL_X] = 0;
    nes_write8(nes, REG_PPUSCROLL, 0);
    nes->ram[ADDR_PPU_SCROLL_Y] = 0;
    nes_write8(nes, REG_PPUSCROLL, 0);

    nes->ram[ADDR_CURRENT_PPU_CTRL] = 0x90;
    nes_write8(nes, REG_PPUCTRL, 0x90);
    nes_write8(nes, REG_PPUMASK, 0x06);

    initAudioAndMarkInited(nes);
    soundEffectSlot2_makesNoSound(nes);

    /* Stack page marker values ($01C0..$01C4) */
    nes->ram[0x01C0] = 0xC0;
    nes->ram[0x01C1] = 0x80;
    nes->ram[0x01C3] = 0x35;
    nes->ram[0x01C4] = 0xAC;

    updateAudioWaitForNmiAndDisablePpuRendering(nes);
    disableNmi(nes);

    /* Clear nametables 0, 1, 2, 3 */
    LAA82(nes, 0x20);
    LAA82(nes, 0x24);
    LAA82(nes, 0x28);
    LAA82(nes, 0x2C);

    /* Initialize playfield with empty tiles ($EF) */
    memset_page(nes, TILE_EMPTY, (uint8_t)(ADDR_PLAYFIELD >> 8), (uint8_t)(ADDR_PLAYFIELD_P2 >> 8));

    waitForVBlankAndEnableNmi(nes);
    updateAudioWaitForNmiAndResetOamStaging(nes);
    updateAudioWaitForNmiAndEnablePpuRendering(nes);
    updateAudioWaitForNmiAndResetOamStaging(nes);

    nes->ram[0x000E] = 0x0E;
    nes->ram[ADDR_GAME_MODE_STATE] = 0;
    nes->ram[ADDR_GAME_MODE] = 0;
    nes->ram[ADDR_NUMBER_OF_PLAYERS] = 1;
    nes->ram[ADDR_FRAME_COUNTER + 1] = 0;
}

void initRam(NES_State *nes) {
    initRamContinued(nes);
}

void reset(NES_State *nes) {
    if (!nes) return;
    nes_write8(nes, REG_PPUCTRL, 0);
    nes_write8(nes, REG_PPUMASK, 0);

    /* MMC1 reset write with bit 7 set */
    nes_write8(nes, 0xFF00, 0x80);

    /* MMC1 setup: 4K CHR, 32K PRG, Horizontal mirroring */
    setMMC1Control(nes, 0x10);
    changeCHRBank0(nes, 0x00);
    changeCHRBank1(nes, 0x00);
    changePRGBank(nes, 0x00);

    initRam(nes);
}

void system_step_main_loop(NES_State *nes) {
    if (!nes) return;
    uint8_t old_state = nes->ram[ADDR_GAME_MODE_STATE];
    branchOnGameMode(nes);
    if (nes->ram[ADDR_GAME_MODE_STATE] == old_state) {
        updateAudioWaitForNmiAndResetOamStaging(nes);
    }
    if (nes->ram[ADDR_GAME_MODE] == 0x05) {
        if (nes->ram[ADDR_DEMO_BUTTONS_ADDR + 1] == 0xDF) {
            nes->ram[ADDR_DEMO_BUTTONS_ADDR + 1] = 0xDD;
            nes->ram[ADDR_FRAME_COUNTER + 1] = 0;
            nes->ram[ADDR_GAME_MODE] = 0x01;
        }
    }
}

/*
 * Helper / Control Flow Routines ($AC82, PAL line 4610)
 */
void switch_s_plus_2a(NES_State *nes, uint8_t index, const uint16_t *table) {
    (void)nes;
    (void)index;
    (void)table;
}

void checkSelectHeldToAddPoints(NES_State *nes) {
    if (!nes) return;
    if (nes->ram[ADDR_HELD_BUTTONS_P1] & BUTTON_SELECT) {
        nes->ram[ADDR_SCORE + 2]++;
        nes->ram[ADDR_P1_SCORE + 2]++;
        nes->ram[ADDR_OUT_OF_DATE_RENDER_FLAGS] |= RENDER_SCORE;
    }
    playState_bTypeGoalCheck_ret(nes);
}
