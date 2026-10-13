# TDecomp — Verified C-to-6502 ROM Integration Report

## Executive Summary

- **Project**: `TDecomp` (NES Tetris Decompilation)
- **Objective**: Incrementally replace native 6502 assembly routines with object code compiled from target-ready C inside an experimental NES ROM without breaking game behavior, corrupting hardware state, or shifting memory layout.
- **Routines Integrated**:
  1. **`copyOamStagingToOam`** (canon: `initializeOAM`): `$AB5E` (11 bytes, PRG chunk 1)
  2. **`muteAudio`**: `$E271` (22 bytes, PRG chunk 2)
- **Output Experimental ROM**: `build/rom/tetris_experimental.nes` (49,168 bytes)
- **Cumulative Verification Results**:
  1. **100.0% Bit-Exact Machine Code**: Both C routines compile to the exact 33 bytes of 1989 Nintendo machine code under `cc65`.
  2. **Zero Code Shifting**: All 32,768 bytes of PRG ROM and 16,384 bytes of CHR ROM match the reference cartridge bit-for-bit (0 bytes shifted).
  3. **Caller Alignment**:
     - NMI caller at `$801B` targets `$AB5E` (`20 5E AB`).
     - Audio init caller at `$E1E8` targets `$E271` (`20 71 E2`).
     - Tri control caller at `$E247` targets `$E271` (`20 71 E2`).
  4. **Behavioral Simulation**: Verified in NMOS 6502 differential CPU simulator; standalone execution and NMI interrupt handler succeed with exact hardware register states and 0 side effects.
  5. **Emulator Playability Status**: Marked **`BLOCKED`** for interactive GUI gameplay due to the headless Linux CLI container lacking a display server and emulator binaries (`fceux`/`nestopia`/`mednafen`).

---

## 1. Routines Specification & Original Assembly

### Routine 1: `copyOamStagingToOam` (`$AB5E`, 11 bytes)
```assembly
; canon is initializeOAM (main.asm lines 5486-5491)
copyOamStagingToOam:
        lda     #$00        ; A = $00
        sta     OAMADDR     ; $2003 = $00 (reset sprite pointer)
        lda     #$02        ; A = $02
        sta     OAMDMA      ; $4014 = $02 (trigger DMA from page $0200)
        rts                 ; return to NMI handler
```

- **Call Site**: NMI VBlank handler (`main.asm` line 262, address `$801B`).
- **Hardware Effects**: Resets PPU sprite pointer and initiates 256-byte DMA transfer from RAM page `$0200` to PPU OAM.

### Routine 2: `muteAudio` (`$E271`, 22 bytes)
```assembly
; main.asm lines 6188-6197
muteAudio:
        lda     #$00        ; A = $00
        sta     DMC_RAW     ; $4011 = $00 (clear raw DMC delta counter)
        lda     #$10        ; A = $10
        sta     SQ1_VOL     ; $4000 = $10 (silence square 1 channel)
        sta     SQ2_VOL     ; $4004 = $10 (silence square 2 channel)
        sta     NOISE_VOL   ; $400C = $10 (silence noise channel)
        lda     #$00        ; A = $00
        sta     TRI_LINEAR  ; $4008 = $00 (halt triangle linear counter)
        rts                 ; return to caller
```

- **Call Sites**:
  1. `initAudioAndMarkInited` (`main.asm` line 6116, address `$E1E8`).
  2. `muteAudioAndClearTriControl` (`main.asm` line 6167, address `$E247`).
- **Hardware Effects**: Clears DMC raw output, silences pulse and noise channels via volume envelope registers, and halts triangle wave channel linear counter.

---

## 2. Target-Ready C Implementations

### Source 1: `experimental/c6502/copy_oam.c`
```c
#include "nes_target.h"

#pragma code-name ("C_OAM")

void copyOamStagingToOam(void) {
    REG_OAMADDR = 0x00;
    REG_OAMDMA = 0x02;
}
```

