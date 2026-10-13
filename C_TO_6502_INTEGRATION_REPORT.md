# TDecomp — Verified C-to-6502 ROM Integration Report

## Executive Summary

- **Project**: `TDecomp` (NES Tetris Decompilation)
- **Objective**: Incrementally replace native 6502 assembly routines with object code compiled from target-ready C inside an experimental NES ROM without breaking game behavior, corrupting hardware state, or shifting memory layout.
- **Routines Investigated & Integrated**:
  1. **`copyOamStagingToOam`** (canon: `initializeOAM`): `$AB5E` (11 bytes, PRG chunk 1) — **INTEGRATED & VERIFIED**
  2. **`muteAudio`**: `$E271` (22 bytes, PRG chunk 2) — **INTEGRATED & VERIFIED**
  3. **`generateNextPseudorandomNumber`**: `$AB47` (23 bytes, PRG chunk 1) — **TARGET C PROVEN EQUIVALENT (62 bytes); IN-PLACE INTEGRATION SAFELY BLOCKED**
- **Output Experimental ROM**: `build/rom/tetris_experimental.nes` (49,168 bytes)
- **Cumulative Verification Results**:
  1. **100.0% Bit-Exact Machine Code**: Integrated C routines (`copyOamStagingToOam`, `muteAudio`) compile to the exact 33 bytes of 1989 Nintendo machine code under `cc65`.
  2. **Target C Gameplay RNG Equivalent**: Bare-metal C implementation (`experimental/c6502/rng.c`) exhaustively proven equivalent across all 65,536 16-bit seeds (0 mismatches) and verified across 1,000 continuous LFSR iterations in the NMOS 6502 CPU differential simulator.
  3. **Zero Code Shifting**: All 32,768 bytes of PRG ROM and 16,384 bytes of CHR ROM match the reference cartridge bit-for-bit (0 bytes shifted) with baseline intact.
  4. **Architectural Blocker Identified & Documented**: `generateNextPseudorandomNumber` in 6502 assembly is 23 bytes ($AB47–$AB5D). Pure target C under `cc65` compiles to 62 bytes (+39 bytes / +169.6% expansion). In accordance with safety requirements ("If safe placement is impossible without a broader relocation, stop and document the blocker rather than risking unrelated code"), in-place integration is safely blocked to protect downstream routines at `$AB5E` (`copyOamStagingToOam`) and `$AB69` (`pollController_actualRead`).
  5. **Caller Alignment**:
     - NMI caller at `$801B` targets `$AB5E` (`20 5E AB`).
     - Audio init caller at `$E1E8` targets `$E271` (`20 71 E2`).
     - Tri control caller at `$E247` targets `$E271` (`20 71 E2`).
  6. **Behavioral Simulation**: Verified in NMOS 6502 differential CPU simulator; standalone execution, NMI interrupt handler, and LFSR sequences succeed with exact hardware register states and 0 side effects.
  7. **Emulator Playability Status**: Marked **`BLOCKED`** for interactive GUI gameplay due to the headless Linux CLI container lacking a display server and emulator binaries (`fceux`/`nestopia`/`mednafen`).

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

### Routine 3: `generateNextPseudorandomNumber` (`$AB47`, 23 bytes)
```assembly
; main.asm lines 5468-5484
; reg x: zeropage addr of seed ($0017); reg y: size of seed ($02)
generateNextPseudorandomNumber:
        lda     tmp1,x              ; B5 00 (A = seed[0])
        and     #$02                ; 29 02 (isolate bit 1)
        sta     tmp1                ; 85 00 (scratch bit 1 in ZP $00)
        lda     tmp2,x              ; B5 01 (A = seed[1])
        and     #$02                ; 29 02 (isolate bit 1)
        eor     tmp1                ; 45 00 (feedback = seed[0] bit 1 ^ seed[1] bit 1)
        clc                         ; 18
        beq     @updateNextByteInSeed ; F0 01 (if feedback == 0, carry = 0)
        sec                         ; 38    (if feedback != 0, carry = 1)
@updateNextByteInSeed:
        ror     tmp1,x              ; 76 00 (rotate byte right with carry)
        inx                         ; E8    (advance seed byte pointer)
        dey                         ; 88    (decrement byte counter)
        bne     @updateNextByteInSeed ; D0 F9 (loop for 2 bytes)
        rts                         ; 60    (return to caller)
```

