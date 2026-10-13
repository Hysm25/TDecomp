# TDecomp — Repository Audit, Cleanup & Quality Assurance Report

## Executive Summary

- **Repository**: `TDecomp` (NES Tetris Decompilation in Strict C11)
- **Reference Disassembly**: `TetrisNESDisasm/` (Git submodule, `https://github.com/CelestialAmber/TetrisNESDisasm`)
- **Reference ROM**: `Tetris (USA).nes` (SHA-1: `3026d28b63d94c921fe58364f8b0659d10b5a0ac`)
- **Audit Date**: 2026-10-13
- **Branch**: `main` (tracked to `origin/main`)

---

## Audit Checklist & Status

- [x] **Phase 1: Baseline Establishment & Inventory Census**
  - [x] Read `DECOMPILATION_PROGRESS.md`, `routine_inventory.json`, `ROUTINE_INVENTORY.md`, `README.md`, `CMakeLists.txt`.
  - [x] Inspect Git history, remote tracking, working tree state.
  - [x] Baseline build and test execution confirmed passing.
- [x] **Phase 2: File & Directory Classification**
  - [x] Inspected every tracked file and relevant untracked/ignored file.
  - [x] Determined usage, dependencies, and necessity of each file.
  - [x] Confirmed zero obsolete or unreferenced tracked files.
- [x] **Phase 3: Source Code & Header Quality Review**
  - [x] Reviewed all header files in `include/` and `tests/` for include guards, dependencies, portability.
  - [x] Reviewed all source implementations in `src/` for unused variables, sign issues, undefined behavior risks, portability.
  - [x] Removed machine-specific `/root/TDecomp` paths and added `TDECOMP_ROM_PATH` environment variable support.
  - [x] Fixed potential file descriptor leak in `tests/verification/rom_data.c`.
- [x] **Phase 4: Build System & Tooling Review**
  - [x] Removed compiler warning suppression flag (`-Wno-unused-parameter`).
  - [x] Updated `CMakeLists.txt` to modern target-based include directories (`target_include_directories`).
  - [x] Guaranteed active test assertions regardless of build mode (`target_compile_options(tdecomp_tests PRIVATE -UNDEBUG)`).
  - [x] Validated fresh build directory configuration with strict `-std=c11 -Wall -Wextra -Werror -pedantic`.
  - [x] Updated `.gitignore` to match all build folders (`build*/`) and `compile_commands.json`.
- [x] **Phase 5: Test Integrity & Verification Robustness**
  - [x] Audited all 15 test suites and test runner.
  - [x] Standardized all test functions to return `int failures` (added `test_ending_subsystem`).
  - [x] Verified bit-exact ROM table coverage (111 PRG tables) and 6502 differential CPU oracle tests (91 tests).
  - [x] Confirmed no weakened assertions or dummy tests exist.
- [x] **Phase 6: Documentation & Inventory Reconciliation**
  - [x] Reconciled `README.md` from stale Batch 9 status to authoritative 100.0% completion.
  - [x] Reconciled `ROUTINE_INVENTORY.md` and `DECOMPILATION_PROGRESS.md` metrics.
  - [x] Verified `tools/inventory_routines.py` produces consistent output matching codebase.
- [x] **Phase 7: Clean-Build Validation & Final Verification**
  - [x] Executed clean CMake build in fresh directory (`build_fresh`).
  - [x] Strict C11 compilation with zero warnings and zero errors.
  - [x] Full CTest suite and direct test binary execution (100% passing).
  - [x] Verified `git diff --check` passes cleanly with zero whitespace errors.

---

## 1. Repository Structure & Overall Findings

The repository consists of 58 tracked files organized into logical directories:
- `include/`: 17 header files (`common.h`, `nes.h`, 3 constants headers, 12 subsystem headers).
- `src/`: 13 C implementation files covering all game subsystems and core NES hardware abstraction (`libtdecomp_core.a`).
- `tests/`: 14 subsystem unit test files, 2 verification harnesses (`cpu6502.c`, `rom_data.c`), 1 differential runner (`test_differential.c`), and 1 master runner (`test_main.c`).
- `tools/`: Python routine census and inventory synchronization tool (`inventory_routines.py`).
- Root configuration: `CMakeLists.txt`, `.gitignore`, `.gitmodules`.
- Root documentation: `README.md`, `DECOMPILATION_PROGRESS.md`, `ROUTINE_INVENTORY.md`, `routine_inventory.json`, `REPOSITORY_AUDIT.md`.
- Submodule: `TetrisNESDisasm` (pinned reference disassembly).
- Ignored assets: `Tetris (USA).nes` (reference ROM, verified bit-exact, correctly excluded from git tracking to adhere to copyright hygiene).

---

## 2. Files Removed, Retained, or Consolidated

- **Total Tracked Files Audited**: 58
- **Files Removed**: 0
  - Rationale: Every tracked file is actively referenced by `CMakeLists.txt`, C include directives, or project verification tooling.
- **Files Retained**:
  - `TetrisNESDisasm/`: Upstream reference assembly submodule; essential for verifying disassembly labels, opcodes, and memory addresses.
  - `tools/inventory_routines.py`: Authoritative inventory audit script; essential for synchronizing `ROUTINE_INVENTORY.md` and `routine_inventory.json`.
  - All 17 header and 13 source files: Required for complete, bit-exact C11 decompilation.
  - All test files: Required for multi-level verification (unit tests, ROM table matching, 6502 differential CPU tests).