### Source 2: `experimental/c6502/mute_audio.c`
```c
#include "nes_target.h"

#pragma code-name ("C_MUTE")

void muteAudio(void) {
    REG_DMC_RAW = 0x00;
    REG_SQ1_VOL = 0x10;
    REG_SQ2_VOL = 0x10;
    REG_NOISE_VOL = 0x10;
    REG_TRI_LINEAR = 0x00;
}
```

### Toolchain Compilation
```bash
# Compile C routines to 6502 assembly with cc65
cc65 -I experimental/c6502 -t none -O -Oi experimental/c6502/copy_oam.c -o copy_oam.s
cc65 -I experimental/c6502 -t none -O -Oi experimental/c6502/mute_audio.c -o mute_audio.s

# Assemble to object files with ca65
ca65 copy_oam.s -o copy_oam.o
ca65 mute_audio.s -o mute_audio.o
```

---

## 3. Linker Architecture & Non-Destructive Multi-Routine Segmentation

### Preservation of Authoritative Baseline
Upstream assembly in `TetrisNESDisasm/` remains 100% untouched. All staging, segmenting, and linking occur out-of-tree in `build/rom/experimental/`.

### Extended Linker Segmentation Configuration (`tetris_exp.cfg`)
`PRG_chunk1` and `PRG_chunk2` are split around their respective target C routines:

1. **`PRG_chunk1_pre`**: Spans `$8000` to `$AB5D` (preceding `copyOamStagingToOam`).
2. **`C_OAM`**: Explicitly placed at `start = $AB5E, size = $000B, type = ro` containing `copy_oam.o` (11 bytes).
3. **`PRG_chunk1_post`**: Explicitly placed at `start = $AB69` continuing with `pollController_actualRead`.
4. **`PRG_chunk2_pre`**: Spans `$DD00` to `$E270` (preceding `muteAudio`).
5. **`C_MUTE`**: Explicitly placed at `start = $E271, size = $0016, type = ro` containing `mute_audio.o` (22 bytes).
6. **`PRG_chunk2_post`**: Explicitly placed at `start = $E287` continuing with `initSoundEffectShared`.

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
    PRG_chunk2_pre:    load = PRG, type = ro, start = $DD00;
    C_MUTE:            load = PRG, type = ro, start = $E271;
    PRG_chunk2_post:   load = PRG, type = ro, start = $E287;
    unreferenced_data4:load = PRG, type = ro;
    PRG_chunk3:        load = PRG, type = ro, start = $FF00;
    VECTORS:           load = PRG, type = ro, start = $FFFA;
}
```

### Staged Assembly Symbol Aliasing
In `main_exp.asm`:
```assembly
; Routine 1 replacement
.import _copyOamStagingToOam
copyOamStagingToOam = _copyOamStagingToOam
.segment "PRG_chunk1_post"

; Routine 2 replacement
.import _muteAudio
muteAudio = _muteAudio
.segment "PRG_chunk2_post"
```

---

## 4. Machine-Code & Behavioral Comparison

### Machine-Code Byte Comparison

#### Routine 1: `copyOamStagingToOam` (`$AB5E`)
| Offset | CPU Address | Original Assembly Opcode | Compiled C Opcode | Byte Hex | Status |
|:---:|:---:|:---|:---|:---:|:---:|
| `+0x00` | `$AB5E` | `lda #$00` | `lda #$00` | `A9 00` | **MATCH** |
| `+0x02` | `$AB60` | `sta $2003` | `sta $2003` | `8D 03 20` | **MATCH** |
| `+0x05` | `$AB63` | `lda #$02` | `lda #$02` | `A9 02` | **MATCH** |
| `+0x07` | `$AB65` | `sta $4014` | `sta $4014` | `8D 14 40` | **MATCH** |
| `+0x0A` | `$AB68` | `rts` | `rts` | `60` | **MATCH** |