- **Call Sites (7 authoritative gameplay callers)**:
  1. `vblank` interrupt driver (`main.asm` line 273, address `$801B` / `$8038`)
  2. `chooseRandomHole_player1` in Type B setup (`main.asm` line 901, address `$8497`)
  3. `chooseRandomHole_player2` in Type B setup (`main.asm` line 910, address `$84A1`)
  4. `initPlayfieldIfTypeB` game initialization (`main.asm` line 1248, address `$8770`)
  5. `typeBGarbageInRow` line filler loop (`main.asm` line 1364, address `$8815`)
  6. `typeBGuaranteeBlank` blank cell guarantor (`main.asm` line 1385, address `$8837`)
  7. `pickRandomTetrimino` piece selection reroll (`main.asm` line 2988, address `$9923`)
- **Memory & Register Effects**:
  - Inputs: `X = #rng_seed` (`$17`), `Y = #$02`.
  - Zero-page scratch: `tmp1` (`$0000`) overwritten with `(rng_seed[0] & 0x02)`.
  - RNG state: `rng_seed` (`$0017`–`$0018`) stepped 1 bit via LFSR feedback polynomial.
  - Registers modified: `A` clobbered, `X = $19` (`$17 + 2`), `Y = $00`, flags `N, Z, C` modified.
- **Contrast with `unreferenced_soundRng`**:
  - `unreferenced_soundRng` at `$E09E` is an unreferenced audio LFSR with **zero** callers throughout the ROM.
  - `generateNextPseudorandomNumber` at `$AB47` is the true, authoritative gameplay LFSR driving all piece spawns, Type B garbage holes, and frame-by-frame entropy stepping.

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

### Source 3: `experimental/c6502/rng.c`
```c
#include "nes_target.h"

#pragma static-locals(on)
#pragma code-name ("C_RNG")

/*
 * generateNextPseudorandomNumber ($AB47 in reference ROM)
 * 16-bit LFSR pseudorandom number generator for NES Tetris gameplay.
 *
 * Steps rng_seed ($0017-$0018) by 1 bit using feedback polynomial:
 *   c = (rng_seed[0] bit 1) ^ (rng_seed[1] bit 1)
 *   rng_seed[0] = (rng_seed[0] >> 1) | (c << 7)
 *   rng_seed[1] = (rng_seed[1] >> 1) | ((old_seed[0] bit 0) << 7)
 * Also sets ZP tmp1 ($0000) = (rng_seed[0] & 0x02).
 *
 * Exhaustively proven equivalent across all 65,536 input seeds.
 */
void generateNextPseudorandomNumber(void) {
    uint8_t b0 = ZP_RNG_SEED_LO;
    uint8_t b1 = ZP_RNG_SEED_HI;
    uint8_t c = (b0 ^ b1) & 0x02;

    ZP_TMP1 = (uint8_t)(b0 & 0x02);
    ZP_RNG_SEED_LO = (uint8_t)((b0 >> 1) | (c ? 0x80 : 0x00));
    ZP_RNG_SEED_HI = (uint8_t)((b1 >> 1) | ((b0 & 0x01) ? 0x80 : 0x00));
}
```

