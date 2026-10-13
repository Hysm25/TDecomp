# TDecomp — First Verified C-to-6502 ROM Integration Report

## Executive Summary

- **Project**: `TDecomp` (NES Tetris Decompilation)
- **Objective**: Replace a native 6502 assembly routine with object code compiled from target-ready C inside an experimental NES ROM without breaking game behavior or disrupting memory layout.
- **Routine Replaced**: `copyOamStagingToOam` (canon: `initializeOAM`)
- **ROM Address**: `$AB5E` (PRG ROM chunk 1)
- **Size**: 11 bytes (`$AB5E`–`$AB68`)
- **Caller Site**: NMI VBlank Handler at `$801B` (`jsr copyOamStagingToOam`)
- **Output Experimental ROM**: `build/rom/tetris_experimental.nes`
- **Verification Results**:
  1. **100.0% Bit-Exact Machine Code**: The C-compiled routine produces the exact 11 bytes of 1989 Nintendo machine code.
  2. **Zero Code Shifting**: All 32,768 bytes of PRG ROM and 16,384 bytes of CHR ROM match the reference ROM bit-for-bit (0 bytes shifted).
  3. **Caller Alignment**: Call site at `$801B` compiles to `20 5E AB`, branching to the exact C-compiled routine entry.
  4. **Behavioral Simulation**: Tested via NMOS 6502 differential CPU simulator; standalone routine and NMI execution both succeed with 0 side effects.
  5. **Emulator Playability Status**: Marked **`BLOCKED`** for interactive GUI gameplay due to the headless Linux CLI container lacking a display server and emulator binaries (`fceux`/`nestopia`/`mednafen`).

---

## 1. Routine Scope & Technical Specification

### Original Assembly Definition (`TetrisNESDisasm/main.asm` lines 5485–5491)
```assembly
; canon is initializeOAM
copyOamStagingToOam:
        lda     #$00
        sta     OAMADDR
        lda     #$02
        sta     OAMDMA
        rts
```

### Hardware Operation & Calling Context
- **Interrupt Context**: Called every frame inside the NMI interrupt handler (`main.asm` line 262) during vertical blanking interval.
- **`OAMADDR` ($2003)**: Writing `$00` resets the PPU sprite data address pointer to the beginning of internal 256-byte OAM memory.
- **`OAMDMA` ($4014)**: Writing `$02` instructs the NES DMA controller to halt the CPU and transfer 256 bytes from system RAM page `$0200`–`$02FF` into PPU OAM.
- **Calling Convention**: Void parameters, returns via native 6502 `rts` ($60). Accumulator is set to `$02` upon exit; X and Y registers are untouched.

---

## 2. Target-Ready C Implementation

### Source Code (`experimental/c6502/copy_oam.c`)
```c
/*
 * copy_oam.c - Target 6502 C implementation of copyOamStagingToOam ($AB5E)
 *
 * Resets OAM write address ($2003) to $00 and initiates OAM DMA transfer
 * ($4014) from staging page $0200. Bit-exact replacement for original assembly routine.
 */

#include "nes_target.h"

#pragma code-name ("C_OAM")

void copyOamStagingToOam(void) {
    REG_OAMADDR = 0x00;
    REG_OAMDMA = 0x02;
}
```

### Compiler Invocations
```bash
# 1. Compile C to 6502 assembly with cc65
cc65 -I experimental/c6502 -t none -O -Oi experimental/c6502/copy_oam.c -o copy_oam.s

# 2. Assemble to object file with ca65
ca65 copy_oam.s -o copy_oam.o
```

### Compiler Assembly Output (`copy_oam.s`)
```assembly
.segment	"C_OAM"

.proc	_copyOamStagingToOam: near

.segment	"C_OAM"

	lda     #$00
	sta     $2003
	lda     #$02
	sta     $4014
	rts

.endproc
```

---

## 3. Linker Architecture & Non-Destructive Segmentation Strategy

### Requirement: Zero Modifications to Authoritative Baseline
The upstream disassembly in `TetrisNESDisasm/` remains 100% untouched. All experimental staging and linking occurs out-of-tree in `build/rom/experimental/`.

### Linker Segmentation Strategy (`tetris_exp.cfg`)
In the original `tetris.nes.cfg`, `PRG_chunk1` is a single contiguous segment from `$8000` to `$D6C9`.
To inject the compiled C routine at its authoritative address `$AB5E` without shifting any preceding or succeeding code:

1. **`PRG_chunk1_pre`**: Spans `$8000` to `$AB5D` (up to `updateNextByteInSeed`).
2. **`C_OAM`**: Explicitly placed at `start = $AB5E, size = $000B, type = ro` containing `copy_oam.o`.
3. **`PRG_chunk1_post`**: Explicitly placed at `start = $AB69` continuing with `pollController_actualRead`.

