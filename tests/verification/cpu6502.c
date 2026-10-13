#include "cpu6502.h"
#include "constants/memory.h"

static inline uint8_t mem_read(CPU6502 *cpu, uint16_t addr) {
    if (addr == ADDR_VERTICAL_BLANK_INTERVAL) {
        if (cpu->nes->ram[ADDR_VERTICAL_BLANK_INTERVAL] == 0) {
            if (cpu->nes->ram[ADDR_SLEEP_COUNTER] > 0) {
                cpu->nes->ram[ADDR_SLEEP_COUNTER]--;
            }
            return 1;
        }
    }
    return nes_read8(cpu->nes, addr);
}

static inline void mem_write(CPU6502 *cpu, uint16_t addr, uint8_t val) {
    nes_write8(cpu->nes, addr, val);
}

static inline void push(CPU6502 *cpu, uint8_t val) {
    mem_write(cpu, (uint16_t)(0x0100 | cpu->sp), val);
    cpu->sp--;
}

static inline uint8_t pop(CPU6502 *cpu) {
    cpu->sp++;
    return mem_read(cpu, (uint16_t)(0x0100 | cpu->sp));
}

static inline void set_flag(CPU6502 *cpu, uint8_t flag, bool val) {
    if (val) cpu->p |= flag;
    else     cpu->p &= ~flag;
}

static inline bool get_flag(const CPU6502 *cpu, uint8_t flag) {
    return (cpu->p & flag) != 0;
}

static inline void set_zn(CPU6502 *cpu, uint8_t val) {
    set_flag(cpu, FLAG_Z, val == 0);
    set_flag(cpu, FLAG_N, (val & 0x80) != 0);
}

void cpu6502_init(CPU6502 *cpu, NES_State *nes) {
    if (!cpu) return;
    cpu->a = 0;
    cpu->x = 0;
    cpu->y = 0;
    cpu->sp = 0xFD;
    cpu->p = FLAG_U | FLAG_I;
    cpu->pc = 0x8000;
    cpu->nes = nes;
    cpu->cycles = 0;
    cpu->halted = false;
}

void cpu6502_set_state(CPU6502 *cpu, uint16_t pc, uint8_t a, uint8_t x, uint8_t y, uint8_t p, uint8_t sp) {
    if (!cpu) return;
    cpu->pc = pc;
    cpu->a = a;
    cpu->x = x;
    cpu->y = y;
    cpu->p = p | FLAG_U;
    cpu->sp = sp;
    cpu->halted = false;
}

static inline uint8_t fetch(CPU6502 *cpu) {
    uint8_t val = mem_read(cpu, cpu->pc);
    cpu->pc++;
    return val;
}

static inline uint16_t fetch16(CPU6502 *cpu) {
    uint8_t lo = fetch(cpu);
    uint8_t hi = fetch(cpu);
    return MAKE_WORD(lo, hi);
}

/* Addressing mode helpers */
static inline uint16_t addr_zp(CPU6502 *cpu) {
    return fetch(cpu);
}

static inline uint16_t addr_zpx(CPU6502 *cpu) {
    return (uint8_t)(fetch(cpu) + cpu->x);
}

static inline uint16_t addr_zpy(CPU6502 *cpu) {
    return (uint8_t)(fetch(cpu) + cpu->y);
}

static inline uint16_t addr_abs(CPU6502 *cpu) {
    return fetch16(cpu);
}

static inline uint16_t addr_absx(CPU6502 *cpu) {
    return (uint16_t)(fetch16(cpu) + cpu->x);
}

static inline uint16_t addr_absy(CPU6502 *cpu) {
    return (uint16_t)(fetch16(cpu) + cpu->y);
}

static inline uint16_t addr_indx(CPU6502 *cpu) {
    uint8_t ptr = (uint8_t)(fetch(cpu) + cpu->x);
    uint8_t lo = mem_read(cpu, ptr);
    uint8_t hi = mem_read(cpu, (uint8_t)(ptr + 1));
    return MAKE_WORD(lo, hi);
}

