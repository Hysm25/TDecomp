#include "nes.h"

void nes_init(NES_State *nes) {
    if (!nes) return;
    memset(nes, 0, sizeof(NES_State));
}

bool nes_load_rom(NES_State *nes, const uint8_t *data, size_t size) {
    if (!nes || !data) return false;

    /* Handle standard 16-byte iNES header */
    size_t prg_offset = 0;
    if (size >= 16 && data[0] == 'N' && data[1] == 'E' && data[2] == 'S' && data[3] == 0x1A) {
        prg_offset = 16;
    }

    if (size < prg_offset + NES_PRG_SIZE) {
        return false;
    }

    memcpy(nes->prg_rom, data + prg_offset, NES_PRG_SIZE);

    if (size >= prg_offset + NES_PRG_SIZE + NES_CHR_SIZE) {
        memcpy(nes->chr_rom, data + prg_offset + NES_PRG_SIZE, NES_CHR_SIZE);
    }

    return true;
}

uint8_t nes_read8(const NES_State *nes, uint16_t addr) {
    if (!nes) return 0;

    /* CPU RAM: $0000 - $07FF, mirrored to $1FFF */
    if (addr < 0x2000) {
        return nes->ram[addr & 0x07FF];
    }

    /* PPU Registers: $2000 - $2007, mirrored to $3FFF */
    if (addr < 0x4000) {
        uint16_t ppu_reg = 0x2000 + (addr & 0x0007);
        if (ppu_reg == REG_PPUSTATUS) {
            return nes->ppu_status;
        }
        return 0;
    }

    /* APU / I/O registers: $4000 - $401F */
    if (addr < 0x4020) {
        if (addr == REG_JOY1) {
            uint8_t val = ((NES_State *)nes)->joy1_shift & 1;
            ((NES_State *)nes)->joy1_shift = (uint8_t)((((NES_State *)nes)->joy1_shift >> 1) | 0x80);
            return val;
        }
        if (addr == REG_JOY2_APUFC) {
            uint8_t val = ((NES_State *)nes)->joy2_shift & 1;
            ((NES_State *)nes)->joy2_shift = (uint8_t)((((NES_State *)nes)->joy2_shift >> 1) | 0x80);
            return val;
        }
        return nes->apu_regs[addr - 0x4000];
    }

    /* PRG ROM: $8000 - $FFFF */
    if (addr >= 0x8000) {
        return nes->prg_rom[addr - 0x8000];
    }

    return 0;
}

void nes_write8(NES_State *nes, uint16_t addr, uint8_t val) {
    if (!nes) return;

    /* CPU RAM: $0000 - $07FF, mirrored to $1FFF */
    if (addr < 0x2000) {
        nes->ram[addr & 0x07FF] = val;
        return;
    }

    /* PPU Registers: $2000 - $2007, mirrored to $3FFF */
    if (addr < 0x4000) {
        uint16_t ppu_reg = 0x2000 + (addr & 0x0007);
        switch (ppu_reg) {
            case REG_PPUCTRL: nes->ppu_ctrl = val; break;
            case REG_PPUMASK: nes->ppu_mask = val; break;
            case REG_OAMADDR: nes->oam_addr = val; break;
            case REG_PPUSCROLL:
                /* Scroll alternates X / Y */
                break;
            case REG_PPUADDR:
                nes->ppu_addr = (uint16_t)((nes->ppu_addr << 8) | val);
                break;
            default:
                break;
        }
        return;
    }

    /* APU / I/O Registers: $4000 - $401F */
    if (addr < 0x4020) {
        nes->apu_regs[addr - 0x4000] = val;
        if (addr == REG_JOY1) {
            if (val & 1) {
                nes->joy1_shift = nes->joy1_latch;
                nes->joy2_shift = nes->joy2_latch;
            }
        }
        return;
    }

    /* PRG ROM writes (e.g. MMC1 control registers: $8000 - $FFFF) */
    /* Handled by MMC1 driver when required */
}

uint16_t nes_read16(const NES_State *nes, uint16_t addr) {
    uint8_t lo = nes_read8(nes, addr);
    uint8_t hi = nes_read8(nes, (uint16_t)(addr + 1));
    return MAKE_WORD(lo, hi);
}

void nes_write16(NES_State *nes, uint16_t addr, uint16_t val) {
    nes_write8(nes, addr, LOW_BYTE(val));
    nes_write8(nes, (uint16_t)(addr + 1), HIGH_BYTE(val));
}

uint8_t *nes_get_ram_ptr(NES_State *nes, uint16_t addr) {
    if (!nes || addr >= 0x0800) return NULL;
    return &nes->ram[addr];
}

const uint8_t *nes_get_ram_const_ptr(const NES_State *nes, uint16_t addr) {
    if (!nes || addr >= 0x0800) return NULL;
    return &nes->ram[addr];
}
