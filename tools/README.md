# TDecomp Tooling Directory

This directory contains standalone Python tools supporting the TDecomp build, verification, and decompilation auditing pipelines.

---

## Retained Tools Summary

| Tool Script | Primary Purpose | Invocation / Entry Point | CMake Integration | Output Artifacts |
| :--- | :--- | :--- | :--- | :--- |
| `build_rom.py` | Authoritative NES ROM build pipeline, C-to-6502 compilation, experimental ROM linking, payload verification, and CPU simulation. | `python3 tools/build_rom.py [target] [--output-dir DIR]` | Targets: `rom`, `rom_baseline`, `rom_poc`, `rom_experimental`, `rom_bank` | `build/rom/tetris_baseline.nes`<br>`build/rom/tetris_experimental.nes`<br>`build/rom/tetris_bank_exp.nes` |
| `inventory_routines.py` | Disassembly symbol parser, routine census, entry-point classification, and decompilation progress synchronizer. | `python3 tools/inventory_routines.py` | Direct script invocation | `ROUTINE_INVENTORY.md`<br>`routine_inventory.json` |

---

## Detailed Tool Documentation

### 1. `build_rom.py`

#### Purpose
`build_rom.py` is the central orchestrator for rebuilding, compiling, linking, and verifying NES ROMs from both original 6502 assembly and target-ready C implementations.

#### Execution Modes
- **`baseline`**:
  - Reassembles `TetrisNESDisasm/tetris.asm` and `TetrisNESDisasm/tetris-ram.asm` out-of-tree using `ca65` and `ld65`.
  - Produces `build/rom/tetris_baseline.nes` (49,168 bytes).
  - Verifies the SHA-1 hash (`77747840541bfc62a28a5957692a98c550bd6b2b`) and checks that PRG (32KB) and CHR (16KB) payloads are 100% bit-exact against the reference cartridge `Tetris (USA).nes`.
- **`poc`**:
  - Compiles bare-metal target C proof-of-concept routines in `experimental/c6502/proof_of_concept.c` using `cc65`.
  - Assembles generated 6502 assembly with `ca65` and links with `ld65`.
  - Verifies that compiled machine code for `copyOamStagingToOam` (11 bytes) and `muteAudio` (22 bytes) matches original 1989 Nintendo machine code bit-for-bit.
  - Measures compilation footprint for `generateNextPseudorandomNumber` (`rng.c`) and reports in-place slot feasibility.
- **`experimental`**:
  - Staged out-of-tree assembly build integrating C-compiled object code into an active NES ROM (`tetris_experimental.nes`).
  - Segments PRG Chunk 1 around `copyOamStagingToOam` ($AB5E, 11 bytes) and PRG Chunk 2 around `muteAudio` ($E271, 22 bytes) with zero code shifting.
  - Verifies caller alignment at `$801B` (`jsr $AB5E`), `$E1E8` (`jsr $E271`), and `$E247` (`jsr $E271`).
  - Runs step-by-step NMOS 6502 CPU simulator behavioral verification of both routines.
- **`bank`**:
  - Performs bank-level C decompilation feasibility audit on the High Score subsystem (`high_scores_cluster.c`, $A0EE–$A200, 275 bytes).
  - Measures compiled C footprint (636 bytes, 2.31x expansion) and documents in-place slot collision (+361B overflow into $A201).
  - Builds relocatable experimental bank ROM (`tetris_bank_exp.nes`, 49,168 bytes) mapping target C code to unreferenced cartridge slack (`$D6C9` in `unreferenced_data1`) with a 3-byte trampoline at `$A0EE`.
  - Verifies NMOS 6502 CPU differential execution across Type A (109 cycles) and Type B (112 cycles) modes with clean returns.
- **`all`** (default):
  - Runs all four phases sequentially.

#### Usage
```bash
# Run full build and verification pipeline
python3 tools/build_rom.py all

# Run individual target phases
python3 tools/build_rom.py baseline
python3 tools/build_rom.py poc
python3 tools/build_rom.py experimental
python3 tools/build_rom.py bank

# Custom output directory
python3 tools/build_rom.py all --output-dir /path/to/rom_out
```

#### Prerequisites
- Python 3.8+
- `ca65`, `ld65`, `cc65` (compiled automatically from `TetrisNESDisasm/cc65` if not present)
- Reference cartridge `Tetris (USA).nes` in project root

---

### 2. `inventory_routines.py`

#### Purpose
`inventory_routines.py` scans the assembly source files in `TetrisNESDisasm/` and label maps to generate an exhaustive census of all callable routines, alternate entry points, and authoritative data tables. It cross-checks decompiled implementations in `src/` to track project progress.

#### What it Analyzes
1. **Source Disassembly**: Parses labels, opcodes, data directives, and comments from `TetrisNESDisasm/main.asm`.
2. **Symbol Map**: Reads label addresses from `TetrisNESDisasm/tetris.lbl`.
3. **Subsystem Categorization**: Categorizes symbols into 11 functional subsystems (`MATH_RNG`, `MATH_BCD`, `TETRIMINO`, `PLAYFIELD`, `PLAYER_INPUT`, `GAMEPLAY_STATE`, `MENUS_UI`, `AUDIO`, `RENDER`, `CORE_LOGIC`, `SYSTEM`).
4. **Decompilation Verification**: Greps `src/` to confirm if routines are implemented and verified.
5. **Output Generation**:
   - Updates `ROUTINE_INVENTORY.md` (human-readable markdown catalog).
   - Updates `routine_inventory.json` (machine-readable structured schema).

#### Usage
```bash
python3 tools/inventory_routines.py
```

#### Prerequisites
- Python 3.8+
- `TetrisNESDisasm/` submodule checked out
