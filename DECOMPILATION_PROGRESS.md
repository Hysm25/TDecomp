# NES Tetris Decompilation Progress Tracker

## Project Overview

- **Project:** NES Tetris (USA) Decompilation
- **Source of Truth:** `TetrisNESDisasm/main.asm` (NES 6502 Assembly, NROM/MMC1)
- **Reference ROM:** `Tetris (USA).nes` (SHA-1: `3026d28b63d94c921fe58364f8b0659d10b5a0ac`, PRG payload verified bit-exact)
- **Target Specification:** Strict C11 (`-std=c11 -Wall -Wextra -Werror -pedantic`), modular, portable to Linux, Windows, macOS, Android.
- **Git Repository:** `Hysm25/TDecomp` on `origin/main`

---

## Executive Status Dashboard

| Metric | Status | Details |
|:---|:---:|:---|
| **Total Standalone Routines** | **205** | Inventory extracted from disassembly and symbol table |
| **Verified Routines** | **5** | Mathematically and behaviorally proven against ROM/ASM |
| **Decompiled (Untested)** | **0** | Code converted but awaiting verification suite |
| **Remaining Routines** | **200** | Pending conversion |
| **Current Progress** | **2.44%** | `[#-------------------------------------------------] 2.44%` |
| **Build & Test Suite** | 2 / 2 suites passing | Strict C11, zero warnings, Termux/Android portable |

---

## Subsystem Breakdown

| Subsystem | Total Routines | Verified | Decompiled | Missing | Progress |
|:---|:---:|:---:|:---:|:---:|:---:|
| **MATH_RNG** (Pseudo-Random Generation) | 2 | 2 | 0 | 0 | **100.0%** |
| **MATH_BCD** (Score & Line Math) | 9 | 0 | 0 | 9 | 0.0% |
| **TETRIMINO** (Pieces, Spawns, Rotations) | 19 | 3 | 0 | 16 | **15.8%** |
| **PLAYFIELD** (Grid, Collision, Lines) | 11 | 0 | 0 | 11 | 0.0% |
| **PLAYER_INPUT** (DAS, Joypad, Autorepeat) | 4 | 0 | 0 | 4 | 0.0% |
| **GAMEPLAY_STATE** (State Machine, Timers) | 15 | 0 | 0 | 15 | 0.0% |
| **MENUS_UI** (Title, Menus, High Scores) | 13 | 0 | 0 | 13 | 0.0% |
| **CORE_LOGIC** (Helper Utilities, Buffers) | 51 | 0 | 0 | 51 | 0.0% |
| **SYSTEM** (Reset, NMI, IRQ, VBlank) | 6 | 0 | 0 | 6 | 0.0% |
| **RENDER** (PPU, OAM, Palettes, Nametables) | 19 | 0 | 0 | 19 | 0.0% |
| **AUDIO** (Music Engine, SFX, APU) | 56 | 0 | 0 | 56 | 0.0% |
| **TOTAL** | **205** | **5** | **0** | **200** | **2.44%** |

---

## Decompilation Roadmap (Shortest Reliable Path)

1. [x] **Batch 0 (Foundation & Infrastructure)**:
   - Established C project structure: `include/`, `src/`, `tests/`, `CMakeLists.txt`.
   - Core hardware abstraction: `NES_State`, memory mapping, CPU register modeling.
   - Comprehensive constants matching `constants.asm` and `tetris-ram.asm`.
   - Verified memory mirroring, pointer safety, and 16-bit little-endian accessors.

2. [x] **Batch 1 (RNG & Piece Generation)**:
   - Decompiled `generateNextPseudorandomNumber` (LFSR algorithm).
   - Decompiled `unreferenced_soundRng` (audio LFSR variant).
   - Decompiled `pickRandomTetrimino` (spawn reroll and modulo-7 logic).
   - Decompiled `chooseNextTetrimino` (demo stream vs gameplay branching).
   - Decompiled `incrementPieceStat` (BCD piece counters).
   - Unit tests verified with edge cases, anti-repetition rerolls, and BCD carry.

3. [ ] **Batch 2 (BCD Math & Scoring)**:
   - `addBCD` / `updateScore`.
   - `updateLines` / `levelNumber` advancement.
   - Drop point calculations.

4. [ ] **Batch 3 (Tetrimino Geometry, Rotation & Collision)**:
   - Piece definitions and orientations (19 orientations, 7 tetrominoes).
   - `lookupPieceLocation` and bounding box math.
   - `tetriminoFitCheck` (collision detection against playfield bounds and placed minos).
   - `rotateTetrimino`, `moveTetriminoLeft`, `moveTetriminoRight`, `moveTetriminoDown`.

5. [ ] **Batch 4 (Playfield Grid & Line Clear)**:
   - Playfield representation ($0400, 10x20 cells).
   - Row completion detection (`playState_checkForCompletedRows`).
   - Line clear animation framing (`playState_lineClearAnimation`).
   - Row collapsing (`collapseRows`).
   - Type B garbage generation (`initPlayfieldForTypeB`).

6. [ ] **Batch 5 (Player Controls & DAS)**:
   - Joypad state decoding.
   - Autorepeat timing (DAS initial delay 16 frames, repeat 6 frames; NTSC vs PAL).
   - Drop speed table lookup.

7. [ ] **Batch 6 (Gameplay State Machine)**:
   - `playState` machine (Active -> Lock -> Check Rows -> Clear Animation -> Update Scores -> Spawn -> Curtain / Game Over).
   - Two-player / Type A vs Type B state handling.

8. [ ] **Batch 7 (Menus, High Scores & Systems)**:
   - Legal screen, title screen, menu navigation.
   - High score table sorting and storage ($0700).

9. [ ] **Batch 8 (PPU Rendering & OAM)**:
   - OAM sprite staging.
   - Nametable row updates.

10. [ ] **Batch 9 (Audio & Sound Effects)**:
    - APU sound effects and music sequencing.

---

## Verified Batch Log

| Batch # | Description | Routines Added | Cumulative Verified | Tests Passing | Commit |
|:---|:---|:---:|:---:|:---:|:---:|
| Baseline | Initial repository configuration & routine census | 0 | 0 | N/A | `985e8df` |
| Batch 0 & 1 | Infrastructure, NES State, RNG & Piece Selection | 5 | 5 | 2 suites / 11 test cases | Pending |
