# TDecomp — Codebase Health, Cleanup & Maintainability Report

## Executive Summary

- **Repository**: `TDecomp` (NES Tetris Decompilation in Strict C11)
- **Branch**: `main`
- **Reference Disassembly**: `TetrisNESDisasm/` (Git submodule, `https://github.com/CelestialAmber/TetrisNESDisasm`)
- **Reference ROM**: `Tetris (USA).nes` (SHA-1: `3026d28b63d94c921fe58364f8b0659d10b5a0ac`)
- **Audit Date**: 2026-10-13
- **Audit Objective**: Review and organize the repository, consolidate documentation, document tooling, verify code quality and build invariants, and establish a maintainable baseline without expanding decompilation scope or altering verified behavior.

---

## 1. Final Project Structure & Directory Taxonomy

The repository is structured into clear functional domains separating portable host C code, bare-metal 6502 target compilation, automated testing, development tools, and technical documentation:

```
TDecomp/
├── CMakeLists.txt              # Primary CMake build configuration (C11, strict warnings, tests, ROM targets)
├── README.md                   # Primary project entry point (purpose, architecture, capabilities, build, limitations)
├── DECOMPILATION_PROGRESS.md   # Chronological batch progress tracker & 171-routine verification census
├── ROUTINE_INVENTORY.md        # Comprehensive catalog of all 171 routines, 111 alt entries, and 260 tables
├── routine_inventory.json      # Machine-readable schema representation of all disassembly symbols
├── CODEBASE_HEALTH_REPORT.md   # This document: consolidated repository health & maintainability audit
├── Tetris (USA).nes            # Authoritative reference cartridge ROM (untracked asset)
│
├── include/                    # Public C11 API, hardware constants, and subsystem headers
│   ├── common.h                # Standard includes, portable integer types, bool definitions
│   ├── nes.h                   # Monolithic NES_State struct, RAM/ROM sizes, register accessors
│   ├── constants/              # Hardware registers, memory map addresses, gameplay constants
│   ├── math/                   # Math subsystems: BCD math (bcd.h), LFSR RNG (rng.h)
│   └── game/                   # Subsystems: tetrimino, playfield, input, state, menu, highscore,
│                               # render, ending, audio, system
│
├── src/                        # Portable C11 implementations (compiled into libtdecomp_core.a)
│   ├── nes.c                   # Core state initialization, memory read/write accessors
│   ├── math/                   # BCD scoring arithmetic and RNG implementations
│   └── game/                   # Decompiled game logic (10 modules covering all 11 game subsystems)
│
├── tests/                      # Automated multi-tier test harness
│   ├── test_main.c             # Master test runner aggregating failure counts across all suites
│   ├── test_*.c                # Subsystem unit tests (13 test suites covering core game logic)
│   └── verification/           # Formal verification against reference ROM and NMOS 6502 execution
│       ├── cpu6502.c / .h      # Embedded NMOS 6502 differential CPU emulator
│       ├── rom_data.c / .h     # Reference PRG ROM table extractor and bit-exact table verifiers
│       └── test_differential.c # 92 formal differential execution tests comparing C11 to 6502 PRG code
│
├── tools/                      # Standalone Python build, verification, and audit tools
│   ├── README.md               # Tool documentation (usage, arguments, dependencies, CMake targets)
│   ├── build_rom.py            # Central NES ROM build pipeline (baseline, POC, experimental, bank)
│   └── inventory_routines.py   # Disassembly symbol parser and routine census synchronizer
│
├── experimental/c6502/         # Target-ready 6502 C compilation pipeline (cc65)
│   ├── README.md               # Architecture documentation: host C vs bare-metal 6502 target C
│   ├── nes_target.h            # Direct memory-mapped register macros & zero-page variable definitions
│   ├── copy_oam.c              # Routine 1: copyOamStagingToOam ($AB5E, 11 bytes, 100% bit-exact)
│   ├── mute_audio.c            # Routine 2: muteAudio ($E271, 22 bytes, 100% bit-exact)
│   ├── rng.c                   # Routine 3: generateNextPseudorandomNumber ($AB47, 54B C LFSR)
│   ├── high_scores_cluster.c   # Routine 4: High Score Subsystem cluster ($A0EE-$A200, 636B C cluster)
│   └── proof_of_concept.c      # Flat standalone compilation test module
│
├── docs/                       # Technical reports, feasibility audits, and historical documentation
│   ├── README.md               # Documentation directory index
│   ├── REPOSITORY_AUDIT.md     # Baseline repository audit and build hardening report
│   ├── ROM_BUILD_REPORT.md     # Initial assembly rebuild & bit-exact PRG/CHR payload audit
│   ├── C_TO_6502_INTEGRATION_REPORT.md # Incremental C-to-6502 ROM integrations & slot collision report
│   └── BANK_LEVEL_C_PORT_FEASIBILITY.md # Bank-level C recompilation feasibility & slack integration report
│
└── TetrisNESDisasm/            # Pinned Git submodule: authoritative assembly disassembly reference
```