static inline uint16_t addr_indy(CPU6502 *cpu) {
    uint8_t ptr = fetch(cpu);
    uint8_t lo = mem_read(cpu, ptr);
    uint8_t hi = mem_read(cpu, (uint8_t)(ptr + 1));
    return (uint16_t)(MAKE_WORD(lo, hi) + cpu->y);
}

/* ALU operations */
static inline void do_adc(CPU6502 *cpu, uint8_t val) {
    uint8_t c = get_flag(cpu, FLAG_C) ? 1 : 0;
    uint16_t sum = (uint16_t)cpu->a + (uint16_t)val + (uint16_t)c;
    bool v = (~(cpu->a ^ val) & (cpu->a ^ sum) & 0x80) != 0;
    set_flag(cpu, FLAG_C, sum > 0xFF);
    set_flag(cpu, FLAG_V, v);
    cpu->a = (uint8_t)(sum & 0xFF);
    set_zn(cpu, cpu->a);
}

static inline void do_sbc(CPU6502 *cpu, uint8_t val) {
    do_adc(cpu, (uint8_t)(~val));
}

static inline void do_cmp(CPU6502 *cpu, uint8_t reg, uint8_t val) {
    uint16_t diff = (uint16_t)reg - (uint16_t)val;
    set_flag(cpu, FLAG_C, reg >= val);
    set_zn(cpu, (uint8_t)(diff & 0xFF));
}

static inline uint8_t do_asl(CPU6502 *cpu, uint8_t val) {
    set_flag(cpu, FLAG_C, (val & 0x80) != 0);
    uint8_t res = (uint8_t)(val << 1);
    set_zn(cpu, res);
    return res;
}

static inline uint8_t do_lsr(CPU6502 *cpu, uint8_t val) {
    set_flag(cpu, FLAG_C, (val & 0x01) != 0);
    uint8_t res = (uint8_t)(val >> 1);
    set_zn(cpu, res);
    return res;
}

static inline uint8_t do_rol(CPU6502 *cpu, uint8_t val) {
    uint8_t c = get_flag(cpu, FLAG_C) ? 1 : 0;
    set_flag(cpu, FLAG_C, (val & 0x80) != 0);
    uint8_t res = (uint8_t)((val << 1) | c);
    set_zn(cpu, res);
    return res;
}

static inline uint8_t do_ror(CPU6502 *cpu, uint8_t val) {
    uint8_t c = get_flag(cpu, FLAG_C) ? 0x80 : 0x00;
    set_flag(cpu, FLAG_C, (val & 0x01) != 0);
    uint8_t res = (uint8_t)((val >> 1) | c);
    set_zn(cpu, res);
    return res;
}

