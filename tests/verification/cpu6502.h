#ifndef TDECOMP_CPU6502_H
#define TDECOMP_CPU6502_H

#include "common.h"
#include "nes.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 6502 Status Register Flags */
#define FLAG_C  0x01  /* Carry */
#define FLAG_Z  0x02  /* Zero */
#define FLAG_I  0x04  /* Interrupt Disable */
#define FLAG_D  0x08  /* Decimal Mode */
#define FLAG_B  0x10  /* Break Command */
#define FLAG_U  0x20  /* Unused (always 1 in P) */
#define FLAG_V  0x40  /* Overflow */
#define FLAG_N  0x80  /* Negative */

typedef struct CPU6502 {
    uint8_t a;        /* Accumulator */
    uint8_t x;        /* X index */
    uint8_t y;        /* Y index */
    uint8_t sp;       /* Stack pointer ($0100 + sp) */
    uint8_t p;        /* Processor status */
    uint16_t pc;      /* Program counter */

    NES_State *nes;   /* Bound NES memory system */
    uint64_t cycles;  /* Cycle counter */
    bool halted;
} CPU6502;

/* Initialize CPU and bind to NES state */
void cpu6502_init(CPU6502 *cpu, NES_State *nes);

/* Set CPU registers directly for test harness */
void cpu6502_set_state(CPU6502 *cpu, uint16_t pc, uint8_t a, uint8_t x, uint8_t y, uint8_t p, uint8_t sp);

/* Execute a single 6502 instruction. Returns cycles taken. */
uint8_t cpu6502_step(CPU6502 *cpu);

/*
 * Executes a subroutine starting at entry_addr until the matching RTS/RTI returns.
 * Tracks stack pointer so that internal JSR/RTS calls within the routine execute fully.
 * Halts if cycles exceed max_cycles (protection against infinite loops).
 * Returns true if subroutine returned cleanly via RTS/RTI, false if timeout or error.
 */
bool cpu6502_run_subroutine(CPU6502 *cpu, uint16_t entry_addr, uint32_t max_cycles);

#ifdef __cplusplus
}
#endif

#endif /* TDECOMP_CPU6502_H */
