# TDecomp - NES Tetris Decompilation

A modular C decompilation and 6502 reverse-engineering project for NES Tetris (USA), maintaining a portable host C reference library alongside experimental bare-metal C-to-6502 ROM integrations.

---

## Two Distinct Decompilation Tracks

The repository maintains two strictly separated tracks:

1. **Host C Reference Port (`src/`, `include/`)**:
   - A modular C11 library (`libtdecomp_core.a`) modeling the complete NES Tetris state (`NES_State*`) and game subsystems (RNG, BCD math, playfield, piece logic, state machines, menu navigation, audio sequencing, and PPU rendering).
   - Designed to run and be tested on the host development machine (tested on Linux x86_64).
   - Verified via standalone C11 unit tests and differential execution against an embedded NMOS 6502 CPU simulator.

2. **Target C Integrated into the NES ROM (`experimental/c6502/`)**:
   - Bare-metal, zero-page-optimized C code compiled via `cc65` to native 6502 machine code.
   - Replaces original assembly routines in place (`tetris_experimental.nes`) or relocates expanded subsystems into unreferenced cartridge slack (`tetris_bank_exp.nes`).
   - Verified via bit-exact machine code diffing and embedded NMOS 6502 CPU simulation.

---

## Source of Truth & ROM Checksums

