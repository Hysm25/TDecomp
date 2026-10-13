# TDecomp Tooling Directory

This directory contains standalone Python tooling supporting the TDecomp decompilation auditing and symbol tracking pipeline.

---

## Retained Tools Summary

| Tool Script | Primary Purpose | Invocation / Entry Point | Output Artifacts |
| :--- | :--- | :--- | :--- |
| `inventory_routines.py` | Disassembly symbol parser, routine census, entry-point classification, and decompilation progress synchronizer. | `python3 tools/inventory_routines.py` | `ROUTINE_INVENTORY.md`<br>`routine_inventory.json` |

---

## Detailed Tool Documentation

### `inventory_routines.py`

#### Purpose
`inventory_routines.py` scans the assembly source files in `TetrisNESDisasm/` and label maps to generate an exhaustive census of all callable routines, alternate entry points, and authoritative data tables. It cross-checks decompiled implementations in `src/` to track project progress.

#### What it Analyzes
1. **Source Disassembly**: Parses labels, opcodes, data directives, and comments from `TetrisNESDisasm/main.asm`.
2. **Symbol Map**: Reads label addresses from `TetrisNESDisasm/tetris.lbl`.
3. **Subsystem Categorization**: Categorizes symbols into 11 functional subsystems (`MATH_RNG`, `MATH_BCD`, `TETRIMINO`, `PLAYFIELD`, `PLAYER_INPUT`, `GAMEPLAY_STATE`, `MENUS_UI`, `AUDIO`, `RENDER`, `CORE_LOGIC`, `SYSTEM`).
4. **Decompilation Verification**: Greps `src/` to confirm routines implemented and verified.
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
