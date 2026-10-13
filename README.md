# TDecomp - NES Tetris C Decompilation

TDecomp is a clean, modular C decompilation of NES Tetris (USA), providing a portable C11 library (`libtdecomp_core.a`) alongside a comprehensive test suite. The implementation models the full NES Tetris game state and subsystems (RNG, BCD math, tetrimino movement and rotation, playfield logic, player input, game state machines, menu flows, high scores, audio sequencer, and PPU rendering).

---

## Prerequisites

- **CMake**: version 3.16 or higher
- **C Compiler**: C11 compliant compiler (`gcc` or `clang`)
- **Git**: with submodule support
- **Make** or **Ninja** build system
- **Python**: version 3.8+ (optional, for routine census tooling)
- **Emscripten** (`emcc`): version 3.0+ (optional, for WebAssembly compilation)

---

## Cloning and Submodules

Initialize the reference disassembly submodule when cloning:

```bash
git clone --recursive https://github.com/Hysm25/TDecomp.git
```

If already cloned without submodules:

```bash
git submodule update --init --recursive
```

---

## Building and Testing

### Build

```bash
cmake -B build
cmake --build build
```

### Run Tests

```bash
ctest --test-dir build --output-on-failure
```

Or execute the test binary directly:

```bash
./build/tdecomp_tests
```

A convenience CMake target is also provided:

```bash
cmake --build build --target verify
```

---

## WebAssembly Build & Browser Demo

TDecomp includes a WebAssembly port allowing the full C engine to run at 60 FPS directly in web browsers.

### Compile WebAssembly

```bash
make -C web
```

Compiles `web/tdecomp.js` and `web/tdecomp.wasm` using strict C11 (`-O2 -std=c11 -Wall -Wextra -Werror -pedantic`).

### Run Test Suite (Node.js)

```bash
node web/test_wasm.js
```

### Launch Browser Demo

Start a local HTTP server from the project root:

```bash
python3 -m http.server 8080 --directory web
```

Open `http://localhost:8080` in your web browser. The demo features:
- Authentic NES Tetris piece mechanics, rotation tables, gravity timings, and line clear logic.
- Type A (marathon) and Type B (25-line goal with starting height).
- Real-time score, lines, level, and next-piece HUD.
- Full keyboard controls (&larr;/&rarr; move, &darr; soft drop, Space hard drop, Z rotate CCW, X/&uarr; rotate CW, Enter pause, R restart) and on-screen touch controls.
- Clean JavaScript API (`web/tdecomp_api.js`) wrapping the WebAssembly module.

---

## Routine Inventory Census

To audit or regenerate the routine census against the reference disassembly:

```bash
python3 tools/inventory_routines.py
```

This scans `TetrisNESDisasm/main.asm` and updates [ROUTINE_INVENTORY.md](ROUTINE_INVENTORY.md) and [routine_inventory.json](routine_inventory.json).

---

## Project Structure

```
TDecomp/
├── CMakeLists.txt              # CMake build configuration
├── LICENSE                     # MIT License & attribution notices
├── README.md                   # Project documentation
├── DECOMPILATION_PROGRESS.md   # Subsystem progress & architecture
├── ROUTINE_INVENTORY.md        # Assembly symbol census
├── routine_inventory.json      # Structured routine census data
├── include/                    # Public C headers (NES state & subsystem APIs)
├── src/                        # C source implementations (libtdecomp_core.a)
├── tests/                      # Unit tests & 6502 differential CPU verification
├── web/                        # WebAssembly port (C bridge, JS API, and browser demo)
├── tools/                      # Disassembly routine census script (inventory_routines.py)
└── TetrisNESDisasm/            # Reference disassembly submodule
```

---

## Reference ROM & Verification

A commercial reference ROM (`Tetris (USA).nes`, SHA-1: `3026d28b63d94c921fe58364f8b0659d10b5a0ac`) is used to verify data tables and execute differential tests against an embedded 6502 CPU simulator.

- **ROM Not Included**: The reference ROM is not included with this repository and must be obtained independently.
- **Handling When Absent**: When the reference ROM is not found, ROM-dependent differential tests output `SKIPPED: reference ROM not found` and exit cleanly (0 failures). All core unit tests run independently without requiring a ROM.
- **Copyright Warning**: Never commit copyrighted NES ROM files or proprietary binary assets to the repository.

---

## Credits & Attribution

This project is built upon the research and reverse engineering of the NES Tetris community:

- **[TetrisNESDisasm](https://github.com/CelestialAmber/TetrisNESDisasm)**: Disassembly created and maintained by Amber Brault (CelestialAmber) and contributors.
- **[taus](https://github.com/ejona86/taus)**: Disassembly data, info files, and research by ejona86.
- **[nes-util](https://github.com/qalle2/nes-util)**: CHR conversion utilities by qalle2.

Tetris is a registered trademark of Tetris Holding, LLC. NES is a registered trademark of Nintendo of America Inc. This project is an independent educational and research decompilation not affiliated with or endorsed by Nintendo or Tetris Holding, LLC.