### Toolchain Compilation
```bash
# Compile C routines to 6502 assembly with cc65
cc65 -I experimental/c6502 -t none -O -Oi experimental/c6502/copy_oam.c -o copy_oam.s
cc65 -I experimental/c6502 -t none -O -Oi experimental/c6502/mute_audio.c -o mute_audio.s
cc65 -I experimental/c6502 -t none -O -Oi experimental/c6502/rng.c -o rng.s

# Assemble to object files with ca65
ca65 copy_oam.s -o copy_oam.o
ca65 mute_audio.s -o mute_audio.o
ca65 rng.s -o rng.o
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
### Linker Slot Capacity Analysis & Blocker Demonstration (`generateNextPseudorandomNumber`)

In contrast to `copyOamStagingToOam` (11 bytes) and `muteAudio` (22 bytes), which compiled to identical byte lengths under `cc65`, `generateNextPseudorandomNumber` presents a fundamental architectural constraint:

1. **Fixed ROM Slot**: In the authoritative cartridge layout, `generateNextPseudorandomNumber` is situated at `$AB47`–`$AB5D` (23 bytes, `$17` hex), bounded directly by `copyOamStagingToOam` at `$AB5E`.
2. **Compiled Code Footprint**: The bare-metal C implementation in `experimental/c6502/rng.c` compiles to **62 bytes** under `cc65` and `ca65`.
3. **Footprint Expansion**: `62 - 23 = +39` bytes (+169.6% code expansion).
4. **Collision Impact**:
   - Placing `C_RNG` at `$AB47` causes object code to extend to `$AB84`.
   - This directly overwrites `copyOamStagingToOam` at `$AB5E` (11 bytes) and clobbers the first 28 bytes of `pollController_actualRead` at `$AB69`.
5. **Relocation Constraints**:
   - Shifting subsequent routines down by 39 bytes would displace all downstream code and jump tables in `PRG_chunk1`, invalidating all caller addresses and absolute pointers.
   - Moving the routine out-of-line into unreferenced ROM space would alter its entry point, requiring manual redirection of all 7 caller sites across multiple files.
6. **Safety Decision**:
   In strict accordance with the safety requirements:
   > *"If safe placement is impossible without a broader relocation, stop and document the blocker rather than risking unrelated code. If exact behavior or safe placement cannot be demonstrated, leave the existing build intact and document the blocker."*
   In-place integration of `generateNextPseudorandomNumber` is formally designated **`BLOCKED`**. The experimental ROM retains its bit-exact baseline with the two proven C routines (`copyOamStagingToOam` and `muteAudio`).

---

## 4. Machine-Code & Behavioral Comparison

### Machine-Code Byte Comparison

#### Routine 1: `copyOamStagingToOam` (`$AB5E`, 11 bytes)
| Offset | CPU Address | Original Assembly Opcode | Compiled C Opcode | Byte Hex | Status |
|:---:|:---:|:---|:---|:---:|:---:|
| `+0x00` | `$AB5E` | `lda #$00` | `lda #$00` | `A9 00` | **MATCH** |
| `+0x02` | `$AB60` | `sta $2003` | `sta $2003` | `8D 03 20` | **MATCH** |
| `+0x05` | `$AB63` | `lda #$02` | `lda #$02` | `A9 02` | **MATCH** |
| `+0x07` | `$AB65` | `sta $4014` | `sta $4014` | `8D 14 40` | **MATCH** |
| `+0x0A` | `$AB68` | `rts` | `rts` | `60` | **MATCH** |

#### Routine 2: `muteAudio` (`$E271`, 22 bytes)
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

#### Routine 3: `generateNextPseudorandomNumber` (`$AB47`, 23 bytes vs 62 bytes)
| Implementation | Size | Machine Code / Instructions | Analysis |
|:---|:---:|:---|:---|
| **Original 6502 Assembly** | **23 B** | `B5 00 29 02 85 00 B5 01 29 02 45 00 18 F0 01 38 76 00 E8 88 D0 F9 60` | Highly optimized 2-byte loop utilizing `ror tmp1,x`, register indexing, and carry chaining. |
| **Compiled Target C (`cc65`)** | **62 B** | 27 instructions (`lda`, `sta`, `eor`, `and`, `lsr`, `ora`, branches, `ptr1` references) + 3B BSS | C lacks rotate-through-carry. Shifts and bit manipulation decompose into multi-instruction sequences. |

### Technical Analysis of Compiler Code Expansion
1. **Lack of Hardware Rotate-Through-Carry (`ROR`) in C**: The 6502 `ror` instruction simultaneously shifts an 8-bit memory location and chains the carry flag into the next byte's rotation. Standard C requires separate shift operations (`>> 1`), bit extractions, conditional branches, and bitwise ORs (`| 0x80`), tripling code size.
2. **Calling Convention Mismatch**: The 6502 original takes arguments in registers `X` (`#rng_seed` = `$17`) and `Y` (`#$02`). `cc65` C functions do not natively support register-parameter calling conventions without wrapper trampolines that add further byte overhead.
3. **Compiler Zero-Page Scratch Allocation**: `cc65` emits references to internal pointer `ptr1` (`$0008`), introducing external runtime dependencies not present in the original game's zero-page memory map.

