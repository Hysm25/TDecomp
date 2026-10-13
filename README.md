# TDecomp - NES Tetris C Decompilation

TDecomp is a clean, modular C decompilation of NES Tetris (USA), providing a portable C11 library (`libtdecomp_core.a`) alongside a comprehensive test suite. The implementation models the full NES Tetris game state and subsystems (RNG, BCD math, tetrimino movement and rotation, playfield logic, player input, game state machines, menu flows, high scores, audio sequencer, and PPU rendering).

---

## Prerequisites

- **CMake**: version 3.16 or higher
- **C Compiler**: C11 compliant compiler (`gcc` or `clang`)
- **Git**: with submodule support
- **Make** or **Ninja** build system
- **Python**: version 3.8+ (optional, for routine census tooling)
- **raylib**: version 4.5+ (optional; automatically resolved via FetchContent if not installed on system)
- **Emscripten** (`emcc`): version 3.0+ (optional, for WebAssembly HTML5 build)

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

## Native Desktop Application (raylib)

TDecomp features a native desktop application powered by [raylib](https://www.raylib.com/) and driven directly by the verified decompiled C11 engine (`libtdecomp_core.a`). The frontend provides authentic gameplay rendering, authentic level palettes, Delayed Auto Shift (DAS) mechanics, score/line accounting, and procedural sound effects without embedding an NES emulator or requiring an external `.nes` ROM at runtime.

### Launch Desktop Application

```bash
./build/tdecomp
```

### Command-Line Options

```bash
# Start directly at a specific level (0 to 19)
./build/tdecomp --level 9

# Run headless simulation verification (no display or audio required)
./build/tdecomp --headless --test-frames 180

# Run without procedural audio
./build/tdecomp --no-audio
```

### Controls

| Action | Primary Key | Secondary Key | Controller / NES Equivalent |
| :--- | :--- | :--- | :--- |
| **Shift Left / Right** | <kbd>&larr;</kbd> / <kbd>&rarr;</kbd> | <kbd>A</kbd> / <kbd>D</kbd> | D-Pad Left / Right |
| **Soft Drop** | <kbd>&darr;</kbd> | <kbd>S</kbd> | D-Pad Down |
| **Rotate CCW** | <kbd>Z</kbd> | <kbd>J</kbd> | Button B |
| **Rotate CW** | <kbd>X</kbd> | <kbd>K</kbd> / <kbd>&uarr;</kbd> | Button A |
| **Start / Pause** | <kbd>Enter</kbd> | <kbd>Space</kbd> | Button Start |
| **Toggle Next Piece** | <kbd>Tab</kbd> | <kbd>Shift</kbd> | Button Select |
| **Quick Restart** | <kbd>R</kbd> | — | Reset |
| **Mute Audio** | <kbd>M</kbd> | — | Audio Toggle |
| **Quit** | <kbd>Esc</kbd> | — | Window Close |

---

## WebAssembly HTML5 Port (Browser)

TDecomp can be compiled directly to WebAssembly using **Emscripten** and **raylib's HTML5 backend**. It reuses the exact same verified C11 decompilation, game adapter, authentic level palettes, and procedural audio without running an NES emulator or requiring a `.nes` ROM file.

### Build WebAssembly Port

```bash
make -C web
```

Or via CMake:

```bash
cmake --build build --target web
```

Compiles the core engine and raylib frontend to `web/tdecomp.html`, `web/tdecomp.js`, and `web/tdecomp.wasm` (along with `web/index.html`).

### Test WebAssembly Build (Node.js)

To verify the WebAssembly build in headless mode without a browser:

```bash
make -C web test
```

Or run Node.js directly:

```bash
node web/tdecomp.js --headless --test-frames 120
```

### Play in Web Browser

Start a local HTTP server:

```bash
python3 -m http.server 8080 --directory web
# or:
make -C web serve
```

Open `http://localhost:8080` in any modern web browser. Click the canvas to focus controls and play!

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
├── native/                     # Native Raylib desktop frontend (adapter, renderer, audio)
├── web/                        # WebAssembly HTML5 port (Makefile, shell template, browser build)
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
- **[raylib](https://www.raylib.com/)**: Simple and easy-to-use library to enjoy videogames programming, used for the native desktop frontend.

Tetris is a registered trademark of Tetris Holding, LLC. NES is a registered trademark of Nintendo of America Inc. This project is an independent educational and research decompilation not affiliated with or endorsed by Nintendo or Tetris Holding, LLC.
