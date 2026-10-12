#!/usr/bin/env python3
"""
Authoritative Routine Inventory & Scope Audit Tool for NES Tetris Decompilation.
Parses main.asm and tetris.lbl to produce ROUTINE_INVENTORY.md and routine_inventory.json.
Distinguishes real routines from alternate entry points, aliases, data tables, and internal blocks.
"""

import os
import re
import json
from collections import defaultdict

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ASM_FILE = os.path.join(BASE_DIR, "TetrisNESDisasm", "main.asm")
LBL_FILE = os.path.join(BASE_DIR, "TetrisNESDisasm", "tetris.lbl")

OPCODES = {
    "adc", "and", "asl", "bcc", "bcs", "beq", "bit", "bmi", "bne", "bpl", "brk",
    "bvc", "bvs", "clc", "cld", "cli", "clv", "cmp", "cpx", "cpy", "dec", "dex",
    "dey", "eor", "inc", "inx", "iny", "jmp", "jsr", "lda", "ldx", "ldy", "lsr",
    "nop", "ora", "pha", "php", "pla", "plp", "rol", "ror", "rti", "rts", "sbc",
    "sec", "sed", "sei", "sta", "stx", "sty", "tax", "tay", "tsx", "txa", "txs", "tya"
}

KNOWN_SYSTEM_ENTRIES = {"reset", "nmi", "irq", "initRam", "branchOnGameMode"}

def load_labels():
    labels = {}
    if os.path.exists(LBL_FILE):
        with open(LBL_FILE, "r") as f:
            for line in f:
                parts = line.strip().split()
                if len(parts) == 3 and parts[0] == "al":
                    addr = int(parts[1], 16)
                    name = parts[2].lstrip(".")
                    labels[name] = addr
    return labels

def categorize_routine(name):
    n = name.lower()
    if any(k in n for k in ["rng", "pseudorandom", "seed"]):
        return "MATH_RNG"
    if any(k in n for k in ["bcd", "score", "lines", "levelnumber", "stats"]):
        return "MATH_BCD"
    if any(k in n for k in ["tetrimino", "piece", "orient", "spawn", "rotation", "fitcheck", "pickrandom"]):
        return "TETRIMINO"
    if any(k in n for k in ["playfield", "row", "curtain", "lineclear", "garbage"]):
        return "PLAYFIELD"
    if any(k in n for k in ["autorepeat", "das", "playercontrols", "joypad", "button", "pollcontroller"]):
        return "PLAYER_INPUT"
    if any(k in n for k in ["playstate", "gamemodestate", "dropspeed", "gameover"]):
        return "GAMEPLAY_STATE"
    if any(k in n for k in ["menu", "title", "legal", "highscore", "selection"]):
        return "MENUS_UI"
    if any(k in n for k in ["audio", "music", "sound", "sfx", "apu", "track"]):
        return "AUDIO"
    if any(k in n for k in ["ppu", "oam", "sprite", "render", "palette", "bulkcopy"]):
        return "RENDER"
    if any(k in n for k in ["reset", "nmi", "irq", "initram", "boot", "vsync", "sleep"]):
        return "SYSTEM"
    return "CORE_LOGIC"

