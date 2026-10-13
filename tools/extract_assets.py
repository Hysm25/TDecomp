#!/usr/bin/env python3
"""
tools/extract_assets.py - Authoritative ROM Importer and Asset Extractor for TDecomp

Validates the original commercial Tetris (USA).nes ROM against its authoritative
SHA-1 hash, checks the iNES header structure, and extracts all CHR pattern graphics,
color palettes, piece tables, and audio data needed by TDecomp.
"""

import sys
import os
import hashlib
import argparse

EXPECTED_ROM_SIZE = 49168
EXPECTED_SHA1 = "3026d28b63d94c921fe58364f8b0659d10b5a0ac"
HEADER_SIZE = 16
PRG_SIZE = 32768
CHR_SIZE = 16384
CHR_BANK_SIZE = 4096

# PRG offsets (CPU address - $8000)
TABLE_OFFSETS = {
    "colorTable": (0x984C - 0x8000, 40, "Palettes: 10 levels x 4 colors"),
    "game_palette": (0xACF3 - 0x8000, 36, "Palettes: In-game background and sprite palettes"),
    "menu_palette": (0xAD2B - 0x8000, 24, "Palettes: Menu and title screen palettes"),
    "ending_palette": (0xAD43 - 0x8000, 36, "Palettes: Ending cutscene palettes"),
    "legal_palette": (0xAD17 - 0x8000, 20, "Palettes: Legal disclaimer palette"),
    "orientationTable": (0x8A9C - 0x8000, 228, "Pieces: 19 orientations x 4 minos x 3 bytes"),
    "rotationTable": (0x88EE - 0x8000, 38, "Pieces: CCW/CW rotation state transitions"),
    "framesPerDropTable": (0x898E - 0x8000, 29, "Gameplay: Gravity drop speed table"),
    "tetriminoTypeTable": (0x993B - 0x8000, 7, "Pieces: Tetrimino base types"),
    "spawnTable": (0x994E - 0x8000, 7, "Pieces: Initial spawn coordinates"),
    "spawnOrientTable": (0x9956 - 0x8000, 7, "Pieces: Initial spawn orientations"),
    "pointsTable": (0x9CA5 - 0x8000, 8, "Scoring: Line clear score multiplier"),
    "musicSelectionTable": (0x85D2 - 0x8000, 8, "Audio: Track address lookup"),
    "noteToWaveTable": (0xEB13 - 0x8000, 156, "Audio: Note frequency table"),
    "noteDurationTable": (0xEBAF - 0x8000, 116, "Audio: Note duration table"),
    "sfx_slot0_init_table": (0xE009 - 0x8000, 6, "Audio: SFX slot 0 initialization"),
    "sfx_slot1_init_table": (0xE015 - 0x8000, 20, "Audio: SFX slot 1 initialization"),
    "sfx_slot2_init_table": (0xE045 - 0x8000, 189, "Audio: SFX slot 2 initialization"),
    "sfx_slot3_init_table": (0xE03D - 0x8000, 4, "Audio: SFX slot 3 initialization"),
    "highScoreCharToTile": (0xA08C - 0x8000, 48, "Text: High score character to PPU tile"),
    "defaultHighScores": (0xAD67 - 0x8000, 24, "Text: Default high score table entries"),
    "game_nametable": (0xBF3C - 0x8000, 1121, "Graphics: Playfield layout nametable patch"),
}

CHR_BANKS = [
    (0, "title_menu_tileset.chr", "CHR Bank 0: Title & Menu screen tileset"),
    (1, "typeB_ending_tileset.chr", "CHR Bank 1: Type B ending cutscene tileset"),
    (2, "typeA_ending_tileset.chr", "CHR Bank 2: Type A ending cutscene tileset"),
    (3, "game_tileset.chr", "CHR Bank 3: In-game playfield & tetromino tileset"),
]