#### Routine 2: `muteAudio` (`$E271`)
| Offset | CPU Address | Original Assembly Opcode | Compiled C Opcode | Byte Hex | Status |
|:---:|:---:|:---|:---|:---:|:---:|
| `+0x00` | `$E271` | `lda #$00` | `lda #$00` | `A9 00` | **MATCH** |
| `+0x02` | `$E273` | `sta $4011` | `sta $4011` | `8D 11 40` | **MATCH** |
| `+0x05` | `$E276` | `lda #$10` | `lda #$10` | `A9 10` | **MATCH** |
| `+0x07` | `$E278` | `sta $4000` | `sta $4000` | `8D 00 40` | **MATCH** |
| `+0x0A` | `$E27B` | `sta $4004` | `sta $4004` | `8D 04 40` | **MATCH** |
| `+0x0D` | `$E27E` | `sta $400C` | `sta $400C` | `8D 0C 40` | **MATCH** |
| `+0x10` | `$E281` | `lda #$00` | `lda #$00` | `A9 00` | **MATCH** |
| `+0x12` | `$E283` | `sta $4008` | `sta $4008` | `8D 08 40` | **MATCH** |
| `+0x15` | `$E286` | `rts` | `rts` | `60` | **MATCH** |

### Call Site Audits
- **`$801B`**: `20 5E AB` (`jsr $AB5E`) — NMI VBlank handler
- **`$E1E8`**: `20 71 E2` (`jsr $E271`) — `initAudioAndMarkInited`
- **`$E247`**: `20 71 E2` (`jsr $E271`) — `muteAudioAndClearTriControl`

### PRG & CHR Payload Audit
- Total PRG ROM size: 32,768 bytes
- Total CHR ROM size: 16,384 bytes
- Differing payload bytes against reference ROM (`Tetris (USA).nes`): **0 bytes (100.0% BIT-EXACT MATCH)**

---

## 5. Verification & Test Execution

### 1. Build Verification Commands
```bash
# Build and verify experimental C-integrated ROM:
cmake --build build --target rom_experimental
# or directly:
python3 tools/build_rom.py experimental

# Full suite:
cmake --build build --target rom
```

### 2. Differential CPU Simulation Test
Integrated into `tests/verification/test_differential.c` (`test_experimental_c_rom`):
- **`copyOamStagingToOam` ($AB5E)**:
  - `A` = `0x02`, `oam_addr` = `0x00`, clean RTS return.
- **NMI Vector ($8005)**:
  - Invokes `$AB5E`, increments `frameCounter` (`$0020` -> `$0021`), RNG seed updated, 0 side effects.
- **`muteAudio` ($E271)**:
  - `A` = `0x00`
  - `SQ1_VOL` (`$4000`) = `0x10`
  - `SQ2_VOL` (`$4004`) = `0x10`
  - `NOISE_VOL` (`$400C`) = `0x10`
  - `TRI_LINEAR` (`$4008`) = `0x00`
  - `DMC_RAW` (`$4011`) = `0x00`
  - Clean RTS return with zero stack leakage.

### 3. Emulator Playability Status
- **Status**: **`BLOCKED`**
- **Reason**: The Linux execution environment is a headless CLI container with no X11/Wayland display server and no emulator binaries installed (`fceux`, `nestopia`, `mednafen`, `mesen` are absent from `PATH`).
- **Playability vs. Simulation**: Subroutine-level and interrupt-level execution is verified via automated 6502 CPU differential simulation; interactive game playability is explicitly marked `BLOCKED`.

---

## 6. Next Integration Candidate

The next candidates for incremental C-to-6502 integration:
1. **`soundEffectSlot2_makesNoSound`** (`$E244`, 3 bytes):
   - Jumps directly to `muteAudioAndClearTriControl` (`jmp $E247`).
2. **`unreferenced_soundRng`** (`$E09E`, 42 bytes):
   - Audio LFSR random number generator; behavioral equivalence already demonstrated in POC.