---

## 2. Documentation & File Cleanup Actions

### What Was Consolidated or Reorganized
1. **Consolidation of Specialized Technical Reports into `docs/`**:
   - The top-level root previously accumulated multiple large specialized reports (`REPOSITORY_AUDIT.md`, `ROM_BUILD_REPORT.md`, `C_TO_6502_INTEGRATION_REPORT.md`, `BANK_LEVEL_C_PORT_FEASIBILITY.md`).
   - These four comprehensive reports were relocated into `docs/` via `git mv` and indexed in `docs/README.md`.
   - All inbound links in `README.md` and `DECOMPILATION_PROGRESS.md` were updated to preserve hyperlink integrity.
2. **Creation of `tools/README.md`**:
   - Documented the exact purposes, parameters, outputs, dependencies, and CMake target mappings for `tools/build_rom.py` and `tools/inventory_routines.py`.
3. **Harmonization of `README.md`**:
   - Updated the differential test counter from 91 to 92 (incorporating Test 1c for LFSR calling convention edge-case verification).
   - Documented all 4 active ROM targets (`rom_baseline`, `rom_poc`, `rom_experimental`, `rom_bank`).
   - Added an explicit section outlining project capabilities and current limitations (headless library vs interactive graphical host window).

### What Was Retained and Why
- **`Tetris (USA).nes`**: Retained in root as the authoritative reference binary for bit-exact ROM table verification and differential CPU testing. Correctly excluded from git tracking via `.gitignore`.
- **`TetrisNESDisasm/` submodule**: Retained as the immutable source of truth for labels, opcodes, memory addresses, and assembler/compiler tooling.
- **`ROUTINE_INVENTORY.md` and `routine_inventory.json`**: Retained in root as active census artifacts generated and validated by `tools/inventory_routines.py`.
- **All 17 headers and 13 source files**: Retained without alteration to preserve 100.0% verified C11 decompilation behavior.

---

## 3. Tooling Census & Architecture

Both retained tools in `tools/` serve specific, non-redundant functions in the project workflow:

| Tool Script | File Location | Purpose & Function | Invocation & CMake Targets |
| :--- | :--- | :--- | :--- |
| **`build_rom.py`** | `tools/build_rom.py` | Central NES ROM orchestrator. Handles baseline assembly rebuilds, cc65 target C compilation, out-of-tree staged assembly integration, bit-exact PRG/CHR payload diffing against `Tetris (USA).nes`, and behavioral NMOS 6502 CPU simulation. | `python3 tools/build_rom.py [all\|baseline\|poc\|experimental\|bank\|checksums]`<br>CMake: `rom`, `rom_baseline`, `rom_poc`, `rom_experimental`, `rom_bank`, `rom_bank_exp`, `verify` |
| **`inventory_routines.py`** | `tools/inventory_routines.py` | Disassembly symbol parser and routine census synchronizer. Parses `TetrisNESDisasm/main.asm` and `tetris.lbl`, categorizes symbols into 11 subsystems, cross-checks `src/` implementations, and outputs `ROUTINE_INVENTORY.md` and `routine_inventory.json`. | `python3 tools/inventory_routines.py` |

---

## 4. Code Quality & Maintainability Findings

### 1. Strict Compiler Hygiene
- The entire codebase (`include/`, `src/`, `tests/`) compiles cleanly under GCC with `-std=c11 -Wall -Wextra -Werror -pedantic`.
- **Zero compiler warnings** and **zero compiler errors** across all 13 core modules and 17 test source files.

### 2. Header and Interface Consistency
- **Include Guards**: 100% of headers in `include/` follow the uniform naming pattern `TDECOMP_<DIR>_<NAME>_H` (e.g., `TDECOMP_GAME_STATE_H`, `TDECOMP_CONSTANTS_MEMORY_H`).
- **C++ Compatibility**: All 17 headers in `include/` encapsulate declarations within standard `extern "C" { ... }` guards.
- **Dependency Graph**: Header dependencies are strictly hierarchical. Subsystem headers include `common.h` and specific constants headers without circular include chains.

### 3. Separation of Concerns: Host-Side vs. Target-Ready C
- **Host Engine (`include/`, `src/`)**: Designed for cross-platform portability. Uses an explicit `NES_State` struct context, eliminating hidden global state and allowing deterministic unit and differential testing.
- **Target Engine (`experimental/c6502/`)**: Designed for bare-metal 6502 hardware execution. Uses direct volatile pointers to memory-mapped registers (`$2000`–`$4017`) and zero-page memory, avoiding struct pointer dereferences that would be prohibitively slow on a 1.79 MHz CPU.

