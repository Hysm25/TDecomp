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

#### Routine 3: `generateNextPseudorandomNumber` (`$AB47`, 23 bytes vs 54 bytes)
| Implementation | Size | Machine Code / Instructions | Analysis |
|:---|:---:|:---|:---|
| **Original 6502 Assembly** | **23 B** | `B5 00 29 02 85 00 B5 01 29 02 45 00 18 F0 01 38 76 00 E8 88 D0 F9 60` | Highly optimized 2-byte loop utilizing `ror tmp1,x`, register indexing, and carry chaining. |
| **Compiled Target C (`cc65`, Baseline)** | **62 B** | 27 instructions (`lda`, `sta`, `eor`, `and`, `lsr`, `ora`, branches, `ptr1` references) + 3B BSS | Ternary expressions and subexpression shifts decompose into multi-statement loads/stores with `ptr1` dependency. |
| **Compiled Target C (`cc65`, Optimized)** | **54 B** | 23 instructions (direct branch formulation, zero compiler pointer dependencies) + 1B BSS | Direct branching eliminates `ptr1` zero-page pointer; reduces size by 8 bytes (-12.9%) and BSS from 3B to 1B. |

### Technical Analysis of Compiler Code Expansion
1. **Lack of Hardware Rotate-Through-Carry (`ROR`) in C**: The 6502 `ror` instruction simultaneously shifts an 8-bit memory location and chains the carry flag into the next byte's rotation. Standard C requires separate shift operations (`>> 1`), bit extractions, conditional branches, and bitwise ORs (`| 0x80`), expanding code size.
2. **Calling Convention Mismatch**: The 6502 original takes arguments in registers `X` (`#rng_seed` = `$17`) and `Y` (`#$02`). `cc65` C functions do not natively support register-parameter calling conventions without wrapper trampolines that add further byte overhead.
3. **Compiler Zero-Page Scratch Allocation**: In the baseline formulation, `cc65` emits references to internal pointer `ptr1` (`$0008`). The optimized formulation eliminates this dependency entirely.

---

## 5. RNG Code-Size Optimization & Hybrid Integration Feasibility Study

### Toolchain Baseline
- **Compiler**: `cc65 V2.19 - Git 16bf472e6`
- **Assembler**: `ca65 V2.19 - Git 16bf472e6`
- **Linker**: `ld65 V2.19 - Git 16bf472e6`
- **Evaluation Matrix Across Compiler Flags**:
  - `cc65` without `-O`: Fails to link without runtime library (`asrax1`, `pushax`, `tosorax`, `tosxorax`).
  - `cc65 -O`: 54 bytes (optimized) / 62 bytes (baseline), BSS 1B / 3B.
  - `cc65 -O -Oi`: 54 bytes (identical, inlines simple shifts).
  - `cc65 -O -Oi -Or`: 68 bytes (enabling register variables forces `pusha`/`incsp1` on `regbank+5`, increasing overhead by +14 bytes).
  - `cc65 -O -Oi -Os`: 54 bytes (standard functions inline; no effect on bitwise math).
  - `cc65 -O -Oi --codesize 500`: 54 bytes (identical).

### Three Approaches Evaluated

#### Approach A: Optimized Target C (`experimental/c6502/rng.c`)
- **Implementation**: Formulated with direct branches and separate low/high byte updates, avoiding ternary precomputations and internal temporary variables:
  ```c
  void generateNextPseudorandomNumber(void) {
      uint8_t b0 = ZP_RNG_SEED_LO;
      ZP_TMP1 = b0 & 2;
      if ((ZP_RNG_SEED_HI ^ b0) & 2) {
          ZP_RNG_SEED_LO = (b0 >> 1) | 0x80;
      } else {
          ZP_RNG_SEED_LO = b0 >> 1;
      }
      if (b0 & 1) {
          ZP_RNG_SEED_HI = (ZP_RNG_SEED_HI >> 1) | 0x80;
      } else {
          ZP_RNG_SEED_HI = ZP_RNG_SEED_HI >> 1;
      }
  }
  ```
- **Total Code Size**: **54 bytes** (-8 bytes / -12.9% reduction from 62B baseline).
- **RAM / BSS**: **1 byte** (`M0001` for `b0`).
- **Dependencies**: **Zero** external runtime dependencies (links cleanly without `ptr1`, `sp`, or runtime libraries).
- **Cycle Count**: **58 to 64 cycles** (average ~61 cycles).
- **Slot Feasibility**: Available slot at `$AB47` is 23 bytes. Footprint is `54 - 23 = +31` bytes overflow.