uint8_t cpu6502_step(CPU6502 *cpu) {
    if (!cpu || cpu->halted) return 0;

    uint8_t op = fetch(cpu);
    uint16_t addr = 0;
    uint8_t val = 0;

    switch (op) {
        /* NOP */
        case 0xEA: break;

        /* LDA */
        case 0xA9: cpu->a = fetch(cpu); set_zn(cpu, cpu->a); break;
        case 0xA5: cpu->a = mem_read(cpu, addr_zp(cpu)); set_zn(cpu, cpu->a); break;
        case 0xB5: cpu->a = mem_read(cpu, addr_zpx(cpu)); set_zn(cpu, cpu->a); break;
        case 0xAD: cpu->a = mem_read(cpu, addr_abs(cpu)); set_zn(cpu, cpu->a); break;
        case 0xBD: cpu->a = mem_read(cpu, addr_absx(cpu)); set_zn(cpu, cpu->a); break;
        case 0xB9: cpu->a = mem_read(cpu, addr_absy(cpu)); set_zn(cpu, cpu->a); break;
        case 0xA1: cpu->a = mem_read(cpu, addr_indx(cpu)); set_zn(cpu, cpu->a); break;
        case 0xB1: cpu->a = mem_read(cpu, addr_indy(cpu)); set_zn(cpu, cpu->a); break;

        /* LDX */
        case 0xA2: cpu->x = fetch(cpu); set_zn(cpu, cpu->x); break;
        case 0xA6: cpu->x = mem_read(cpu, addr_zp(cpu)); set_zn(cpu, cpu->x); break;
        case 0xB6: cpu->x = mem_read(cpu, addr_zpy(cpu)); set_zn(cpu, cpu->x); break;
        case 0xAE: cpu->x = mem_read(cpu, addr_abs(cpu)); set_zn(cpu, cpu->x); break;
        case 0xBE: cpu->x = mem_read(cpu, addr_absy(cpu)); set_zn(cpu, cpu->x); break;

        /* LDY */
        case 0xA0: cpu->y = fetch(cpu); set_zn(cpu, cpu->y); break;
        case 0xA4: cpu->y = mem_read(cpu, addr_zp(cpu)); set_zn(cpu, cpu->y); break;
        case 0xB4: cpu->y = mem_read(cpu, addr_zpx(cpu)); set_zn(cpu, cpu->y); break;
        case 0xAC: cpu->y = mem_read(cpu, addr_abs(cpu)); set_zn(cpu, cpu->y); break;
        case 0xBC: cpu->y = mem_read(cpu, addr_absx(cpu)); set_zn(cpu, cpu->y); break;

        /* STA */
        case 0x85: mem_write(cpu, addr_zp(cpu), cpu->a); break;
        case 0x95: mem_write(cpu, addr_zpx(cpu), cpu->a); break;
        case 0x8D: mem_write(cpu, addr_abs(cpu), cpu->a); break;
        case 0x9D: mem_write(cpu, addr_absx(cpu), cpu->a); break;
        case 0x99: mem_write(cpu, addr_absy(cpu), cpu->a); break;
        case 0x81: mem_write(cpu, addr_indx(cpu), cpu->a); break;
        case 0x91: mem_write(cpu, addr_indy(cpu), cpu->a); break;

        /* STX */
        case 0x86: mem_write(cpu, addr_zp(cpu), cpu->x); break;
        case 0x96: mem_write(cpu, addr_zpy(cpu), cpu->x); break;
        case 0x8E: mem_write(cpu, addr_abs(cpu), cpu->x); break;

        /* STY */
        case 0x84: mem_write(cpu, addr_zp(cpu), cpu->y); break;
        case 0x94: mem_write(cpu, addr_zpx(cpu), cpu->y); break;
        case 0x8C: mem_write(cpu, addr_abs(cpu), cpu->y); break;

        /* Register Transfers */
        case 0xAA: cpu->x = cpu->a; set_zn(cpu, cpu->x); break;
        case 0xA8: cpu->y = cpu->a; set_zn(cpu, cpu->y); break;
        case 0x8A: cpu->a = cpu->x; set_zn(cpu, cpu->a); break;
        case 0x98: cpu->a = cpu->y; set_zn(cpu, cpu->a); break;
        case 0xBA: cpu->x = cpu->sp; set_zn(cpu, cpu->x); break;
        case 0x9A: cpu->sp = cpu->x; break;

        /* Stack */
        case 0x48: push(cpu, cpu->a); break;
        case 0x08: push(cpu, (uint8_t)(cpu->p | FLAG_B | FLAG_U)); break;
        case 0x68: cpu->a = pop(cpu); set_zn(cpu, cpu->a); break;
        case 0x28: cpu->p = (uint8_t)((pop(cpu) & ~FLAG_B) | FLAG_U); break;

        /* ADC */
        case 0x69: do_adc(cpu, fetch(cpu)); break;
        case 0x65: do_adc(cpu, mem_read(cpu, addr_zp(cpu))); break;
        case 0x75: do_adc(cpu, mem_read(cpu, addr_zpx(cpu))); break;
        case 0x6D: do_adc(cpu, mem_read(cpu, addr_abs(cpu))); break;
        case 0x7D: do_adc(cpu, mem_read(cpu, addr_absx(cpu))); break;
        case 0x79: do_adc(cpu, mem_read(cpu, addr_absy(cpu))); break;
        case 0x61: do_adc(cpu, mem_read(cpu, addr_indx(cpu))); break;
        case 0x71: do_adc(cpu, mem_read(cpu, addr_indy(cpu))); break;

        /* SBC */
        case 0xE9: do_sbc(cpu, fetch(cpu)); break;
        case 0xE5: do_sbc(cpu, mem_read(cpu, addr_zp(cpu))); break;
        case 0xF5: do_sbc(cpu, mem_read(cpu, addr_zpx(cpu))); break;
        case 0xED: do_sbc(cpu, mem_read(cpu, addr_abs(cpu))); break;
        case 0xFD: do_sbc(cpu, mem_read(cpu, addr_absx(cpu))); break;
        case 0xF9: do_sbc(cpu, mem_read(cpu, addr_absy(cpu))); break;
        case 0xE1: do_sbc(cpu, mem_read(cpu, addr_indx(cpu))); break;
        case 0xF1: do_sbc(cpu, mem_read(cpu, addr_indy(cpu))); break;

        /* CMP */
        case 0xC9: do_cmp(cpu, cpu->a, fetch(cpu)); break;
        case 0xC5: do_cmp(cpu, cpu->a, mem_read(cpu, addr_zp(cpu))); break;
        case 0xD5: do_cmp(cpu, cpu->a, mem_read(cpu, addr_zpx(cpu))); break;
        case 0xCD: do_cmp(cpu, cpu->a, mem_read(cpu, addr_abs(cpu))); break;
        case 0xDD: do_cmp(cpu, cpu->a, mem_read(cpu, addr_absx(cpu))); break;
        case 0xD9: do_cmp(cpu, cpu->a, mem_read(cpu, addr_absy(cpu))); break;
        case 0xC1: do_cmp(cpu, cpu->a, mem_read(cpu, addr_indx(cpu))); break;
        case 0xD1: do_cmp(cpu, cpu->a, mem_read(cpu, addr_indy(cpu))); break;

        /* CPX */
        case 0xE0: do_cmp(cpu, cpu->x, fetch(cpu)); break;
        case 0xE4: do_cmp(cpu, cpu->x, mem_read(cpu, addr_zp(cpu))); break;
        case 0xEC: do_cmp(cpu, cpu->x, mem_read(cpu, addr_abs(cpu))); break;

        /* CPY */
        case 0xC0: do_cmp(cpu, cpu->y, fetch(cpu)); break;
        case 0xC4: do_cmp(cpu, cpu->y, mem_read(cpu, addr_zp(cpu))); break;
        case 0xCC: do_cmp(cpu, cpu->y, mem_read(cpu, addr_abs(cpu))); break;

        /* AND */
        case 0x29: cpu->a &= fetch(cpu); set_zn(cpu, cpu->a); break;
        case 0x25: cpu->a &= mem_read(cpu, addr_zp(cpu)); set_zn(cpu, cpu->a); break;
        case 0x35: cpu->a &= mem_read(cpu, addr_zpx(cpu)); set_zn(cpu, cpu->a); break;
        case 0x2D: cpu->a &= mem_read(cpu, addr_abs(cpu)); set_zn(cpu, cpu->a); break;
        case 0x3D: cpu->a &= mem_read(cpu, addr_absx(cpu)); set_zn(cpu, cpu->a); break;
        case 0x39: cpu->a &= mem_read(cpu, addr_absy(cpu)); set_zn(cpu, cpu->a); break;
        case 0x21: cpu->a &= mem_read(cpu, addr_indx(cpu)); set_zn(cpu, cpu->a); break;
        case 0x31: cpu->a &= mem_read(cpu, addr_indy(cpu)); set_zn(cpu, cpu->a); break;

        /* ORA */
        case 0x09: cpu->a |= fetch(cpu); set_zn(cpu, cpu->a); break;
        case 0x05: cpu->a |= mem_read(cpu, addr_zp(cpu)); set_zn(cpu, cpu->a); break;
        case 0x15: cpu->a |= mem_read(cpu, addr_zpx(cpu)); set_zn(cpu, cpu->a); break;
        case 0x0D: cpu->a |= mem_read(cpu, addr_abs(cpu)); set_zn(cpu, cpu->a); break;
        case 0x1D: cpu->a |= mem_read(cpu, addr_absx(cpu)); set_zn(cpu, cpu->a); break;
        case 0x19: cpu->a |= mem_read(cpu, addr_absy(cpu)); set_zn(cpu, cpu->a); break;
        case 0x01: cpu->a |= mem_read(cpu, addr_indx(cpu)); set_zn(cpu, cpu->a); break;
        case 0x11: cpu->a |= mem_read(cpu, addr_indy(cpu)); set_zn(cpu, cpu->a); break;

        /* EOR */
        case 0x49: cpu->a ^= fetch(cpu); set_zn(cpu, cpu->a); break;
        case 0x45: cpu->a ^= mem_read(cpu, addr_zp(cpu)); set_zn(cpu, cpu->a); break;
        case 0x55: cpu->a ^= mem_read(cpu, addr_zpx(cpu)); set_zn(cpu, cpu->a); break;
        case 0x4D: cpu->a ^= mem_read(cpu, addr_abs(cpu)); set_zn(cpu, cpu->a); break;
        case 0x5D: cpu->a ^= mem_read(cpu, addr_absx(cpu)); set_zn(cpu, cpu->a); break;
        case 0x59: cpu->a ^= mem_read(cpu, addr_absy(cpu)); set_zn(cpu, cpu->a); break;
        case 0x41: cpu->a ^= mem_read(cpu, addr_indx(cpu)); set_zn(cpu, cpu->a); break;
        case 0x51: cpu->a ^= mem_read(cpu, addr_indy(cpu)); set_zn(cpu, cpu->a); break;

        /* BIT */
        case 0x24:
            val = mem_read(cpu, addr_zp(cpu));
            set_flag(cpu, FLAG_Z, (cpu->a & val) == 0);
            set_flag(cpu, FLAG_V, (val & 0x40) != 0);
            set_flag(cpu, FLAG_N, (val & 0x80) != 0);
            break;
        case 0x2C:
            val = mem_read(cpu, addr_abs(cpu));
            set_flag(cpu, FLAG_Z, (cpu->a & val) == 0);
            set_flag(cpu, FLAG_V, (val & 0x40) != 0);
            set_flag(cpu, FLAG_N, (val & 0x80) != 0);
            break;

        /* ASL */
        case 0x0A: cpu->a = do_asl(cpu, cpu->a); break;
        case 0x06: addr = addr_zp(cpu); mem_write(cpu, addr, do_asl(cpu, mem_read(cpu, addr))); break;
        case 0x16: addr = addr_zpx(cpu); mem_write(cpu, addr, do_asl(cpu, mem_read(cpu, addr))); break;
        case 0x0E: addr = addr_abs(cpu); mem_write(cpu, addr, do_asl(cpu, mem_read(cpu, addr))); break;
        case 0x1E: addr = addr_absx(cpu); mem_write(cpu, addr, do_asl(cpu, mem_read(cpu, addr))); break;

        /* LSR */
        case 0x4A: cpu->a = do_lsr(cpu, cpu->a); break;
        case 0x46: addr = addr_zp(cpu); mem_write(cpu, addr, do_lsr(cpu, mem_read(cpu, addr))); break;
        case 0x56: addr = addr_zpx(cpu); mem_write(cpu, addr, do_lsr(cpu, mem_read(cpu, addr))); break;
        case 0x4E: addr = addr_abs(cpu); mem_write(cpu, addr, do_lsr(cpu, mem_read(cpu, addr))); break;
        case 0x5E: addr = addr_absx(cpu); mem_write(cpu, addr, do_lsr(cpu, mem_read(cpu, addr))); break;

        /* ROL */
        case 0x2A: cpu->a = do_rol(cpu, cpu->a); break;
        case 0x26: addr = addr_zp(cpu); mem_write(cpu, addr, do_rol(cpu, mem_read(cpu, addr))); break;
        case 0x36: addr = addr_zpx(cpu); mem_write(cpu, addr, do_rol(cpu, mem_read(cpu, addr))); break;
        case 0x2E: addr = addr_abs(cpu); mem_write(cpu, addr, do_rol(cpu, mem_read(cpu, addr))); break;
        case 0x3E: addr = addr_absx(cpu); mem_write(cpu, addr, do_rol(cpu, mem_read(cpu, addr))); break;

        /* ROR */
        case 0x6A: cpu->a = do_ror(cpu, cpu->a); break;
        case 0x66: addr = addr_zp(cpu); mem_write(cpu, addr, do_ror(cpu, mem_read(cpu, addr))); break;
        case 0x76: addr = addr_zpx(cpu); mem_write(cpu, addr, do_ror(cpu, mem_read(cpu, addr))); break;
        case 0x6E: addr = addr_abs(cpu); mem_write(cpu, addr, do_ror(cpu, mem_read(cpu, addr))); break;
        case 0x7E: addr = addr_absx(cpu); mem_write(cpu, addr, do_ror(cpu, mem_read(cpu, addr))); break;

        /* INC */
        case 0xE6: addr = addr_zp(cpu); val = (uint8_t)(mem_read(cpu, addr) + 1); mem_write(cpu, addr, val); set_zn(cpu, val); break;
        case 0xF6: addr = addr_zpx(cpu); val = (uint8_t)(mem_read(cpu, addr) + 1); mem_write(cpu, addr, val); set_zn(cpu, val); break;
        case 0xEE: addr = addr_abs(cpu); val = (uint8_t)(mem_read(cpu, addr) + 1); mem_write(cpu, addr, val); set_zn(cpu, val); break;
        case 0xFE: addr = addr_absx(cpu); val = (uint8_t)(mem_read(cpu, addr) + 1); mem_write(cpu, addr, val); set_zn(cpu, val); break;

        /* DEC */
        case 0xC6: addr = addr_zp(cpu); val = (uint8_t)(mem_read(cpu, addr) - 1); mem_write(cpu, addr, val); set_zn(cpu, val); break;
        case 0xD6: addr = addr_zpx(cpu); val = (uint8_t)(mem_read(cpu, addr) - 1); mem_write(cpu, addr, val); set_zn(cpu, val); break;
        case 0xCE: addr = addr_abs(cpu); val = (uint8_t)(mem_read(cpu, addr) - 1); mem_write(cpu, addr, val); set_zn(cpu, val); break;
        case 0xDE: addr = addr_absx(cpu); val = (uint8_t)(mem_read(cpu, addr) - 1); mem_write(cpu, addr, val); set_zn(cpu, val); break;

        /* INX / DEX / INY / DEY */
        case 0xE8: cpu->x++; set_zn(cpu, cpu->x); break;
        case 0xCA: cpu->x--; set_zn(cpu, cpu->x); break;
        case 0xC8: cpu->y++; set_zn(cpu, cpu->y); break;
        case 0x88: cpu->y--; set_zn(cpu, cpu->y); break;

        /* Flag Clears / Sets */
        case 0x18: set_flag(cpu, FLAG_C, false); break;
        case 0x38: set_flag(cpu, FLAG_C, true); break;
        case 0xD8: set_flag(cpu, FLAG_D, false); break;
        case 0xF8: set_flag(cpu, FLAG_D, true); break;
        case 0x58: set_flag(cpu, FLAG_I, false); break;
        case 0x78: set_flag(cpu, FLAG_I, true); break;
        case 0xB8: set_flag(cpu, FLAG_V, false); break;

        /* Branches */
        case 0x10: { int8_t rel = (int8_t)fetch(cpu); if (!get_flag(cpu, FLAG_N)) cpu->pc = (uint16_t)(cpu->pc + rel); break; }
        case 0x30: { int8_t rel = (int8_t)fetch(cpu); if ( get_flag(cpu, FLAG_N)) cpu->pc = (uint16_t)(cpu->pc + rel); break; }
        case 0x50: { int8_t rel = (int8_t)fetch(cpu); if (!get_flag(cpu, FLAG_V)) cpu->pc = (uint16_t)(cpu->pc + rel); break; }
        case 0x70: { int8_t rel = (int8_t)fetch(cpu); if ( get_flag(cpu, FLAG_V)) cpu->pc = (uint16_t)(cpu->pc + rel); break; }
        case 0x90: { int8_t rel = (int8_t)fetch(cpu); if (!get_flag(cpu, FLAG_C)) cpu->pc = (uint16_t)(cpu->pc + rel); break; }
        case 0xB0: { int8_t rel = (int8_t)fetch(cpu); if ( get_flag(cpu, FLAG_C)) cpu->pc = (uint16_t)(cpu->pc + rel); break; }
        case 0xD0: { int8_t rel = (int8_t)fetch(cpu); if (!get_flag(cpu, FLAG_Z)) cpu->pc = (uint16_t)(cpu->pc + rel); break; }
        case 0xF0: { int8_t rel = (int8_t)fetch(cpu); if ( get_flag(cpu, FLAG_Z)) cpu->pc = (uint16_t)(cpu->pc + rel); break; }

        /* Jumps & Subroutines */
        case 0x4C: cpu->pc = addr_abs(cpu); break;
        case 0x6C: {
            uint16_t ptr = addr_abs(cpu);
            uint8_t lo = mem_read(cpu, ptr);
            /* 6502 page wrap bug: ptr+1 on page boundary wraps within same page */
            uint16_t ptr_hi = (uint16_t)((ptr & 0xFF00) | ((ptr + 1) & 0x00FF));
            uint8_t hi = mem_read(cpu, ptr_hi);
            cpu->pc = MAKE_WORD(lo, hi);
            break;
        }
        case 0x20: {
            uint16_t dest = fetch16(cpu);
            uint16_t ret = (uint16_t)(cpu->pc - 1);
            push(cpu, HIGH_BYTE(ret));
            push(cpu, LOW_BYTE(ret));
            cpu->pc = dest;
            break;
        }
        case 0x60: { /* RTS */
            uint8_t lo = pop(cpu);
            uint8_t hi = pop(cpu);
            cpu->pc = (uint16_t)(MAKE_WORD(lo, hi) + 1);
            break;
        }
        case 0x40: { /* RTI */
            cpu->p = (uint8_t)((pop(cpu) & ~FLAG_B) | FLAG_U);
            uint8_t lo = pop(cpu);
            uint8_t hi = pop(cpu);
            cpu->pc = MAKE_WORD(lo, hi);
            break;
        }

        default:
            /* Unknown opcode */
            cpu->halted = true;
            return 0;
    }

    cpu->cycles++;
    return 2; /* Approximate */
}

bool cpu6502_run_subroutine(CPU6502 *cpu, uint16_t entry_addr, uint32_t max_cycles) {
    if (!cpu || !cpu->nes) return false;

    uint8_t target_sp = cpu->sp;
    /* Push sentinel return address: 0x0000 */
    push(cpu, 0x00);
    push(cpu, 0x00);

    cpu->pc = entry_addr;
    cpu->halted = false;

    uint32_t count = 0;
    while (!cpu->halted && count < max_cycles) {
        /* Check if next instruction is RTS and SP matches target_sp - 2 */
        uint8_t next_op = mem_read(cpu, cpu->pc);
        if (next_op == 0x60 && cpu->sp == (uint8_t)(target_sp - 2)) {
            /* We are at the final RTS! Execute it and stop */
            cpu6502_step(cpu);
            return true;
        }
        if (next_op == 0x40 && (cpu->sp == (uint8_t)(target_sp - 2) || cpu->sp == (uint8_t)(target_sp - 3))) {
            /* RTI final return */
            return true;
        }

        cpu6502_step(cpu);
        count++;
    }

    return false;
}
