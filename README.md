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
- **Standalone Routines Verified:** 46 / 171 (26.90%)
- **Test Suites:** 10 / 10 passing (unit tests across all decompiled subsystems)
- **Authoritative Data Tables:** 20 ROM data tables verified bit-exact against reference ROM
- **6502 Differential CPU Oracle:** 28 differential test blocks proving decompiled C routines equivalent to original 6502 PRG ROM machine code execution
- **Subsystem Progress:**
  - `MATH_RNG` (Pseudo-Random Generation): **100.0%** (2 / 2)
  - `PLAYFIELD` (Grid, Line Clears, Garbage): **77.8%** (7 / 9)
  - `PLAYER_INPUT` (DAS, Joypad, Autorepeat): **75.0%** (3 / 4)
  - `TETRIMINO` (Geometry, Movement, Spawns): **68.8%** (11 / 16)
  - `MENUS_UI` (Legal, Title, Menus): **46.2%** (6 / 13)
  - `GAMEPLAY_STATE` (State Machine, Framing): **42.9%** (6 / 14)
  - `CORE_LOGIC` (Context Switch, Dispatchers): **21.1%** (8 / 38)
  - `MATH_BCD` (Score & Line Math): **8.3%** (1 / 12)

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
