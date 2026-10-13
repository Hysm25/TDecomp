#ifndef TDECOMP_NES_H
#define TDECOMP_NES_H

#include "common.h"
#include "constants/hardware.h"
#include "constants/memory.h"
#include "constants/gameplay.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NES_RAM_SIZE    0x0800  /* 2KB internal CPU RAM */
#define NES_PRG_SIZE    0x8000  /* 32KB PRG ROM */
#define NES_CHR_SIZE    0x4000  /* 16KB CHR ROM */

typedef struct NES_State {
    uint8_t ram[NES_RAM_SIZE];       /* CPU RAM ($0000 - $07FF) */
    uint8_t prg_rom[NES_PRG_SIZE];   /* PRG ROM ($8000 - $FFFF) */
    uint8_t chr_rom[NES_CHR_SIZE];   /* CHR ROM */

    /* PPU state */
    uint8_t ppu_ctrl;
    uint8_t ppu_mask;
    uint8_t ppu_status;
    uint8_t ppu_scroll_x;
    uint8_t ppu_scroll_y;
    uint16_t ppu_addr;
    uint8_t oam_addr;

    /* APU state registers ($4000 - $401F) */
    uint8_t apu_regs[0x20];

    /* Joypad input latch */
    uint8_t joy1_latch;
    uint8_t joy1_shift;
    uint8_t joy2_latch;
    uint8_t joy2_shift;

    /* Frame timing / interrupts */
    bool nmi_pending;
    bool irq_pending;

    /* MMC1 mapper state */
    uint8_t mmc1_shift;
    uint8_t mmc1_shift_count;
    uint8_t mmc1_control;
    uint8_t mmc1_chr0;
    uint8_t mmc1_chr1;
    uint8_t mmc1_prg;
} NES_State;

/* Initialize or reset the NES state */
void nes_init(NES_State *nes);

/* Attach / load PRG and CHR ROM data */
bool nes_load_rom(NES_State *nes, const uint8_t *data, size_t size);

/* Memory read / write operations */
uint8_t nes_read8(const NES_State *nes, uint16_t addr);
void nes_write8(NES_State *nes, uint16_t addr, uint8_t val);

uint16_t nes_read16(const NES_State *nes, uint16_t addr);
void nes_write16(NES_State *nes, uint16_t addr, uint16_t val);

/* Pointer access to internal RAM buffers */
uint8_t *nes_get_ram_ptr(NES_State *nes, uint16_t addr);
const uint8_t *nes_get_ram_const_ptr(const NES_State *nes, uint16_t addr);

/* Fast Zero-Page inlines */
static inline uint8_t nes_read_zp(const NES_State *nes, uint8_t zp_addr) {
    return nes->ram[zp_addr];
}

static inline void nes_write_zp(NES_State *nes, uint8_t zp_addr, uint8_t val) {
    nes->ram[zp_addr] = val;
}

#ifdef __cplusplus
}
#endif

#endif /* TDECOMP_NES_H */