- **`.gitignore` Updates**:
  - Expanded `build/` to `build*/` to automatically ignore any temporary clean build directories (e.g., `build_fresh`, `build_test`).
  - Added `compile_commands.json` generated by CMake IDE integration.

---

## 3. Source Code & Build-System Issues Fixed

1. **Hardcoded Machine Paths in `tests/verification/rom_data.c`**:
   - Replaced machine-specific `/root/TDecomp/...` paths with relative search paths (`../../../...`) and added `TDECOMP_ROM_PATH` environment variable support.
2. **File Descriptor Leak in `rom_data.c`**:
   - Added `fclose(f)` in the allocation failure fallback branches of `load_reference_rom` to prevent file descriptor leaks.
3. **Compiler Flag Suppression in `CMakeLists.txt`**:
   - Removed `-Wno-unused-parameter` from `CMAKE_C_FLAGS`. The entire codebase now builds cleanly under pure `-Wall -Wextra -Werror -pedantic`.
4. **Modern Target-Based Include Directories**:
   - Converted global `include_directories(...)` to `target_include_directories(tdecomp_core PUBLIC ...)` and `target_include_directories(tdecomp_tests PRIVATE ...)`.
5. **Assertion Preservation Across Build Configurations**:
   - Added `target_compile_options(tdecomp_tests PRIVATE -UNDEBUG)` to prevent test assertions from being disabled if built with `-DCMAKE_BUILD_TYPE=Release` or `RelWithDebInfo`.
6. **Unused-Variable Warning Guard in `tests/test_highscore.c`**:
   - Added explicit `(void)done;` annotations after assertions on `highScoreEntryScreen_step`.
7. **Uniform Test Failure Accumulation**:
   - Added `test_ending_subsystem(void)` in `tests/test_ending.c` returning `int failures`, standardizing all 15 test suites to return failure counts in `tests/test_main.c`.
8. **Whitespace and Formatting Integrity**:
   - Removed trailing blank line at EOF in `tests/test_ending.c`, satisfying `git diff --check`.

---

## 4. Documentation & Inventory Inconsistencies Corrected

- **`README.md` Reconciled**:
  - Previously reported stale Batch 9 figures (46/171 routines, 26.90%, 10 test suites, 20 ROM tables, 28 differential tests).
  - Updated to reflect actual 100.0% verified completion: 171/171 standalone routines, 111/111 alternate entry points, 567 PRG symbols, 15/15 test suites, 111 ROM tables, 91 differential tests, and 100.0% across all 11 subsystems.
- **Authoritative Inventory Consistency**:
  - Validated that `routine_inventory.json` and `ROUTINE_INVENTORY.md` are in complete agreement with `DECOMPILATION_PROGRESS.md` and codebase implementations.

---

## 5. Tests Executed & Actual Results

### Test Execution Summary

| Test Category | Suite Count / Tests | Result | Coverage |
|:---|:---:|:---:|:---|
| **Subsystem Unit Tests** | 13 suites / 60+ cases | **100% PASS** | State machines, RNG, BCD, DAS, collision, audio, rendering |
| **Authoritative ROM Tables** | 1 suite / 111 tables | **100% PASS** | Bit-for-bit match against `Tetris (USA).nes` PRG ROM |
| **6502 Differential CPU Oracle** | 1 suite / 91 tests | **100% PASS** | Equivalence proven against 6502 machine code execution |
| **CTest Integration** | 1 target / all tests | **100% PASS** | `ctest --output-on-failure` passes in 0.11s |

---

## 6. Clean-Build Results

- **Compiler**: GCC 16.2.1 (Linux x86_64)
- **Language Standard**: Strict C11 (`-std=c11`)
- **Flags**: `-Wall -Wextra -Werror -pedantic`
- **Warnings**: 0
- **Errors**: 0
- **Build Configurations Tested**:
  - `Debug`: Clean build, 100% tests pass.
  - Fresh directory build (`build_fresh`): Clean build, 100% tests pass.

---

## 7. Remaining Warnings, Risks & Blockers

- **Warnings**: 0 warnings across all libraries and tests.
- **Risks**: None identified for current decompilation scope.
- **Blockers**: None.
- **Known Design Notes**:
  - `checkSelectHeldToAddPoints` ($A41C): Conditionally compiled in PAL disassembly (`.if PAL = 1`), absent in NTSC PRG ROM. Retained and verified as a documented PAL routine.
  - Game Mode 5: Retains dual naming (`MODE_PLAY_ENDING_HIGH_SCORE` / `MODE_DEMO`) to mirror original assembly multiplexing.

---

## 8. Issues Deliberately Left Unchanged

1. **Submodule `TetrisNESDisasm`**: Retained in full to preserve the verifiable audit trail against the original 6502 assembly and symbols.
2. **Global RAM Layout in `src/game/*.c`**: NES game logic operates on flat memory-mapped zero-page and RAM offsets ($0000–$07FF). The C code mirrors this memory model via `nes->ram[...]` to guarantee bit-exact behavioral equivalence.
3. **Reference ROM Exclusion**: Copyrighted ROM files remain strictly uncommitted, while the build and test system gracefully discovers local reference copies via relative paths or `TDECOMP_ROM_PATH`.