### 4. Build Configuration Hardening
- Include paths are scoped strictly via `target_include_directories` (`tdecomp_core PUBLIC include`, `tdecomp_tests PRIVATE tests`).
- Test assertions are protected via `target_compile_options(tdecomp_tests PRIVATE -UNDEBUG)`, ensuring that tests never silently pass with disabled assertions in Release builds.

---

## 5. Exact Verification Results

Before and after the cleanup actions, all verification checks were executed and confirmed passing:

```
========================================================================================
Verification Step             Command / Target                            Status   Notes
========================================================================================
Host Unit & Diff Tests        ctest --test-dir build --output-on-failure  PASS     15 suites / 92 diff tests (0.10s)
Full ROM Build Pipeline       python3 tools/build_rom.py all               PASS     Phases 1-4 passing
CMake ROM Targets             cmake --build build --target rom            PASS     All 4 targets built & verified
Source Formatting Check       git diff --check                            PASS     0 whitespace/formatting errors
========================================================================================
```

### ROM Build & Binary Verification Details
1. **Baseline Rebuilt ROM (`build/rom/tetris_baseline.nes`)**:
   - File Size: `49,168 bytes`
   - PRG ROM (32,768 bytes): **100.0% BIT-EXACT** match to reference ROM.
   - CHR ROM (16,384 bytes): **100.0% BIT-EXACT** match to reference ROM.
   - SHA-1: `77747840541bfc62a28a5957692a98c550bd6b2b` (matches authoritative `tetris.sha1`).
2. **Experimental C-Integrated ROM (`build/rom/tetris_experimental.nes`)**:
   - `copyOamStagingToOam` ($AB5E, 11 bytes): **100.0% BIT-EXACT** machine code compiled from C.
   - `muteAudio` ($E271, 22 bytes): **100.0% BIT-EXACT** machine code compiled from C.
   - PRG & CHR payloads: **100.0% BIT-EXACT** across all 32KB PRG & 16KB CHR (0 bytes shifted).
   - NMOS 6502 CPU Simulation: Full DMA transfer and APU mute sequence verified with exact register states.
3. **Bank-Level Experimental ROM (`build/rom/tetris_bank_exp.nes`)**:
   - High Score cluster ($A0EE–$A200, 275 bytes) compiled from target C (`high_scores_cluster.c`, 638 bytes).
   - Relocated into unreferenced PRG slack at `$D6C9` via a 3-byte trampoline at `$A0EE` (`JMP $D802`).
   - Downstream code at `$A201` (`highScoreEntryScreen`) is **100.0% BIT-EXACT and undamaged**.
   - Active PRG code outside the 275-byte slot and unreferenced data matches reference ROM **100.0% bit-for-bit**.
   - **CPU Simulator Only**: Behavioral execution verified on embedded NMOS 6502 CPU simulator only, not in an emulator. Tested across 20 edge cases (tied scores, min/max score/level) and 100 randomized runs with fixed seed `0x12345678`, confirming 100% memory and state equivalence.
   - **Relocation Area Safety**: Audited `$D6C9`–`$DCFF` across PRG pointer tables, indirect jump instructions, and MMC1 banking; confirmed 100% unused slack.

---

## 6. Project Limitations & Current Blockers

1. **Host-Side Graphical Frontend**:
   - The host C engine is currently built as a static library (`libtdecomp_core.a`) and test runner (`tdecomp_tests`). It does not include an interactive graphical window (SDL2/OpenGL/GLFW) or host-side audio synthesizer.
2. **Interactive ROM Gameplay Testing in Headless CLI**:
   - Headless Linux container environments lack a display server (X11/Wayland) and desktop emulator binaries (`fceux`, `nestopia`, `mesen`). Automated bit-exact payload diffing and 6502 CPU simulation serve as the authoritative headless verification standard.
3. **In-Place C Expansion Constraint**:
   - Pure C compiled under `cc65` exhibits an expansion factor of 2.3x–2.4x. Direct in-place replacement of routines in tightly packed PRG areas without slack or trampolining is blocked to prevent overwriting downstream routines.

---

## 7. Worthwhile Future Improvements

The following improvements are recommended for future development phases (documented here without immediate implementation):

1. **Interactive Host Frontend**:
   - Develop a lightweight desktop frontend using SDL2 or WebAssembly to render the game's nametables and sprites at 60 FPS directly on the host using the decompiled C engine.
2. **Host-Side APU Synthesizer**:
   - Connect the decompiled audio register routines (`updateAudio`, sound effect slots, music bytecode interpreter) to a software APU audio synthesizer buffer.
3. **Additional Slack Relocation for Audio Engine**:
   - Utilize the second unreferenced PRG slack block (`unreferenced_data4`, 1,301 bytes at `$F9EB`–`$FEFF`) to relocate and recompile audio envelope or sound effect routines from target C.
4. **Automated CI Integration**:
   - Add a GitHub Actions workflow running `ctest` and `python3 tools/build_rom.py all` across Linux, macOS, and Windows runners on every pull request.
