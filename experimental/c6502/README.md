# Experimental: C-to-6502 Compilation & Bare-Metal NES Target

This directory contains experimental target-ready C implementations and research for compiling TDecomp directly into an executable NES 6502 ROM using the `cc65` C compiler.

---

## 1. Architectural Distinction: Host-Side vs. Target-Ready C

TDecomp maintains two distinct C paradigms:

### A. Host-Side Portable C Engine (`include/`, `src/`)
- **Purpose**: Cross-platform execution (Linux, Windows, macOS, Android, WebAssembly), headless testing, and formal differential verification.
- **State Representation**: Monolithic 48KB `NES_State` struct containing CPU RAM ($0000–$07FF), PRG ROM, CHR ROM, and hardware registers.
- **Calling Convention**: Every routine takes `NES_State *nes`. Memory operations are performed via struct indexing (`nes->ram[...]`) or accessor functions (`nes_read8`, `nes_write8`).
- **Target Compatibility**: Cannot run on a physical NES (CPU RAM is only 2KB total; passing 16-bit struct pointers to every function on a 1.79 MHz 6502 is prohibitively slow and exhausts memory).

### B. Target-Ready 6502 C (`experimental/c6502/`)
- **Purpose**: Direct compilation into bare-metal NMOS 6502 machine code for an NES ROM cartridge.
- **State Representation**: Direct hardware memory-mapped addresses (e.g. `REG_OAMADDR` at `$2003`, `REG_OAMDMA` at `$4014`) and fixed zero-page addresses (`$0000`–`$00FF`).
- **Calling Convention**: Void or register-based parameters matching 6502 calling conventions.
- **Stack & Memory Model**: Uses `#pragma static-locals(on)` to avoid cc65's software stack overhead, preserving zero-page allocations for original game variables.

---

## 2. Compilation Results & Evidence

Using `cc65 -t none -O -Oi`:

### 1. Direct Register Routines: 100% Bit-Exact Equivalence
- **`copyOamStagingToOam`** ($AB5E, 11 bytes):
  ```c
  void copyOamStagingToOam(void) {
      REG_OAMADDR = 0x00;
      REG_OAMDMA = 0x02;
  }
  ```
  Compiles to:
  ```asm
  lda #$00
  sta $2003
  lda #$02
  sta $4014
  rts
  ```
  Result: **100% BIT-EXACT** to the 1989 Nintendo PRG ROM machine code (`a9 00 8d 03 20 a9 02 8d 14 40 60`).

- **`muteAudio`** ($E271, 22 bytes):
  ```c
  void muteAudio(void) {
      REG_DMC_RAW = 0x00;
      REG_SQ1_VOL = 0x10;
      REG_SQ2_VOL = 0x10;
      REG_NOISE_VOL = 0x10;
      REG_TRI_LINEAR = 0x00;
  }
  ```
  Compiles to:
  ```asm
  lda #$00
  sta $4011
  lda #$10
  sta $4000
  sta $4004
  sta $400C
  lda #$00
  sta $4008
  rts
  ```
  Result: **100% BIT-EXACT** to the 1989 Nintendo PRG ROM machine code.

### 2. Algorithmic / Shift Routines: Behavioral Equivalence
- **`unreferenced_soundRng`** ($E09E):
  - In 6502 assembly, shifts use `ror` through the processor Carry flag.
  - In standard C, shifts use `>>` and `|` bitwise operations.
  - Result: Computes the exact same 16-bit pseudo-random sequence on every frame, demonstrating **behavioral equivalence** without relying on compiler-specific inline assembly.

---

## 3. How to Build & Verify

Run the automated ROM build and proof-of-concept verification tool:

```bash
# Build baseline assembly ROM and verify C-to-6502 proof-of-concept
python3 tools/build_rom.py all

# Or via CMake:
cmake --build build --target rom
```
