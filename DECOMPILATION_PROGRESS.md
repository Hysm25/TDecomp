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
| **Total Standalone Routines** | **171** | Authoritative routines with independent execution scope |
| **Alternate Entry Points** | **111** | Secondary entry points into parent routines / shared logic |
| **Authoritative Data Tables** | **260** | Lookup, geometry, sprite, and audio tables in PRG ROM |
| **Total PRG Symbols Tracked** | **567** | Complete PRG ROM symbol census |
| **Verified Routines** | **13** | Bit-exact & differential CPU proven against reference ROM |
| **Decompiled (Untested)** | **0** | Code converted but awaiting verification suite |
| **Remaining Routines** | **158** | Pending conversion |
| **Current Progress** | **7.60%** | `[####----------------------------------------------] 7.60%` |
| **Build & Test Suite** | 6 / 6 suites passing | Unit, 9 ROM data tables, & 6502 differential CPU oracle |

---

## Subsystem Breakdown

| Subsystem | Total Routines | Verified | Decompiled | Missing | Progress |
|:---|:---:|:---:|:---:|:---:|:---:|
| **MATH_RNG** (Pseudo-Random Generation) | 2 | 2 | 0 | 0 | **100.0%** |
| **TETRIMINO** (Pieces, Spawns, Rotations) | 16 | 6 | 0 | 10 | **37.5%** |
| **PLAYFIELD** (Grid, Collision, Lines) | 9 | 1 | 0 | 8 | **11.1%** |
| **MATH_BCD** (Score & Line Math) | 12 | 1 | 0 | 11 | **8.3%** |
| **AUDIO** (Music Engine, SFX, APU) | 41 | 2 | 0 | 39 | **4.9%** |
| **CORE_LOGIC** (Helper Utilities, Buffers) | 38 | 1 | 0 | 37 | **2.6%** |
| **GAMEPLAY_STATE** (State Machine, Timers) | 14 | 0 | 0 | 14 | 0.0% |
| **MENUS_UI** (Title, Menus, High Scores) | 13 | 0 | 0 | 13 | 0.0% |
| **RENDER** (PPU, OAM, Palettes, Nametables) | 16 | 0 | 0 | 16 | 0.0% |
| **SYSTEM** (Reset, NMI, IRQ, VBlank) | 6 | 0 | 0 | 6 | 0.0% |
| **PLAYER_INPUT** (DAS, Joypad, Autorepeat) | 4 | 0 | 0 | 4 | 0.0% |
| **TOTAL** | **171** | **13** | **0** | **158** | **7.60%** |

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

3. [x] **Batch 2 (BCD Math, Scoring, & Line Calculations)**:
   - Decompiled `bcd_add8` and `bcd_sub8` low-level operations.
   - Decompiled `addHoldDownPoints` (soft drop BCD accumulation).
   - Decompiled `addLineClearPoints` (multi-line BCD multipliers & 999,999 clamp).
   - Decompiled `updateLinesAndLevel` (Type A tens-boundary level up & Type B decrement).
   - Decompiled `playState_updateLinesAndStatistics` (PlayState 4 complete handler).
   - Decompiled `updateMusicSpeed` (row 5 height detection & danger allegro music).
   - Decompiled `setMusicTrack` (track selection and demo mode mute).

4. [x] **Batch 3 (Tetrimino Geometry, Rotation, Movement & Collision)**:
   - Full 19-orientation geometry table and 38-entry rotation table.
   - Decompiled `isPositionValid` (bounding box, wall, floor, and mino collision).
   - Decompiled `rotate_tetrimino` (Button A clockwise, Button B counter-clockwise, wall revert).
   - Decompiled `shift_tetrimino` (Left/Right shifting with DAS auto-repeat).
   - Decompiled `drop_tetrimino` (natural gravity, soft drop acceleration, floor lock).
   - Decompiled `updatePlayfield` (VRAM dirty row tracker).
   - Decompiled `getDropSpeed` (levels 0-29 gravity, killscreen).

5. [x] **Verification Infrastructure & Routine Scope Audit**:
   - Reference ROM verification: Bit-for-bit extraction and validation against `Tetris (USA).nes`.
   - 9 PRG ROM tables verified byte-for-byte at exact symbol addresses (`orientationTable`, `rotationTable`, `framesPerDropTable`, `pointsTable`, `spawnTable`, `tetriminoTypeFromOrientation`, `spawnOrientationFromOrientation`, `musicSelectionTable`, `demoTetriminoTypeTable`).
   - Differential CPU testing: Built NMOS 6502 CPU simulator oracle executing original PRG ROM assembly directly against decompiled C routines across hundreds of test vectors.
   - Authoritative scope audit: Distinguished 171 standalone routines, 111 alternate entry points, 260 data tables, and exact address aliases in `ROUTINE_INVENTORY.md` and `routine_inventory.json`.

6. [ ] **Batch 4 (Playfield Grid & Line Clear)**:
   - Playfield representation ($0400, 10x20 cells).
   - Row completion detection (`playState_checkForCompletedRows`).
   - Line clear animation framing (`playState_lineClearAnimation`).
   - Row collapsing (`collapseRows`).
   - Type B garbage generation (`initPlayfieldForTypeB`).
   - Differential testing against 6502 assembly in reference ROM.

7. [ ] **Batch 5 (Player Controls & DAS)**:
   - Joypad state decoding.
   - Autorepeat timing (DAS initial delay 16 frames, repeat 6 frames; NTSC vs PAL).

8. [ ] **Batch 6 (Gameplay State Machine)**:
   - `playState` machine (Active -> Lock -> Check Rows -> Clear Animation -> Update Scores -> Spawn -> Curtain / Game Over).
   - Two-player / Type A vs Type B state handling.

9. [ ] **Batch 7 (Menus, High Scores & Systems)**:
   - Legal screen, title screen, menu navigation.
   - High score table sorting and storage ($0700).

10. [ ] **Batch 8 (PPU Rendering & OAM)**:
    - OAM sprite staging.
    - Nametable row updates.

11. [ ] **Batch 9 (Audio & Sound Effects)**:
    - APU sound effects and music sequencing.

---

## Verified Batch Log

| Batch # | Description | Routines Added | Cumulative Verified | Tests Passing | Commit |
|:---|:---|:---:|:---:|:---:|:---:|
| Baseline | Initial repository configuration & routine census | 0 | 0 | N/A | `985e8df` |
| Batch 0 & 1 | Infrastructure, NES State, RNG & Piece Selection | 5 | 5 | 2 suites / 11 test cases | `275a177` |
| Batch 2 | BCD Math, Scoring, Lines Progression, & Music Speed | 3 | 8 | 3 suites / 17 test cases | `d3110ca` |
| Batch 3 | Tetrimino Geometry, Rotation, Movement & Collision | 5 | 13 | 4 suites / 26 test cases | `17c2e00` |
| Verification Infra | 6502 Differential CPU Oracle, ROM Tables & Routine Audit | 0 (Infra) | 13 | 6 suites / all ROM differential | Pending |
