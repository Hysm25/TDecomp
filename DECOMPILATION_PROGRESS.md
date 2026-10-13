# NES Tetris Decompilation Progress

## Overview

- **Source of Truth:** `TetrisNESDisasm/main.asm` (NES 6502 Assembly, Mapper 1 / MMC1)
- **Reference ROM:** `Tetris (USA).nes` (SHA-1: `3026d28b63d94c921fe58364f8b0659d10b5a0ac`)
- **Target Implementation:** Portable C11 (`src/`, `include/`), compiled with `-std=c11 -Wall -Wextra -Werror -pedantic`
- **Archived ROM Experiments:** Target-specific C-to-6502 ROM compilation experiments (`cc65`) are preserved on branch `archive/rom-experiments`.

---

## Executive Status Dashboard

| Metric | Status | Details |
| :--- | :---: | :--- |
| **Standalone Routines** | **171 / 171 (100%)** | All independent callable routines decompiled and verified |
| **Alternate Entry Points** | **111 / 111 (100%)** | Secondary entry points and shared fall-throughs mapped |
| **Authoritative Data Tables** | **260 / 260 (100%)** | Lookup, geometry, sprite, and audio tables in PRG ROM |
| **Total PRG Symbols Tracked** | **567 / 567 (100%)** | Complete symbol census of PRG ROM |
| **Unit Test Suites** | **13 / 13 passing** | Subsystem test suites covering game logic, math, audio, and video |
| **ROM Table Verifications** | **111 / 111 matched** | Byte-for-byte matching against reference ROM data tables |
| **6502 Differential Tests** | **90 / 90 passing** | Equivalent execution proven against NMOS 6502 CPU simulator |

---

## Subsystem Architecture & Status

All 11 functional subsystems are fully decompiled into portable C11 with corresponding headers and unit tests:

| Subsystem | Routines | C Implementation | Header | Test Suite | Status |
| :--- | :---: | :--- | :--- | :--- | :---: |
| **MATH_RNG** | 2 | `src/math/rng.c` | `include/math/rng.h` | `tests/test_rng.c` | **100%** |
| **MATH_BCD** | 12 | `src/math/bcd.c` | `include/math/bcd.h` | `tests/test_bcd.c` | **100%** |
| **PLAYER_INPUT** | 4 | `src/game/input.c` | `include/game/input.h` | `tests/test_input.c` | **100%** |
| **GAMEPLAY_STATE** | 14 | `src/game/state.c` | `include/game/state.h` | `tests/test_state.c` | **100%** |
| **TETRIMINO** | 16 | `src/game/tetrimino.c` | `include/game/tetrimino.h` | `tests/test_tetrimino.c` | **100%** |
| **PLAYFIELD** | 9 | `src/game/playfield.c` | `include/game/playfield.h` | `tests/test_playfield.c` | **100%** |
| **MENUS_UI** | 13 | `src/game/menu.c` | `include/game/menu.h` | `tests/test_menu.c` | **100%** |
| **AUDIO** | 41 | `src/game/audio.c` | `include/game/audio.h` | `tests/test_audio.c` | **100%** |
| **RENDER** | 16 | `src/game/render.c` | `include/game/render.h` | `tests/test_render.c` | **100%** |
| **CORE_LOGIC** | 38 | `src/game/system.c` | `include/game/system.h` | `tests/test_system.c` | **100%** |
| **SYSTEM** | 6 | `src/game/system.c` | `include/game/system.h` | `tests/test_system.c` | **100%** |
| **TOTAL** | **171** | — | — | — | **100%** |

*(Ending cutscene animations are implemented in `src/game/ending.c` and tested in `tests/test_ending.c`).*

---

## Verification Methodology

Verification is performed at multiple independent layers:

1. **Subsystem Unit Tests (`tests/test_*.c`)**: Validate algorithmic correctness, edge cases, BCD overflow, game state transitions, and memory invariants using standard C assertions.
2. **ROM Data Table Verification (`tests/verification/rom_data.c`)**: Validates 111 lookup tables (geometry, scoring, sound, palettes, PPU addresses) byte-for-byte against the original PRG ROM.
3. **Differential 6502 CPU Simulation (`tests/verification/test_differential.c`)**: Executes the original 6502 assembly routines on an embedded cycle-accurate NMOS 6502 CPU simulator and compares final machine state (registers, zero page, RAM, VRAM staging) against the C implementation.

When `Tetris (USA).nes` is not present, ROM-dependent tests output `SKIPPED: reference ROM not found` and exit cleanly, while unit tests continue to pass.

---

## Completed Milestones

| Milestone | Scope | Key Deliverables |
| :--- | :--- | :--- |
| **Batches 0–3** | Foundation & Core Mechanics | `NES_State`, RNG/LFSR, piece selection, BCD score math, tetrimino movement & rotation |
| **Verification Infra** | Differential Oracle | NMOS 6502 simulator, bit-exact ROM table extractor, initial differential tests |
| **Batches 4–7** | Gameplay Loop | Playfield row clearing, 2P garbage, joypad & DAS processing, 12-state player state machine |
| **Batches 8–10** | Menus & High Scores | Legal/title screens, menu flows, high score entry & sorting, soft reset detection |
| **Batches 11–13** | Rendering & Cutscenes | PPU row streaming, OAM sprite staging, level palettes, Type A/B ending cutscenes |
| **Batches 14–15** | Audio Subsystem | APU channel drivers, sound effect slots 0–3, music engine, sequence interpreter |
| **Batch 16** | System Boot & Mapper | Reset vector ($FF00), RAM clearing, MMC1 bankswitch registers, PPU bulk copying |
| **Simplification** | Minimal C Project | Removed experimental cc65 ROM tooling (archived to `archive/rom-experiments`), streamlined CMake & documentation |

---

## Detailed Routine Catalog

For a complete symbol-by-symbol directory with assembly addresses, line numbers, call mechanisms, and status notes, see [ROUTINE_INVENTORY.md](ROUTINE_INVENTORY.md) and [routine_inventory.json](routine_inventory.json).