```cfg
MEMORY {
    ZP:     start = $0000, size = $0100, type = rw, file = "";
    RAM:    start = $0100, size = $1F00, type = rw, file = "";
    HDR:    start = $0000, size = $0010, type = ro, fill = yes, fillval = $00;
    PRG:    start = $8000, size = $8000, type = ro, fill = yes, fillval = $00;
    CHR:    start = $0000, size = $4000, type = ro, fill = yes, fillval = $00;
}

SEGMENTS {
    ZEROPAGE: load = ZP;
    BSS:      load = RAM, type = bss;
    HEADER:   load = HDR, type = ro;
    CHR:      load = CHR, type = ro;
    PRG_chunk1_pre:    load = PRG, type = ro;
    C_OAM:             load = PRG, type = ro, start = $AB5E;
    PRG_chunk1_post:   load = PRG, type = ro, start = $AB69;
    unreferenced_data1:load = PRG, type = ro;
    PRG_chunk2:        load = PRG, type = ro, start = $DD00;
    unreferenced_data4:load = PRG, type = ro;
    PRG_chunk3:        load = PRG, type = ro, start = $FF00;
    VECTORS:           load = PRG, type = ro, start = $FFFA;
}
```

### Symbol Binding & Caller Resolution
In `main_exp.asm`, the original inline assembly routine definition is replaced by:
```assembly
; Replaced by cc65 compiled C routine
.import _copyOamStagingToOam
copyOamStagingToOam = _copyOamStagingToOam

.segment "PRG_chunk1_post"
```
Because `copy_oam.o` places `_copyOamStagingToOam` at `$AB5E`, the symbol alias binds `copyOamStagingToOam` to `$AB5E`. The caller `jsr copyOamStagingToOam` at line 262 (`$801B`) resolves to `20 5E AB` (`jsr $AB5E`).

---

## 4. Machine-Code & Behavioral Comparison

### Machine-Code Byte Comparison

| Byte Offset | CPU Address | Original Assembly Opcode | Compiled C Opcode | Byte Hex | Status |
|:---:|:---:|:---|:---|:---:|:---:|
| `+0x00` | `$AB5E` | `lda #$00` | `lda #$00` | `A9 00` | **MATCH** |
| `+0x02` | `$AB60` | `sta $2003` | `sta $2003` | `8D 03 20` | **MATCH** |
| `+0x05` | `$AB63` | `lda #$02` | `lda #$02` | `A9 02` | **MATCH** |
| `+0x07` | `$AB65` | `sta $4014` | `sta $4014` | `8D 14 40` | **MATCH** |
| `+0x0A` | `$AB68` | `rts` | `rts` | `60` | **MATCH** |

### PRG ROM Bit-for-Bit Audit
- Total bytes in PRG ROM: 32,768 bytes
- Differing bytes between baseline ROM and experimental ROM: **0 bytes (100.0% BIT-EXACT MATCH)**
- Differing bytes between reference ROM (`Tetris (USA).nes`) and experimental ROM: **0 bytes** (excluding 3 header metadata flags).

---

## 5. Verification & Test Execution

### 1. Build Verification Commands
```bash
# Build and verify experimental C-integrated ROM:
cmake --build build --target rom_experimental
# or:
python3 tools/build_rom.py experimental
```

### 2. Differential CPU Simulation Test
Integrated into `tests/verification/test_differential.c`:
- **Direct Routine Execution**: `cpu6502_run_subroutine(&cpu, 0xAB5E, 50)` executes `copyOamStagingToOam` inside `tetris_experimental.nes`:
  - `A` = `0x02`
  - `oam_addr` = `0x00`
  - SP restored cleanly by `rts`
- **Interrupt Vector Execution**: `cpu6502_run_subroutine(&cpu, 0x8005, 5000)` executes `nmi` which calls `copyOamStagingToOam`:
  - `frameCounter` increments from `0x20` to `0x21`
  - Timers and RNG update with 0 corrupted state

### 3. Emulator Playability Status
- **Status**: **`BLOCKED`**
- **Reason**: The Linux execution environment is a headless CLI container with no X11/Wayland display server and no emulator packages installed (`fceux`, `nestopia`, `mednafen`, `mesen` are absent from `PATH`).
- **Safety**: Rather than falsely claiming GUI playability, runtime emulator verification is formally marked `BLOCKED`, backed by 100% bit-exact machine code validation and NMOS 6502 cycle-accurate simulation.

---

## 6. Next Integration Candidate

The next ideal candidate routine for incremental C-to-6502 integration is:
- **`muteAudio`** (`$E271`, 22 bytes):
  - Proven 100% bit-exact in POC (`a9 00 8d 11 40 a9 10 8d 00 40 8d 04 40 8d 0c 40 a9 00 8d 08 40 60`).
  - Operates strictly on APU memory-mapped registers (`$4000`, `$4004`, `$4008`, `$400C`, `$4011`).
  - Takes no arguments, has no zero-page conflicts, and returns via `rts`.