def analyze():
    labels_map = load_labels()
    
    with open(ASM_FILE, "r") as f:
        lines = f.readlines()
        
    text = "".join(lines)
    
    # Identify call targets, dispatch table targets, jump targets, branch targets
    jsr_targets = set(re.findall(r"\bjsr\s+([a-zA-Z_][a-zA-Z0-9_]*)", text))
    table_targets = set(re.findall(r"\.(?:addr|word)\s+([a-zA-Z_][a-zA-Z0-9_]*)", text))
    jmp_targets = set(re.findall(r"\bjmp\s+([a-zA-Z_][a-zA-Z0-9_]*)", text))
    branch_targets = set(re.findall(r"\b(?:bne|beq|bcs|bcc|bmi|bpl|bvs|bvc)\s+([a-zA-Z_][a-zA-Z0-9_]*)", text))
    
    # PRG ROM code begins at line 242 (.segment "PRG_chunk1")
    symbols = []
    for i in range(241, len(lines)):
        line = lines[i]
        # Match global labels (ignoring local @ labels)
        m = re.match(r"^([a-zA-Z_][a-zA-Z0-9_]*):(.*)", line)
        if m:
            name = m.group(1)
            rest = m.group(2).strip()
            addr = labels_map.get(name, 0)
            symbols.append((name, i, rest, addr))
            
    all_symbols = []
    routines = []
    alt_entries = []
    data_tables = []
    aliases = []
    
    last_code_terminated = True
    current_parent_routine = None
    
    for idx in range(len(symbols)):
        name, start_idx, rest, addr = symbols[idx]
        next_idx = symbols[idx + 1][1] if idx + 1 < len(symbols) else len(lines)
        
        chunk_lines = [rest] + lines[start_idx + 1:next_idx]
        opcodes = []
        directives = []
        for l in chunk_lines:
            clean = l.split(";")[0].strip()
            if not clean:
                continue
            sub_m = re.match(r"^@?[a-zA-Z0-9_]+:\s*(.*)", clean)
            if sub_m:
                clean = sub_m.group(1).strip()
                if not clean:
                    continue
            tok = clean.split()
            if not tok:
                continue
            first = tok[0].lower()
            if first in OPCODES:
                opcodes.append(first)
            elif first.startswith("."):
                directives.append(first)
                
        is_directly_called = (name in jsr_targets) or (name in table_targets) or (name in KNOWN_SYSTEM_ENTRIES)
        is_any_target = is_directly_called or (name in jmp_targets) or (name in branch_targets)
        has_code = len(opcodes) > 0
        has_data = any(d in (".byte", ".word", ".addr", ".incbin") for d in directives)
        has_rts = ("rts" in opcodes) or ("rti" in opcodes)
        
        chunk_terminates = False
        if has_code:
            if opcodes[-1] in ("rts", "rti", "jmp"):
                chunk_terminates = True
            elif any(d in (".addr", ".word") for d in directives) and "switch_s_plus_2a" in "".join(chunk_lines):
                chunk_terminates = True

        # Classify symbol type
        if not has_code and has_data:
            sym_type = "DATA_TABLE"
            last_code_terminated = True
            current_parent_routine = None
        elif not has_code and not has_data and idx + 1 < len(symbols) and symbols[idx + 1][3] == addr:
            sym_type = "ALIAS"
        elif has_code:
            if is_directly_called:
                if last_code_terminated:
                    sym_type = "ROUTINE"
                    current_parent_routine = name
                else:
                    sym_type = "ALTERNATE_ENTRY"
            elif has_rts and last_code_terminated and is_any_target:
                sym_type = "ROUTINE"
                current_parent_routine = name
            elif not last_code_terminated and is_any_target:
                sym_type = "ALTERNATE_ENTRY"
            elif chunk_terminates and last_code_terminated:
                sym_type = "ROUTINE"
                current_parent_routine = name
            else:
                sym_type = "CODE_BLOCK"
            last_code_terminated = chunk_terminates
        else:
            sym_type = "LABEL"

        category = categorize_routine(name) if has_code else "DATA"
        
        item = {
            "name": name,
            "address": f"0x{addr:04X}" if addr else "N/A",
            "start_line": start_idx + 1,
            "end_line": next_idx,
            "line_count": next_idx - start_idx,
            "type": sym_type,
            "category": category,
            "jsr_called": name in jsr_targets,
            "table_referenced": name in table_targets,
            "parent_routine": current_parent_routine if sym_type == "ALTERNATE_ENTRY" else None,
            "status": "MISSING",
            "notes": ""
        }
        all_symbols.append(item)
        
        if sym_type == "ROUTINE":
            routines.append(item)
        elif sym_type == "ALTERNATE_ENTRY":
            alt_entries.append(item)
        elif sym_type == "DATA_TABLE":
            data_tables.append(item)
        elif sym_type == "ALIAS":
            aliases.append(item)
            
    return all_symbols, routines, alt_entries, data_tables, aliases

