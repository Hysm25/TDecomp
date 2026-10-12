#ifndef TDECOMP_CONSTANTS_HARDWARE_H
#define TDECOMP_CONSTANTS_HARDWARE_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PPU Memory-Mapped Registers */
#define REG_PPUCTRL     0x2000
#define REG_PPUMASK     0x2001
#define REG_PPUSTATUS   0x2002
#define REG_OAMADDR     0x2003
#define REG_OAMDATA     0x2004
#define REG_PPUSCROLL   0x2005
#define REG_PPUADDR     0x2006
#define REG_PPUDATA     0x2007

/* APU & I/O Registers */
#define REG_SQ1_VOL     0x4000
#define REG_SQ1_SWEEP   0x4001
#define REG_SQ1_LO      0x4002
#define REG_SQ1_HI      0x4003

#define REG_SQ2_VOL     0x4004
#define REG_SQ2_SWEEP   0x4005
#define REG_SQ2_LO      0x4006
#define REG_SQ2_HI      0x4007

#define REG_TRI_LINEAR  0x4008
#define REG_TRI_LO      0x400A
#define REG_TRI_HI      0x400B

#define REG_NOISE_VOL   0x400C
#define REG_NOISE_LO    0x400E
#define REG_NOISE_HI    0x400F

#define REG_DMC_FREQ    0x4010
#define REG_DMC_RAW     0x4011
#define REG_DMC_START   0x4012
#define REG_DMC_LEN     0x4013

#define REG_OAMDMA      0x4014
#define REG_SND_CHN     0x4015
#define REG_JOY1        0x4016
#define REG_JOY2_APUFC  0x4017

/* MMC1 Mapper Registers */
#define REG_MMC1_CTRL   0x9FFF
#define REG_MMC1_CHR0   0xBFFF
#define REG_MMC1_CHR1   0xDFFF
#define REG_MMC1_PRG    0xFFF0

#ifdef __cplusplus
}
#endif

#endif /* TDECOMP_CONSTANTS_HARDWARE_H */
