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

## WebAssembly Full ROM Execution

TDecomp includes a WebAssembly emulator integration allowing the original, complete `Tetris (USA).nes` ROM to execute directly in web browsers with cycle-accurate 60 FPS video, 2A03 APU audio synthesis, and controller input.

### Compile WebAssembly Emulator

```bash
make -C web
```

Compiles the MIT-licensed `binjnes` C emulation core to `web/binjnes.js` and `web/binjnes.wasm`.

### Run Automated ROM Verification (Node.js)

```bash
node web/test_rom.js
```

Verifies the original reference ROM (`Tetris (USA).nes`), executes the 6502 CPU and 2C02 PPU through the legal screen, title screen, menus, and active gameplay, and validates 2A03 APU audio buffer generation.

### Launch Browser Emulator

Start a local HTTP server from the project root:

```bash
python3 -m http.server 8080 --directory web
```

Open `http://localhost:8080` in your web browser:
- **Automatic ROM Detection**: If `Tetris (USA).nes` or `tetris.nes` is placed in `web/`, it boots immediately.
- **User-Supplied ROM**: A drag-and-drop file picker allows loading your own `.nes` ROM file with SHA-1 integrity verification.
- **Display & Audio**: Renders the authentic 256×240 NES framebuffer with integer-ratio pixel scaling and Web Audio API sound.
- **Controls**:
  - Keyboard: <kbd>&larr;</kbd> / <kbd>&rarr;</kbd> Left/Right, <kbd>&darr;</kbd> Down, <kbd>Z</kbd> B (Rotate CCW), <kbd>X</kbd> or <kbd>&uarr;</kbd> A (Rotate CW), <kbd>Enter</kbd> Start, <kbd>Shift</kbd> / <kbd>Tab</kbd> Select, <kbd>Space</kbd> Pause, <kbd>R</kbd> Reset.
  - Gamepads: Automatic Gamepad API detection for USB / Bluetooth controllers.
  - Touch: On-screen D-Pad and action buttons for mobile devices.

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
- **[binjnes](https://github.com/binji/binjnes)**: Cycle-accurate NES emulation core in C by Ben Smith (binji), licensed under MIT and adapted for WebAssembly full ROM execution.

Tetris is a registered trademark of Tetris Holding, LLC. NES is a registered trademark of Nintendo of America Inc. This project is an independent educational and research decompilation not affiliated with or endorsed by Nintendo or Tetris Holding, LLC.
