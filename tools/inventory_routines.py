#!/usr/bin/env python3
"""
Authoritative Routine Inventory & Scope Audit Tool for NES Tetris Decompilation.
Parses main.asm and tetris.lbl to produce ROUTINE_INVENTORY.md and routine_inventory.json.
"""

import os
import re
import json
from collections import defaultdict

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ASM_FILE = os.path.join(BASE_DIR, "TetrisNESDisasm", "main.asm")
LBL_FILE = os.path.join(BASE_DIR, "TetrisNESDisasm", "tetris.lbl")
RAM_FILE = os.path.join(BASE_DIR, "TetrisNESDisasm", "tetris-ram.asm")

OPCODES = {
    "adc", "and", "asl", "bcc", "bcs", "beq", "bit", "bmi", "bne", "bpl", "brk",
    "bvc", "bvs", "clc", "cld", "cli", "clv", "cmp", "cpx", "cpy", "dec", "dex",
    "dey", "eor", "inc", "inx", "iny", "jmp", "jsr", "lda", "ldx", "ldy", "lsr",
    "nop", "ora", "pha", "php", "pla", "plp", "rol", "ror", "rti", "rts", "sbc",
    "sec", "sed", "sei", "sta", "stx", "sty", "tax", "tay", "tsx", "txa", "txs", "tya"
}

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
    if any(k in n for k in ["autorepeat", "das", "playercontrols", "joypad", "button"]):
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
    
    # Identify JSR targets, table targets, vectors
    jsr_targets = set(re.findall(r"\bjsr\s+([a-zA-Z_][a-zA-Z0-9_]*)", text))
    table_targets = set(re.findall(r"\.(?:addr|word)\s+([a-zA-Z_][a-zA-Z0-9_]*)", text))
    direct_jmp_targets = set(re.findall(r"\bjmp\s+([a-zA-Z_][a-zA-Z0-9_]*)", text))
    
    # Known hardware / top-level entry points
    known_entry_points = {"reset", "nmi", "irq", "branchOnGameMode"}
    
    # All global labels in main.asm
    global_labels = []
    for idx, line in enumerate(lines):
        m = re.match(r"^([a-zA-Z_][a-zA-Z0-9_]*):", line)
        if m:
            label = m.group(1)
            addr = labels_map.get(label, 0)
            global_labels.append((label, idx, addr))
            
    # Classify each label
    all_symbols = []
    callable_routines = []
    
    for i in range(len(global_labels)):
        name, start_idx, addr = global_labels[i]
        end_idx = global_labels[i+1][1] if i + 1 < len(global_labels) else len(lines)
        chunk = lines[start_idx:end_idx]
        
        has_code = False
        has_data = False
        ends_with_return = False
        
        for l in chunk:
            clean = l.split(";")[0].strip()
            tokens = clean.split()
            if not tokens:
                continue
            first = tokens[0].lower()
            if first in OPCODES:
                has_code = True
                if first in ("rts", "rti"):
                    ends_with_return = True
            elif first in (".byte", ".word", ".addr", ".incbin"):
                has_data = True
                
        is_callable = (
            name in jsr_targets or 
            name in table_targets or 
            name in known_entry_points or 
            (has_code and ends_with_return)
        )
        
        category = categorize_routine(name) if has_code else "DATA"
        symbol_type = "ROUTINE" if (has_code and is_callable) else ("DATA_TABLE" if has_data else ("CODE_BLOCK" if has_code else "LABEL"))
        
        item = {
            "name": name,
            "address": f"0x{addr:04X}" if addr else "N/A",
            "start_line": start_idx + 1,
            "end_line": end_idx,
            "line_count": end_idx - start_idx,
            "type": symbol_type,
            "category": category,
            "jsr_called": name in jsr_targets,
            "table_referenced": name in table_targets,
            "status": "MISSING",
            "notes": ""
        }
        all_symbols.append(item)
        if symbol_type == "ROUTINE":
            callable_routines.append(item)
            
    return all_symbols, callable_routines

def main():
    all_symbols, callable_routines = analyze()
    
    # Read existing status if routine_inventory.json exists
    json_path = os.path.join(BASE_DIR, "routine_inventory.json")
    if os.path.exists(json_path):
        try:
            with open(json_path, "r") as f:
                old_data = json.load(f)
                old_map = {r["name"]: r for r in old_data.get("routines", [])}
                for r in callable_routines:
                    if r["name"] in old_map:
                        r["status"] = old_map[r["name"]].get("status", "MISSING")
                        r["notes"] = old_map[r["name"]].get("notes", "")
        except Exception:
            pass

    # Save routine_inventory.json
    inventory_data = {
        "project": "NES Tetris Decompilation",
        "total_standalone_routines": len(callable_routines),
        "total_symbols": len(all_symbols),
        "routines": callable_routines
    }
    with open(json_path, "w") as f:
        json.dump(inventory_data, f, indent=2)
        
    # Generate ROUTINE_INVENTORY.md
    by_category = defaultdict(list)
    for r in callable_routines:
        by_category[r["category"]].append(r)
        
    verified_count = sum(1 for r in callable_routines if r["status"] == "VERIFIED")
    decompiled_count = sum(1 for r in callable_routines if r["status"] == "DECOMPILED")
    missing_count = sum(1 for r in callable_routines if r["status"] == "MISSING")
    
    pct = (verified_count / len(callable_routines) * 100) if callable_routines else 0.0

    lines_out = [
        "# NES Tetris Authoritative Routine Inventory & Scope Audit",
        "",
        f"- **Disassembly Source of Truth:** `TetrisNESDisasm/main.asm` (NES 6502 Assembly, NROM/MMC1)",
        f"- **Reference ROM:** `Tetris (USA).nes` (PRG SHA-1 verified)",
        f"- **C Target:** Strict C11 (`-std=c11 -Wall -Wextra -Werror -pedantic`)",
        "",
        "---",
        "",
        "## 1. Executive Summary & Decompilation Scope",
        "",
        f"| Metric | Count |",
        f"|:---|:---:|",
        f"| **Total Standalone Routines** | **{len(callable_routines)}** |",
        f"| **Verified (Tested)** | **{verified_count}** |",
        f"| **Decompiled (Untested)** | **{decompiled_count}** |",
        f"| **Remaining (Missing)** | **{missing_count}** |",
        f"| **Completion Percentage** | **{pct:.2f}%** |",
        "",
        "---",
        "",
        "## 2. Category Breakdown",
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
        "## 3. Routine Directory by Category",
        ""
    ])

    for cat in sorted(by_category.keys()):
        cr = by_category[cat]
        lines_out.extend([
            f"### {cat} ({len(cr)} Routines)",
            "",
            "| Routine Name | Address | Lines | JSR Called | Status | Notes |",
            "|:---|:---:|:---:|:---:|:---:|:---|"
        ])
        for r in sorted(cr, key=lambda x: x["start_line"]):
            status_badge = f"`{r['status']}`"
            called = "Yes" if r["jsr_called"] else ("Table" if r["table_referenced"] else "Entry")
            lines_out.append(f"| `{r['name']}` | `{r['address']}` | L{r['start_line']}–L{r['end_line']} | {called} | {status_badge} | {r['notes']} |")
        lines_out.append("")

    with open(os.path.join(BASE_DIR, "ROUTINE_INVENTORY.md"), "w") as f:
        f.write("\n".join(lines_out))

    print(f"Inventory complete: {len(callable_routines)} routines identified.")

if __name__ == "__main__":
    main()