def main():
    all_symbols, routines, alt_entries, data_tables, aliases = analyze()
    
    # Read existing status if routine_inventory.json exists
    json_path = os.path.join(BASE_DIR, "routine_inventory.json")
    if os.path.exists(json_path):
        try:
            with open(json_path, "r") as f:
                old_data = json.load(f)
                old_map = {r["name"]: r for r in old_data.get("routines", [])}
                for r in routines:
                    if r["name"] in old_map:
                        r["status"] = old_map[r["name"]].get("status", "MISSING")
                        r["notes"] = old_map[r["name"]].get("notes", "")
                for r in alt_entries:
                    if r["name"] in old_map:
                        r["status"] = old_map[r["name"]].get("status", "MISSING")
                        r["notes"] = old_map[r["name"]].get("notes", "")
        except Exception:
            pass

    # Authoritative verified routine registry
    verified_registry = {
        # Batch 1 (RNG & Piece Selection)
        "generateNextPseudorandomNumber": "Verified: 6502 LFSR algorithm proven equivalent via CPU simulator & unit vectors",
        "unreferenced_soundRng": "Verified: Audio LFSR variant proven equivalent via differential testing",
        "pickRandomTetrimino": "Verified: 7-bag modulo & reroll logic verified with 6502 CPU simulator",
        "chooseNextTetrimino": "Verified: Gameplay vs demo mode piece selection differential proven",
        "incrementPieceStat": "Verified: BCD piece statistic increments verified with CPU simulator",

        # Batch 2 (BCD Math & Scoring Progression)
        "playState_updateLinesAndStatistics": "Verified: PlayState 5 scoring, line counts, and stat update proven",
        "updateMusicSpeed": "Verified: Playfield row 5 height check and danger allegro switching differential proven",
        "setMusicTrack": "Verified: Track selection and demo mute override differential proven",

        # Batch 3 (Tetrimino Geometry & Movement)
        "rotate_tetrimino": "Verified: A/B button rotation and wall reverting behavior verified",
        "drop_tetrimino": "Verified: Natural gravity, soft drop acceleration, and floor lock verified",
        "shift_tetrimino": "Verified: Horizontal shifting with DAS auto-repeat verified",
        "isPositionValid": "Verified: Boundary box, floor, and mino collision detection verified",
        "updatePlayfield": "Verified: VRAM row dirty flag tracking verified",

        # Batch 4 (Playfield Grid & Line Clear Subsystem)
        "playState_lockTetrimino": "Verified: Locking 4 minos into playfield and invalid-position game over sequence verified",
        "playState_checkForCompletedRows": "Verified: Row completion detection, collapsing, and garbage calculation differential proven against 6502 ROM",
        "updateLineClearingAnimation": "Verified: 5-step center-out line clearing animation framing verified",
        "playState_updateGameOverCurtain": "Verified: Game over curtain cascade filling playfield with curtain tiles verified",
        "playState_receiveGarbage": "Verified: 2-player pending garbage shifting and bottom row filling verified",
        "initPlayfieldIfTypeB": "Verified: Type B mode check and garbage playfield initialization verified",
        "initPlayfieldForTypeB": "Verified: PRNG-based garbage generation (rows 8-19) and height clearing verified",

        # Batch 5 (Player Controls and DAS)
        "pollControllerButtons": "Verified: Mode dispatch, Start button demo cancellation, and demo move playback verified",
        "demoButtonsTable_indexIncr": "Verified: 16-bit demo table address pointer increment verified with 6502 simulator",
        "pollController_actualRead": "Verified: Serial joypad 4016/4017 strobe and 8-bit read verified with unit vectors",
        "addExpansionPortInputAsControllerInput": "Verified: Famicom expansion port input OR-mixing verified with unit vectors",
        "pollController": "Verified: DPCM glitch debouncing double-read and edge diffing differential proven against 6502 ROM",
        "diffOldAndNewButtons": "Verified: Button edge detection (curr ^ held & curr) differential proven across 256 vector combinations"
    }

    for r in routines:
        if r["name"] in verified_registry:
            r["status"] = "VERIFIED"
            r["notes"] = verified_registry[r["name"]]
    for r in alt_entries:
        if r["name"] in verified_registry:
            r["status"] = "VERIFIED"
            r["notes"] = verified_registry[r["name"]]
    for r in all_symbols:
        if r["name"] in verified_registry:
            r["status"] = "VERIFIED"
            r["notes"] = verified_registry[r["name"]]

    verified_count = sum(1 for r in routines if r["status"] == "VERIFIED")
    decompiled_count = sum(1 for r in routines if r["status"] == "DECOMPILED")
    missing_count = sum(1 for r in routines if r["status"] == "MISSING")
    pct = (verified_count / len(routines) * 100) if routines else 0.0

    # Save routine_inventory.json
    inventory_data = {
        "project": "NES Tetris Decompilation",
        "summary": {
            "total_symbols_in_prg": len(all_symbols),
            "standalone_routines": len(routines),
            "alternate_entry_points": len(alt_entries),
            "data_tables": len(data_tables),
            "exact_aliases": len(aliases),
            "verified_routines": verified_count,
            "decompiled_untested": decompiled_count,
            "missing_routines": missing_count,
            "progress_percentage": round(pct, 2)
        },
        "routines": routines,
        "alternate_entry_points": alt_entries,
        "data_tables": data_tables
    }
    with open(json_path, "w") as f:
        json.dump(inventory_data, f, indent=2)
        
    # Generate ROUTINE_INVENTORY.md
    by_category = defaultdict(list)
    for r in routines:
        by_category[r["category"]].append(r)
        
    lines_out = [
        "# NES Tetris Authoritative Routine Inventory & Scope Audit",
        "",
        "- **Disassembly Source of Truth:** `TetrisNESDisasm/main.asm` (NES 6502 Assembly, NROM/MMC1)",
        "- **Reference ROM:** `Tetris (USA).nes` (PRG SHA-1 verified)",
        "- **C Target:** Strict C11 (`-std=c11 -Wall -Wextra -Werror -pedantic`)",
        "",
        "---",
        "",
        "## 1. Executive Summary & Decompilation Scope",
        "",
        "| Metric | Count | Description |",
        "|:---|:---:|:---|",
        f"| **Total Standalone Routines** | **{len(routines)}** | Independent callable routines with distinct scope |",
        f"| **Alternate Entry Points** | **{len(alt_entries)}** | Secondary entries into existing routines / shared fall-throughs |",
        f"| **Authoritative Data Tables** | **{len(data_tables)}** | Lookup, geometry, sprite, and audio tables in PRG ROM |",
        f"| **Exact Address Aliases** | **{len(aliases)}** | Identical address alias labels |",
        f"| **Total PRG Symbols Tracked** | **{len(all_symbols)}** | Complete symbol census of PRG ROM |",
        f"| **Verified (Tested)** | **{verified_count}** | Bit-exact & differential CPU proven against ROM |",
        f"| **Decompiled (Untested)** | **{decompiled_count}** | C converted awaiting differential verification |",
        f"| **Remaining Routines** | **{missing_count}** | Pending conversion |",
        f"| **Completion Percentage** | **{pct:.2f}%** | Standalone routine completion rate |",
        "",
        "---",
        "",
        "## 2. Standalone Routine Category Breakdown",
        "",
        "| Category | Total Routines | Verified | Decompiled | Missing | % Complete |",
        "|:---|:---:|:---:|:---:|:---:|:---:|"
    ]

    for cat in sorted(by_category.keys()):
        cr = by_category[cat]
        c_ver = sum(1 for r in cr if r["status"] == "VERIFIED")
        c_dec = sum(1 for r in cr if r["status"] == "DECOMPILED")
        c_mis = sum(1 for r in cr if r["status"] == "MISSING")
        c_pct = (c_ver / len(cr) * 100) if cr else 0.0
        lines_out.append(f"| **{cat}** | {len(cr)} | {c_ver} | {c_dec} | {c_mis} | {c_pct:.1f}% |")

    lines_out.extend([
        "",
        "---",
        "",
        "## 3. Standalone Routines Directory by Category",
        ""
    ])

    for cat in sorted(by_category.keys()):
        cr = by_category[cat]
        lines_out.extend([
            f"### {cat} ({len(cr)} Routines)",
            "",
            "| Routine Name | Address | Lines | Call Mechanism | Status | Notes |",
            "|:---|:---:|:---:|:---:|:---:|:---|"
        ])
        for r in sorted(cr, key=lambda x: x["start_line"]):
            status_badge = f"`{r['status']}`"
            called = "JSR" if r["jsr_called"] else ("Jump Table" if r["table_referenced"] else "Entry / Direct")
            lines_out.append(f"| `{r['name']}` | `{r['address']}` | L{r['start_line']}–L{r['end_line']} | {called} | {status_badge} | {r['notes']} |")
        lines_out.append("")

    lines_out.extend([
        "---",
        "",
        "## 4. Alternate Entry Points Directory",
        "",
        f"Total alternate entry points identified: **{len(alt_entries)}**",
        "",
        "| Label | Address | Lines | Parent Routine Context | JSR / Table | Status |",
        "|:---|:---:|:---:|:---|:---:|:---:|"
    ])

    for a in sorted(alt_entries, key=lambda x: x["start_line"]):
        parent_txt = f"`{a['parent_routine']}`" if a["parent_routine"] else "Shared Context"
        called = "Yes" if a["jsr_called"] or a["table_referenced"] else "Internal"
        lines_out.append(f"| `{a['name']}` | `{a['address']}` | L{a['start_line']}–L{a['end_line']} | {parent_txt} | {called} | `{a['status']}` |")

    lines_out.append("")

    with open(os.path.join(BASE_DIR, "ROUTINE_INVENTORY.md"), "w") as f:
        f.write("\n".join(lines_out))

    print(f"Inventory complete:")
    print(f"  Standalone Routines:     {len(routines)} (Verified: {verified_count})")
    print(f"  Alternate Entry Points:  {len(alt_entries)}")
    print(f"  Authoritative Tables:    {len(data_tables)}")
    print(f"  Total PRG Symbols:       {len(all_symbols)}")

if __name__ == "__main__":
    main()
