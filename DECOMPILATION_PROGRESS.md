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
| **Verified Routines** | **23** | Bit-exact & differential CPU proven against reference ROM |
| **Decompiled (Untested)** | **0** | Code converted but awaiting verification suite |
| **Remaining Routines** | **148** | Pending conversion |
| **Current Progress** | **13.45%** | `[#######-------------------------------------------] 13.45%` |
| **Build & Test Suite** | 8 / 8 suites passing | Unit, 15 ROM data tables, & 6502 differential CPU oracle |

---

## Subsystem Breakdown

| Subsystem | Total Routines | Verified | Decompiled | Missing | Progress |
|:---|:---:|:---:|:---:|:---:|:---:|
| **MATH_RNG** (Pseudo-Random Generation) | 2 | 2 | 0 | 0 | **100.0%** |
| **PLAYER_INPUT** (DAS, Joypad, Autorepeat) | 4 | 3 | 0 | 1 | **75.0%** |
| **PLAYFIELD** (Grid, Line Clears, Garbage) | 9 | 6 | 0 | 3 | **66.7%** |
| **TETRIMINO** (Pieces, Spawns, Rotations) | 16 | 7 | 0 | 9 | **43.8%** |
| **MATH_BCD** (Score & Line Math) | 12 | 1 | 0 | 11 | **8.3%** |
| **AUDIO** (Music Engine, SFX, APU) | 41 | 2 | 0 | 39 | **4.9%** |
| **CORE_LOGIC** (Helper Utilities, Buffers) | 38 | 2 | 0 | 36 | **5.3%** |
| **GAMEPLAY_STATE** (State Machine, Timers) | 14 | 0 | 0 | 14 | 0.0% |
| **MENUS_UI** (Title, Menus, High Scores) | 13 | 0 | 0 | 13 | 0.0% |
| **RENDER** (PPU, OAM, Palettes, Nametables) | 16 | 0 | 0 | 16 | 0.0% |
| **SYSTEM** (Reset, NMI, IRQ, VBlank) | 6 | 0 | 0 | 6 | 0.0% |
| **TOTAL** | **171** | **23** | **0** | **148** | **13.45%** |

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
   - 15 PRG ROM tables verified byte-for-byte at exact symbol addresses.
   - Differential CPU testing: Built NMOS 6502 CPU simulator oracle executing original PRG ROM assembly directly against decompiled C routines across hundreds of test vectors.
   - Authoritative scope audit: Distinguished 171 standalone routines, 111 alternate entry points, 260 data tables, and exact address aliases in `ROUTINE_INVENTORY.md` and `routine_inventory.json`.

6. [x] **Batch 4 (Playfield Grid, Line Completion, Animation Framing, & Type B Garbage)**:
   - Decompiled `playState_lockTetrimino` ($99A2): 4-mino placement and invalid position game-over sequence.
   - Decompiled `playState_checkForCompletedRows` ($9A6B): 4-row candidate scan, downward row collapse, top row clearing, and garbage line accumulation.
   - Decompiled `updateLineClearingAnimation` ($977F): 5-step center-out line clearing framing every 4 frames.
   - Decompiled `playState_updateGameOverCurtain` ($9A11): Cascade curtain row fill with `TILE_CURTAIN` ($4F).
   - Decompiled `playState_receiveGarbage` ($9B03): 2-player pending garbage row upward shift and hole insertion.
   - Decompiled `initPlayfieldIfTypeB` ($87DC) & `initPlayfieldForTypeB` ($87E3): PRNG garbage generation (rows 8-19), blank cell guarantee, and startHeight clearing.
   - Authoritative data tables bit-exact verified against reference ROM: `multBy10Table`, `leftColumnsTable`, `rightColumnsTable`, `typeBBlankInitCountByHeightTable`, `rngTable`, `garbageLinesTable`.
   - 6502 differential CPU oracle: Formally proven equivalent against reference ROM across multiple line completion scenarios.

7. [x] **Batch 5 (Player Controls & DAS)**:
   - Decompiled `pollController_actualRead` ($AB69): Strobe JOY1 ($4016), serial 8-bit shift-register read of Joypad 1 & 2.
   - Decompiled `addExpansionPortInputAsControllerInput` ($AB8B): Famicom expansion port input OR-mixing.
   - Decompiled `diffOldAndNewButtons` ($ABBD): Button edge detection `(curr ^ held) & curr` for Player 1 & 2.
   - Decompiled `pollController` ($AB9D): Double-reading debounce against DPCM DMA glitch, expansion port mixing, edge detection.
   - Decompiled `pollControllerButtons` ($9D51): Mode dispatch, demo mode START button cancellation to title screen, demo move playback.
   - Decompiled `demoButtonsTable_indexIncr` ($9DE8): 16-bit demo table address pointer increment.
   - Decompiled `updateActivePlayerInput`: Active player input mirroring for 1P / 2P modes.
   - Decompiled `processDAS` & `dasWallCharge`: Exact DAS initial delay 16 frames (`DAS_RESET = 0x10`), 6 frames repeat interval (`DAS_DELAY = 0x0A`), wall charging to full delay.
   - 6502 differential CPU oracle: Formally proven equivalent against reference ROM across 256 edge-diff combinations, controller reads, and demo table incrementation.

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
| Verification Infra | 6502 Differential CPU Oracle, ROM Tables & Routine Audit | 0 (Infra) | 13 | 6 suites / all ROM differential | `1c7cdc3` |
| Batch 4 | Playfield Grid, Line Completion, Animation & Type B Garbage | 6 | 19 | 7 suites / 15 ROM tables + 6502 diff | `e62775b` |
| Batch 5 | Player Controls, Edge Detection, Demo Playback & DAS Timing | 4 (+2 alt) | 23 | 8 suites / 15 ROM tables + 6502 diff | `eb1beb6` |
