#ifndef TDECOMP_GAME_SYSTEM_H
#define TDECOMP_GAME_SYSTEM_H

#include "common.h"
#include "nes.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * System Boot and RAM Initialization ($FF00, $8000, $805A)
 */
void reset(NES_State *nes);
void initRam(NES_State *nes);
void initRamContinued(NES_State *nes);

/*
 * Interrupt Service Routines ($8005, $804A)
 */
void nmi(NES_State *nes);
void irq(NES_State *nes);

/*
 * VBlank Synchronization ($A44D, $A459)
 */
void sleep_for_14_vblanks(NES_State *nes);
void sleep_for_a_vblanks(NES_State *nes, uint8_t count);

/*
 * MMC1 Mapper Operations ($ACA3 - $ACDF)
 */
void setMMC1Control(NES_State *nes, uint8_t val);
void changeCHRBank0(NES_State *nes, uint8_t val);
void changeCHRBank1(NES_State *nes, uint8_t val);
void changePRGBank(NES_State *nes, uint8_t val);

/*
 * PPU Memory Utilities & Stream Copier ($AA82, $AA98, $AC1C)
 */
void LAA82(NES_State *nes, uint8_t ppu_hi);
void memset_ppu_page_and_more(NES_State *nes, uint8_t ppu_hi, uint8_t tile, uint8_t attr_or_count);
void bulkCopyToPpu(NES_State *nes, uint16_t stream_addr);
void copyToPpu(NES_State *nes, uint16_t stream_addr);
void copyAddrAtReturnAddressToTmp_incrReturnAddrBy2(NES_State *nes);

/*
 * Helper / Control Flow Routines ($AC82, PAL line 4610)
 */
void switch_s_plus_2a(NES_State *nes, uint8_t index, const uint16_t *table);
void checkSelectHeldToAddPoints(NES_State *nes);
void system_step_main_loop(NES_State *nes);

/*
 * Authoritative ROM Data Tables in System / Mapper Area
 */
extern const uint8_t game_palette[36];              /* $ACF3 */
extern const uint8_t legal_screen_palette[20];       /* $AD17 */
extern const uint8_t menu_palette[24];               /* $AD2B */
extern const uint8_t ending_palette[36];             /* $AD43 */

#ifdef __cplusplus
}
#endif

#endif /* TDECOMP_GAME_SYSTEM_H */