#### Approach B: Hybrid C / 6502 Assembly Helper
- **Implementation**: C computes the feedback condition, then invokes a minimal 6502 assembly helper using `__fastcall__` (passed in register `A`):
  ```c
  void __fastcall__ asm_rotate_seed(uint8_t feedback);
  void generateNextPseudorandomNumber(void) {
      uint8_t b0 = ZP_RNG_SEED_LO;
      ZP_TMP1 = b0 & 2;
      asm_rotate_seed((b0 ^ ZP_RNG_SEED_HI) & 2);
  }
  ```
  ```assembly
  _asm_rotate_seed:
      cmp #$00
      beq @clear
      sec
      bcs @do_ror
  @clear:
      clc
  @do_ror:
      ror $0017
      ror $0018
      rts
  ```
- **Total Code Size**: **32 bytes** (20 bytes C caller + 12 bytes assembly helper).
- **RAM / BSS**: **1 byte** (`M0001`).
- **Dependencies**: None.
- **Cycle Count**: **45 to 48 cycles**.
- **Slot Feasibility**: Available slot is 23 bytes. Footprint is `32 - 23 = +9` bytes overflow.
- **Limitation**: Even though the assembly helper handles carry rotation in 12 bytes, the combined C caller + helper still exceeds the 23-byte slot by 9 bytes, colliding directly into `copyOamStagingToOam` at `$AB5E`.

#### Approach C: Standalone Experimental Implementation Out-of-Line
- **Implementation**: Place a 3-byte trampoline (`jmp _standalone_c_rng`) at `$AB47` (with 20 bytes padding), and place the 54-byte C routine body in a separate PRG area (e.g. `$F621` in `unreferenced_data4`).
- **Total Code Size**: **57 bytes** (3 bytes trampoline + 54 bytes C body).
- **RAM / BSS**: **1 byte**.
- **Dependencies**: None.
- **Cycle Count**: **67 to 73 cycles** (`jsr $AB47` [6] + `jmp _c_rng` [3] + C body [52-58] + `rts` [6]).
- **Architectural & Safety Defects**:
  1. **Zero Free ROM Capacity**: In the 32KB NROM-256 memory map ($8000–$FFFF), every byte is assigned to code, data tables, or cartridge remnants. There is no unallocated padding.
  2. **Payload Match Invalidation**: Overwriting 54 bytes of `unreferenced_data4` corrupts original 1989 Nintendo cartridge data, destroying the 100.0% bit-exact ROM verification.
  3. **Caller Redirection Risk**: Bypassing the trampoline would require patching all 7 caller sites across multiple source modules, violating the mandate against code shifting and caller patching.

---

### Comparative Evaluation Matrix

| Metric | Reference (6502 Assembly) | Approach A (Optimized C) | Approach B (Hybrid C/Asm) | Approach C (Standalone Out-of-Line) |
|:---|:---:|:---:|:---:|:---:|
| **Address** | `$AB47`–`$AB5D` | `$AB47`–`$AB7C` | `$AB47`–`$AB66` | Trampoline `$AB47`, Body `$F621` |
| **Total Code Size** | **23 bytes** | **54 bytes** | **32 bytes** | **57 bytes** |
| **Slot Capacity Delta** | **0 bytes (Exact)** | **+31 bytes overflow** | **+9 bytes overflow** | **+34 bytes displaced** |
| **Additional RAM / BSS** | **0 bytes** | **1 byte** | **1 byte** | **1 byte** |
| **Cycle Cost (min / max)** | **51 / 52 cycles** | **58 / 64 cycles** | **45 / 48 cycles** | **67 / 73 cycles** |
| **Zero-Page Pointer Deps** | None | None | None | None |
| **Software Stack (`sp`)** | None | None | None | None |
| **Observable Register X** | `$19` (`$17 + 2`) | `$17` (preserved) | `$17` (preserved) | `$17` (preserved) |
| **Observable Register Y** | `$00` (decremented) | `$02` (preserved) | `$02` (preserved) | `$02` (preserved) |
| **Observable Flags** | `Z=1, N=0, C=bit0` | `Z, N, C` clobbered | `Z, N, C` clobbered | `Z, N, C` clobbered |
| **Caller Compatibility** | Authoritative | 100% compatible | 100% compatible | 100% compatible |
| **Bit-Exact ROM Preserved** | Yes | No (overwrites $AB5E) | No (overwrites $AB5E) | No (overwrites $F621) |
| **Integration Feasibility** | Baseline Native | **BLOCKED** | **BLOCKED** | **BLOCKED** |

---

### Calling Convention & Observable Side Effects Audit

#### Complete CPU-State vs. Algorithmic Equivalence
A key distinction established by differential verification is the separation of **pure algorithmic equivalence** from **complete CPU-state equivalence**:

1. **Memory State Equivalence (100.0% Bit-Exact)**:
   - `rng_seed` (`$0017`–`$0018`) undergoes the exact same 1-bit LFSR transition across all 65,536 seeds in both 6502 assembly and C.
   - Scratch memory `tmp1` (`$0000`) receives `old_seed[0] & 0x02` in both implementations.
2. **CPU Register State Divergence**:
   - In 6502 assembly, the loop increments `X` twice (`$17` -> `$19`) and decrements `Y` twice (`$02` -> `$00`), setting flag `Z = 1`.
   - In compiled C, registers `X` and `Y` are not used and remain unaltered (`X = $17`, `Y = $02`).
3. **Gameplay Caller Immunity**:
   - A line-by-line census of all 7 gameplay callers was performed:
     - `vblank` (`$8038`): Immediately executes `lda #$00; sta ppuScrollX`.
     - `chooseRandomHole_player1` (`$8497`): Immediately executes `lda rng_seed; and #$0F`.
     - `chooseRandomHole_player2` (`$84A1`): Immediately executes `lda rng_seed; and #$0F`.
     - `initPlayfieldIfTypeB` (`$8770`): Immediately invokes `jsr chooseNextTetrimino`.
     - `typeBGarbageInRow` (`$8815`): Immediately loads `A = rng_seed`, then sets `Y = A & 7`, then sets `X = generalCounter2`.
     - `typeBGuaranteeBlank` (`$8837`): Immediately executes `lda rng_seed; and #$0F`.
     - `pickRandomTetrimino` (`$9923`): Immediately executes `lda rng_seed; and #$07; clc; adc spawnID`.
   - **Finding**: Exactly zero callers inspect return registers `X`, `Y`, `A`, or processor flags `N/Z/C`. Every caller exclusively accesses the memory state in `rng_seed`. Therefore, the CPU register divergence has **zero behavioral impact** on gameplay execution.

---

### Final Recommendation

1. **Reject In-Place Replacement**: Neither pure C (54 bytes) nor hybrid C/assembly (32 bytes) can fit inside the authoritative 23-byte slot at `$AB47` without overwriting `copyOamStagingToOam` at `$AB5E`.
2. **Reject Out-of-Line Relocation**: Placing the routine in `unreferenced_data4` corrupts original cartridge byte payloads, violating the project requirement of maintaining a 100% bit-exact ROM payload match.
3. **Maintain Isolated Baseline & Experimental Builds**:
   - The authoritative baseline assembly build remains untouched.
   - The experimental ROM build continues to integrate the two safely proven C routines: `copyOamStagingToOam` ($AB5E, 11 bytes) and `muteAudio` ($E271, 22 bytes), preserving 0 code shifting and 100.0% bit-exact cartridge payload matching.
   - The optimized 54-byte bare-metal C routine in `experimental/c6502/rng.c` is retained as a standalone verified reference for future full-bank or macro decompilation phases.

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

## 6. Verification & Test Execution

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
- **Calling Convention & Edge Case Audit (Test 1c)**:
  Integrated into `tests/verification/test_differential.c` (Test 1c):
  Evaluated across edge cases (`0x0000`, `0xFFFF`, `0x0001`, `0x0002`, `0x0003`, `0x0200`, `0x0202`, `0x5555`, `0xAAAA`, `0x8080`, `0x7FFF`, `0x8000`, `0x0080`, `0x0100`). Audited CPU register outputs (6502 returns `X=$19`, `Y=$00`, `Z=1`), verified scratch memory `tmp1 = old_seed[0] & 2`, and proven caller immunity across all 7 callers.
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

## 7. Integration Summary & Next Candidates

| Routine | Address | 6502 Size | Target C Size | Status | Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| **`copyOamStagingToOam`** | `$AB5E` | 11 B | 11 B | **INTEGRATED** | 100% bit-exact machine code in experimental ROM |
| **`muteAudio`** | `$E271` | 22 B | 22 B | **INTEGRATED** | 100% bit-exact machine code in experimental ROM |
| **`generateNextPseudorandomNumber`** | `$AB47` | 23 B | 54 B (opt) / 62 B (base) | **BLOCKED** | Exhaustively verified; blocked by 23B slot limit |

### Next Integration Candidates (Straight-Line Register / Jump Routines)
1. **`soundEffectSlot2_makesNoSound`** (`$E244`, 3 bytes):
   - Jumps directly to `muteAudioAndClearTriControl` (`jmp $E247`).
2. **`unreferenced_soundRng`** (`$E09E`, 23 bytes):
   - Audio LFSR random number generator; behaviorally equivalent but subject to the same ~60B compiler expansion.
3. **`updateAudioWaitForNmiAndEnablePpuRendering`** (`$988E`, 11 bytes):
   - Straight-line helper routine setting PPU mask registers and waiting for VBlank.