### Call Site Audits
- **`$801B`**: `20 5E AB` (`jsr $AB5E`) — NMI VBlank handler (targets `copyOamStagingToOam`)
- **`$E1E8`**: `20 71 E2` (`jsr $E271`) — `initAudioAndMarkInited` (targets `muteAudio`)
- **`$E247`**: `20 71 E2` (`jsr $E271`) — `muteAudioAndClearTriControl` (targets `muteAudio`)
- **`$AB47`**: `20 47 AB` (`jsr $AB47`) — 7 gameplay callers target original verified 23-byte LFSR.

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

# Full suite (Baseline ROM + POC + Experimental ROM):
python3 tools/build_rom.py all
```

### 2. Differential CPU Simulation & Algorithm Verification
- **Exhaustive LFSR Seed Space**:
  All 65,536 possible 16-bit seeds (`0x0000` to `0xFFFF`) verified between the 6502 assembly algorithm and the target C implementation:
  **0 mismatches across 65,536 / 65,536 seeds (100.0% behavioral equivalence)**.
- **Continuous 1,000-Step Sequence Differential Test**:
  Integrated into `tests/verification/test_differential.c` (Test 1b):
  1,000 consecutive iterations of the LFSR executed simultaneously in the NMOS 6502 differential CPU simulator and the C routine, asserting exact match of `rng_seed[0]`, `rng_seed[1]`, and scratch variable `tmp1` ($0000) at every step.
- **`copyOamStagingToOam` ($AB5E)**:
  `A` = `0x02`, `oam_addr` = `0x00`, clean RTS return, 0 side effects.
- **NMI Vector ($8005)**:
  Invokes `$AB5E`, increments `frameCounter` (`$0020` -> `$0021`), RNG seed updated, 0 side effects.
- **`muteAudio` ($E271)**:
  All audio channels silenced (`SQ1_VOL`=$10, `SQ2_VOL`=$10, `NOISE_VOL`=$10, `TRI_LINEAR`=$00, `DMC_RAW`=$00), clean RTS return.

### 3. Emulator Playability Status
- **Status**: **`BLOCKED`**
- **Reason**: The Linux execution environment is a headless CLI container with no X11/Wayland display server and no emulator binaries installed (`fceux`, `nestopia`, `mednafen`, `mesen` are absent from `PATH`).
- **Playability vs. Simulation**: Subroutine-level and interrupt-level execution is verified via automated 6502 CPU differential simulation; interactive game playability is explicitly marked `BLOCKED`.

---

## 6. Integration Summary & Next Candidates

| Routine | Address | 6502 Size | Target C Size | Status | Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| **`copyOamStagingToOam`** | `$AB5E` | 11 B | 11 B | **INTEGRATED** | 100% bit-exact machine code in experimental ROM |
| **`muteAudio`** | `$E271` | 22 B | 22 B | **INTEGRATED** | 100% bit-exact machine code in experimental ROM |
| **`generateNextPseudorandomNumber`** | `$AB47` | 23 B | 62 B | **BLOCKED** | Exhaustively verified; blocked by 23B slot limit |

### Next Integration Candidates (Straight-Line Register / Jump Routines)
1. **`soundEffectSlot2_makesNoSound`** (`$E244`, 3 bytes):
   - Jumps directly to `muteAudioAndClearTriControl` (`jmp $E247`).
2. **`unreferenced_soundRng`** (`$E09E`, 23 bytes):
   - Audio LFSR random number generator; behaviorally equivalent but subject to the same ~60B compiler expansion.
3. **`updateAudioWaitForNmiAndEnablePpuRendering`** (`$988E`, 11 bytes):
   - Straight-line helper routine setting PPU mask registers and waiting for VBlank.