def validate_rom(rom_data):
    """Validates iNES header and SHA-1 hash."""
    if len(rom_data) < EXPECTED_ROM_SIZE:
        return False, f"File size is {len(rom_data)} bytes; expected at least {EXPECTED_ROM_SIZE} bytes."

    # Header check
    if rom_data[:4] != b"NES\x1a":
        return False, f"Invalid header magic: {rom_data[:4]!r}; expected b'NES\\x1a'."

    prg_units = rom_data[4]
    chr_units = rom_data[5]
    if prg_units != 2 or chr_units != 2:
        return False, f"Invalid geometry: PRG units={prg_units} (need 2), CHR units={chr_units} (need 2)."

    # SHA-1 verification
    sha1 = hashlib.sha1(rom_data[:EXPECTED_ROM_SIZE]).hexdigest()
    if sha1.lower() != EXPECTED_SHA1.lower():
        return False, (
            f"SHA-1 hash mismatch!\n"
            f"  Expected: {EXPECTED_SHA1}\n"
            f"  Actual:   {sha1}\n"
            f"This file does not match the official commercial Tetris (USA).nes ROM."
        )

    return True, sha1

def extract_assets(rom_path, output_dir):
    """Extracts all game assets from the ROM."""
    print(f"==> Opening ROM: {rom_path}")
    if not os.path.isfile(rom_path):
        print(f"[ERROR] ROM file not found: {rom_path}", file=sys.stderr)
        return 1

    with open(rom_path, "rb") as f:
        rom_data = f.read()

    valid, msg = validate_rom(rom_data)
    if not valid:
        print(f"[ERROR] ROM Validation Failed:\n{msg}", file=sys.stderr)
        return 1

    sha1 = msg
    print(f"[SUCCESS] ROM validation passed!")
    print(f"  ROM File:  {rom_path}")
    print(f"  File Size: {len(rom_data)} bytes")
    print(f"  SHA-1:     {sha1}")
    print(f"  PRG ROM:   {PRG_SIZE} bytes (32 KB)")
    print(f"  CHR ROM:   {CHR_SIZE} bytes (16 KB)")

    os.makedirs(output_dir, exist_ok=True)

    prg_rom = rom_data[HEADER_SIZE : HEADER_SIZE + PRG_SIZE]
    chr_rom = rom_data[HEADER_SIZE + PRG_SIZE : HEADER_SIZE + PRG_SIZE + CHR_SIZE]

    # 1. Save raw PRG and CHR binary dumps
    prg_path = os.path.join(output_dir, "prg_rom.bin")
    chr_path = os.path.join(output_dir, "chr_rom.bin")
    with open(prg_path, "wb") as f:
        f.write(prg_rom)
    with open(chr_path, "wb") as f:
        f.write(chr_rom)
    print(f"  -> Saved PRG ROM dump to: {prg_path}")
    print(f"  -> Saved CHR ROM dump to: {chr_path}")

    # 2. Extract CHR Banks
    print("\n==> Extracting CHR Pattern Banks:")
    for bank_idx, filename, desc in CHR_BANKS:
        bank_data = chr_rom[bank_idx * CHR_BANK_SIZE : (bank_idx + 1) * CHR_BANK_SIZE]
        bank_path = os.path.join(output_dir, filename)
        with open(bank_path, "wb") as f:
            f.write(bank_data)
        print(f"  [Bank {bank_idx}] {filename} ({len(bank_data)} bytes) - {desc}")

    # 3. Extract PRG Data Tables
    print("\n==> Extracting PRG Data Tables & Palettes:")
    for name, (offset, size, desc) in TABLE_OFFSETS.items():
        table_data = prg_rom[offset : offset + size]
        table_path = os.path.join(output_dir, f"{name}.bin")
        with open(table_path, "wb") as f:
            f.write(table_data)
        print(f"  - {name:24s} ({size:4d} bytes) - {desc}")

    print(f"\n==> All assets successfully extracted to: {output_dir}/")
    return 0

def main():
    parser = argparse.ArgumentParser(description="Extract original assets from Tetris (USA).nes")
    parser.add_argument("rom_path", nargs="?", default="Tetris (USA).nes",
                        help="Path to Tetris (USA).nes (default: 'Tetris (USA).nes')")
    parser.add_argument("--output", "-o", default="assets",
                        help="Output directory for extracted assets (default: 'assets')")
    args = parser.parse_args()

    # Search candidates if default path does not exist
    rom_file = args.rom_path
    if not os.path.isfile(rom_file):
        candidates = [
            "../Tetris (USA).nes",
            "../../Tetris (USA).nes",
            "/mnt/sdcard/Download/Tetris (USA)/Tetris (USA).nes",
        ]
        for c in candidates:
            if os.path.isfile(c):
                rom_file = c
                break

    return extract_assets(rom_file, args.output)

if __name__ == "__main__":
    sys.exit(main())
