# TDecomp - NES Tetris Decompilation

A clean, verified, and modular C decompilation of original NES Tetris (USA), matching the original behavior bit-for-bit while remaining portable to Linux, Windows, macOS, and Android.

## Source of Truth
- **Disassembly Repository:** [TetrisNESDisasm](https://github.com/CelestialAmber/TetrisNESDisasm) (included as submodule `TetrisNESDisasm/`)
- **Reference ROM:** `Tetris (USA).nes` (SHA-1: `3026d28b63d94c921fe58364f8b0659d10b5a0ac`, NROM/MMC1 Mapper 1, 32KB PRG, 16KB CHR)
- **Built Disassembly Verification:** Verified with SHA-1 `77747840541bfc62a28a5957692a98c550bd6b2b`

## Architecture & Principles
1. **Source of Truth Integrity:** The original 6502 assembly is treated as the absolute source of truth. No assumptions or modern redesigns are made without matching original behaviors.
2. **Modularity:** Game subsystems (Math/RNG, BCD, Tetrimino Geometry, Playfield, Controls/DAS, Game States, Audio, Video) are kept cleanly separated in `include/` and `src/`.
3. **Portability:** Written in standard C11 without compiler-specific extensions, supporting desktop systems and mobile platforms.
4. **Behavioral Verification:** Every decompiled routine is accompanied by unit tests and state verification against known disassembly execution paths.

## Project Structure
- `include/`: Header files defining the NES state, memory addresses, constants, and subsystem APIs.
- `src/`: Decompiled C source implementations.
- `tests/`: Automated unit tests and behavioral verification suites.
- `tools/`: Scoping and routine auditing tools (`inventory_routines.py`).
- `TetrisNESDisasm/`: Reference assembly disassembly submodule.
- `DECOMPILATION_PROGRESS.md`: Live tracking of decompilation status across all routines.
- `ROUTINE_INVENTORY.md`: Comprehensive census of all callable routines and data tables.

## Current Project Status
- **Standalone Routines Verified:** 171 / 171 (100.0%)
- **Alternate Entry Points Verified:** 111 / 111 (100.0%)
- **Total PRG Symbols Tracked:** 567
- **Test Suites:** 15 / 15 passing (unit tests across all decompiled subsystems)
- **Authoritative ROM Data Tables:** 111 tables verified bit-exact against reference ROM
- **6502 Differential CPU Oracle:** 91 differential tests proving decompiled C routines equivalent to original 6502 PRG ROM execution
- **Subsystem Progress:**
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

See [DECOMPILATION_PROGRESS.md](DECOMPILATION_PROGRESS.md) for full progress details and roadmap.

## Building and Running Tests
### Prerequisites
- CMake 3.16+
- C11 compliant compiler (`gcc` or `clang`)
- Make / Ninja

### Build
```bash
cmake -B build
cmake --build build
```
Or with traditional directory creation:
```bash
mkdir -p build && cd build
cmake ..
cmake --build .
```

### Run Tests
```bash
ctest --test-dir build --output-on-failure
# or directly execute the test runner binary:
./build/tdecomp_tests
```

### Build NES ROM & Verified C Integration
To build the authoritative baseline assembly ROM, verify the C-to-6502 proof-of-concept, and produce the experimental ROM with compiled C routines:
```bash
# Full ROM pipeline (Baseline assembly + C POC + C-integrated experimental ROM)
cmake --build build --target rom
# or directly:
python3 tools/build_rom.py all

# Specific targets:
cmake --build build --target rom_baseline      # Rebuilds tetris_baseline.nes from assembly
cmake --build build --target rom_poc           # Compiles target-ready C routines with cc65
cmake --build build --target rom_experimental  # Builds tetris_experimental.nes with C routine integrated
```
See [ROM_BUILD_REPORT.md](ROM_BUILD_REPORT.md) and [C_TO_6502_INTEGRATION_REPORT.md](C_TO_6502_INTEGRATION_REPORT.md) for full architecture, linker configurations, and verification details.