- **Disassembly Repository:** [TetrisNESDisasm](https://github.com/CelestialAmber/TetrisNESDisasm) (included as submodule `TetrisNESDisasm/`)
- **Reference ROM:** `Tetris (USA).nes` (SHA-1: `3026d28b63d94c921fe58364f8b0659d10b5a0ac`, Mapper 1 / MMC1, 32KB PRG, 16KB CHR)
- **Authoritative Rebuilt Disassembly:** `build/rom/tetris_baseline.nes` (SHA-1: `77747840541bfc62a28a5957692a98c550bd6b2b`, matching `TetrisNESDisasm/tetris.sha1`)

### Explanation of the Two SHA-1 Checksums

Both ROM files contain **100% bit-exact PRG ROM (32,768 bytes) and CHR ROM (16,384 bytes) payloads** (0 byte differences across the entire 49,152-byte payload). The file-level SHA-1 hash mismatch is confined entirely to 3 bytes in the 16-byte iNES container header:

| Offset | Reference ROM (`Tetris (USA).nes`) | Built Assembly ROM (`tetris_baseline.nes`) | Explanation |
|:---|:---:|:---:|:---|
| `0x07` (Flags 7) | `0x08` | `0x00` | Reference dump has bit 3 set (often present from older dumping utilities or NES 2.0 format flags); disassembly uses a standard clean iNES 1.0 template with upper mapper nibble zeroed. |
| `0x08` (Flags 8 / PRG-RAM) | `0x50` | `0x00` | Reference dump has legacy 8KB RAM flag; NES Tetris uses cartridge board NES-SIROM which contains no PRG-RAM or battery-backed SRAM. |
| `0x0F` (Padding) | `0x01` | `0x00` | Residual non-zero padding byte from ROM dumper; disassembly properly zeroes all reserved padding bytes. |

---

## Architecture & Principles

1. **Source of Truth Integrity:** The original 6502 assembly is treated as the absolute source of truth. No assumptions or modern redesigns are made without matching original behaviors.
2. **Subsystem Modularity:** Game subsystems (Math/RNG, BCD, Tetrimino Geometry, Playfield, Controls/DAS, Game States, Audio, Video, System Boot) are cleanly separated in `include/` and `src/`.
3. **Portability:** Written in standard C11 without compiler-specific extensions; tested on Linux (x86_64).
4. **Multi-Tier Verification:** Every routine is verified via unit tests, ROM lookup table matching (111 tables), and formal differential execution against an integrated NMOS 6502 CPU simulator.
5. **Bare-Metal 6502 Reintegration:** Incremental replacement of assembly routines with object code compiled from target C via `cc65`.

---

## Project Structure

- `include/`: Header files defining the NES state, memory addresses, constants, and subsystem APIs.
- `src/`: Portable C11 reference library implementations (`libtdecomp_core.a`).
- `tests/`: Automated unit tests, bit-exact ROM table verifiers, and 6502 differential CPU tests.
- `tools/`: ROM build pipeline and routine census tools (see [tools/README.md](tools/README.md)).
- `experimental/`: Target-ready 6502 C implementations for cc65 ROM compilation (see [experimental/c6502/README.md](experimental/c6502/README.md)).
- `docs/`: Technical reports, feasibility audits, and integration documentation (see [docs/README.md](docs/README.md)).
- `TetrisNESDisasm/`: Reference assembly disassembly submodule.
- `DECOMPILATION_PROGRESS.md`: Live tracking of decompilation status across all routines.
- `ROUTINE_INVENTORY.md`: Comprehensive census of all callable routines and data tables.
- `CODEBASE_HEALTH_REPORT.md`: Comprehensive repository health, cleanup, and maintainability audit.
- `LICENSE`: MIT License and third-party attribution.

---

## Current Project Status

- **Standalone Routines Verified:** 171 / 171 (100.0%)
- **Alternate Entry Points Verified:** 111 / 111 (100.0%)
- **Total PRG Symbols Tracked:** 567
- **Test Suites:** 15 / 15 passing (unit tests across all decompiled subsystems)
- **Authoritative ROM Data Tables:** 111 tables verified bit-exact against reference ROM
- **6502 Differential CPU Oracle:** 92+ differential tests proving decompiled C routines equivalent to original 6502 PRG ROM execution
- **Subsystem Breakdown:**
  - `MATH_RNG` (Pseudo-Random Generation): **100.0%** (2 / 2)
  - `MATH_BCD` (Score & Line Math): **100.0%** (12 / 12)
  - `PLAYER_INPUT` (DAS, Joypad, Autorepeat): **100.0%** (4 / 4)
  - `GAMEPLAY_STATE` (State Machine, Timers): **100.0%** (14 / 14)
  - `TETRIMINO` (Geometry, Movement, Spawns): **100.0%** (16 / 16)
  - `PLAYFIELD` (Grid, Line Clears, Garbage): **100.0%** (9 / 9)
  - `MENUS_UI` (Legal, Title, Menus, High Scores): **100.0%** (13 / 13)
  - `AUDIO` (Sound Effects, Music Engine, APU Drivers): **100.0%** (41 / 41)
  - `RENDER` (PPU, OAM, Palettes, Nametables): **100.0%** (16 / 16)
  - `CORE_LOGIC` (Context Switch, Dispatchers, Buffers): **100.0%** (38 / 38)
  - `SYSTEM` (Reset, NMI, IRQ, VBlank, MMC1 Mapper): **100.0%** (6 / 6)

See [DECOMPILATION_PROGRESS.md](DECOMPILATION_PROGRESS.md) for full progress details.

---

## Capabilities & Limitations

### What TDecomp Provides
1. **Portable C11 Reference Library**: Algorithmic implementation of all NES Tetris routines, state machines, math, rendering routines, sound generators, and boot sequences.
2. **Multi-Level Verification**: Strict C11 unit testing, bit-exact ROM table matching (111 tables), and an embedded NMOS 6502 CPU simulator verifying behavior against original PRG ROM opcodes.
3. **Reproducible NES ROM Rebuilds**: Standalone tooling to rebuild `tetris_baseline.nes` matching the reference cartridge payload bit-for-bit.
4. **Verified C-to-6502 ROM Integrations**: Standalone experimental ROMs (`tetris_experimental.nes`, `tetris_bank_exp.nes`) integrating object code compiled from target C (`cc65`) into active NES ROM images with zero code shifting and validated 6502 CPU behavior.

### Current Limitations
1. **Host-Side Graphical Frontend**: The host C11 engine is compiled as a headless library (`libtdecomp_core.a`) and verification test runner (`tdecomp_tests`). It does not currently embed an interactive window (e.g. SDL2/OpenGL) or host-side audio synthesizer.
2. **Interactive Gameplay Testing**: Interactive play of generated ROMs (`tetris_baseline.nes`, `tetris_experimental.nes`, `tetris_bank_exp.nes`) requires physical NES hardware (flash cart) or desktop NES emulators (`fceux`, `nestopia`, `mesen`). In headless CLI environments without display servers, automated payload diffing and 6502 CPU simulation serve as the verification standard.
3. **Simulation vs. Emulator Execution**: The relocatable bank experiment (`tetris_bank_exp.nes`) was verified on an embedded NMOS 6502 CPU simulator only, not in an emulator or on hardware.

---

## Building and Running Tests

### Prerequisites
- CMake 3.16+
- C11 compliant compiler (`gcc` or `clang`)
- Make / Ninja
- Python 3.8+ (for ROM build and auditing tools)

### Submodule Initialization
When cloning for the first time, clone recursively or initialize submodules:

```bash
# Clone with submodules:
git clone --recursive https://github.com/Hysm25/TDecomp.git

# Or if already cloned:
git submodule update --init --recursive
```

### Build
```bash
cmake -B build
cmake --build build
```

### Run Tests
```bash
ctest --test-dir build --output-on-failure
# Or directly execute the test runner binary:
./build/tdecomp_tests
```

> [!NOTE]
> **Missing Reference ROM Handling**: Tests that require the commercial reference ROM (`Tetris (USA).nes`) will automatically print `SKIPPED: reference ROM not found` and exit successfully (0 failures) when the ROM file is not present. Standalone unit tests continue to execute and validate full game logic without external ROM dependencies.

---

## NES ROM Build & C-to-6502 Integration Pipeline

To build the baseline assembly ROM, verify the C-to-6502 proof-of-concept, and produce the experimental ROMs with compiled C routines:

```bash
# Full verification: builds all ROMs, runs ctest, and prints SHA-1 checksum table:
cmake --build build --target verify

# Full ROM pipeline (Baseline assembly + C POC + Experimental C ROM + Bank-level C ROM):
cmake --build build --target rom
# Or directly via Python:
python3 tools/build_rom.py all

# Specific targets:
cmake --build build --target rom_baseline      # Rebuilds tetris_baseline.nes from assembly
cmake --build build --target rom_poc           # Compiles target-ready C routines with cc65
cmake --build build --target rom_experimental  # Builds tetris_experimental.nes with C routines integrated
cmake --build build --target rom_bank          # Builds tetris_bank_exp.nes with High Score C cluster
cmake --build build --target rom_bank_exp      # Alias for rom_bank
```

### Documentation & Reports
- [CODEBASE_HEALTH_REPORT.md](CODEBASE_HEALTH_REPORT.md): Comprehensive repository health and maintainability audit.
- [docs/BANK_LEVEL_C_PORT_FEASIBILITY.md](docs/BANK_LEVEL_C_PORT_FEASIBILITY.md): Bank-level C recompilation feasibility audit, code expansion metrics, slack integration architecture, and cycle benchmarks.
- [docs/C_TO_6502_INTEGRATION_REPORT.md](docs/C_TO_6502_INTEGRATION_REPORT.md): Routine-level C-to-6502 integration and in-place slot constraints.
- [docs/ROM_BUILD_REPORT.md](docs/ROM_BUILD_REPORT.md): Out-of-tree ROM build pipeline and reference cartridge payload verification.
- [docs/REPOSITORY_AUDIT.md](docs/REPOSITORY_AUDIT.md): Initial repository audit and build configuration hardening report.
- [tools/README.md](tools/README.md): Detailed documentation for `build_rom.py` and `inventory_routines.py`.
- [experimental/c6502/README.md](experimental/c6502/README.md): Bare-metal 6502 compilation model, memory layouts, and target C implementations.
