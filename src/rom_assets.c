#include "rom_assets.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --- RFC 3174 SHA-1 Implementation --- */

#define SHA1_ROTL(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

typedef struct {
    uint32_t state[5];
    uint64_t count;
    uint8_t buffer[64];
} SHA1_Context;

static void sha1_transform(uint32_t state[5], const uint8_t buffer[64]) {
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3], e = state[4];
    uint32_t w[80];
    int i;

    for (i = 0; i < 16; i++) {
        w[i] = ((uint32_t)buffer[i * 4] << 24) |
               ((uint32_t)buffer[i * 4 + 1] << 16) |
               ((uint32_t)buffer[i * 4 + 2] << 8) |
               ((uint32_t)buffer[i * 4 + 3]);
    }
    for (i = 16; i < 80; i++) {
        w[i] = SHA1_ROTL(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    }

    for (i = 0; i < 80; i++) {
        uint32_t f, k;
        if (i < 20) {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }
        uint32_t temp = SHA1_ROTL(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = SHA1_ROTL(b, 30);
        b = a;
        a = temp;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

static void sha1_init(SHA1_Context *ctx) {
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;
    ctx->count = 0;
}

static void sha1_update(SHA1_Context *ctx, const uint8_t *data, size_t len) {
    size_t i = 0;
    size_t buf_idx = (size_t)(ctx->count % 64);
    ctx->count += len;

    while (i < len) {
        size_t to_copy = 64 - buf_idx;
        if (to_copy > len - i) to_copy = len - i;
        memcpy(ctx->buffer + buf_idx, data + i, to_copy);
        i += to_copy;
        buf_idx = (buf_idx + to_copy) % 64;
        if (buf_idx == 0) {
            sha1_transform(ctx->state, ctx->buffer);
        }
    }
}

static void sha1_final(SHA1_Context *ctx, uint8_t hash[20]) {
    uint8_t final_count[8];
    uint64_t bit_count = ctx->count * 8;
    for (int i = 0; i < 8; i++) {
        final_count[i] = (uint8_t)((bit_count >> ((7 - i) * 8)) & 0xFF);
    }
    uint8_t pad = 0x80;
    sha1_update(ctx, &pad, 1);
    while ((ctx->count % 64) != 56) {
        uint8_t zero = 0x00;
        sha1_update(ctx, &zero, 1);
    }
    sha1_update(ctx, final_count, 8);
    for (int i = 0; i < 5; i++) {
        hash[i * 4]     = (uint8_t)((ctx->state[i] >> 24) & 0xFF);
        hash[i * 4 + 1] = (uint8_t)((ctx->state[i] >> 16) & 0xFF);
        hash[i * 4 + 2] = (uint8_t)((ctx->state[i] >> 8) & 0xFF);
        hash[i * 4 + 3] = (uint8_t)(ctx->state[i] & 0xFF);
    }
}

void sha1_compute(const uint8_t *data, size_t len, uint8_t hash[20]) {
    SHA1_Context ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, data, len);
    sha1_final(&ctx, hash);
}

void sha1_to_hex(const uint8_t hash[20], char hex_out[41]) {
    for (int i = 0; i < 20; i++) {
        snprintf(hex_out + i * 2, 3, "%02x", hash[i]);
    }
    hex_out[40] = '\0';
}

/* --- ROM Extraction & Validation --- */

const char *rom_status_to_string(RomStatus status) {
    switch (status) {
        case ROM_STATUS_OK:
            return "ROM verified and extracted successfully";
        case ROM_ERR_NULL_PTR:
            return "Null pointer supplied to ROM loader";
        case ROM_ERR_FILE_NOT_FOUND:
            return "ROM file not found";
        case ROM_ERR_READ_FAILED:
            return "Failed to read ROM file contents";
        case ROM_ERR_TOO_SMALL:
            return "File size is smaller than expected 49,168 bytes";
        case ROM_ERR_INVALID_MAGIC:
            return "Invalid iNES header magic (expected 'NES\\x1A')";
        case ROM_ERR_INVALID_HEADER:
            return "Invalid iNES header sizes (expected 32KB PRG, 16KB CHR)";
        case ROM_ERR_HASH_MISMATCH:
            return "ROM SHA-1 hash mismatch (expected authentic USA ROM)";
        case ROM_ERR_EXTRACTION_FAILED:
            return "Failed to extract game data tables from ROM";
        default:
            return "Unknown ROM loader error";
    }
}

RomStatus rom_assets_load_from_buffer(RomAssets *assets, const uint8_t *data, size_t size,
                                     char *err_msg, size_t err_msg_len) {
    if (!assets || !data) {
        if (err_msg && err_msg_len > 0) {
            snprintf(err_msg, err_msg_len, "Invalid null arguments provided.");
        }
        return ROM_ERR_NULL_PTR;
    }

    memset(assets, 0, sizeof(RomAssets));

    /* Check minimum size (16-byte header + 32KB PRG + 16KB CHR = 49168) */
    if (size < ROM_EXPECTED_SIZE) {
        if (err_msg && err_msg_len > 0) {
            snprintf(err_msg, err_msg_len,
                     "ROM file too small: received %zu bytes, expected at least %u bytes.",
                     size, (unsigned int)ROM_EXPECTED_SIZE);
        }
        return ROM_ERR_TOO_SMALL;
    }

    /* Validate iNES magic header */
    if (data[0] != 'N' || data[1] != 'E' || data[2] != 'S' || data[3] != 0x1A) {
        if (err_msg && err_msg_len > 0) {
            snprintf(err_msg, err_msg_len,
                     "Invalid header: missing iNES magic 'NES\\x1A' (got 0x%02X 0x%02X 0x%02X 0x%02X).",
                     data[0], data[1], data[2], data[3]);
        }
        return ROM_ERR_INVALID_MAGIC;
    }

    /* Validate PRG (2x16KB) and CHR (2x8KB = 4x4KB) units */
    uint8_t prg_units = data[4];
    uint8_t chr_units = data[5];
    if (prg_units != 2 || chr_units != 2) {
        if (err_msg && err_msg_len > 0) {
            snprintf(err_msg, err_msg_len,
                     "Invalid ROM geometry: expected PRG=2 (32KB), CHR=2 (16KB); got PRG=%u, CHR=%u.",
                     (unsigned int)prg_units, (unsigned int)chr_units);
        }
        return ROM_ERR_INVALID_HEADER;
    }

    /* Compute SHA-1 hash over the exact 49168 bytes */
    uint8_t hash[20];
    sha1_compute(data, ROM_EXPECTED_SIZE, hash);
    sha1_to_hex(hash, assets->sha1_hex);

    /* Verify against official Tetris (USA) SHA-1 hash */
    if (strcmp(assets->sha1_hex, ROM_EXPECTED_SHA1) != 0) {
        if (err_msg && err_msg_len > 0) {
            snprintf(err_msg, err_msg_len,
                     "ROM SHA-1 hash mismatch!\n"
                     "  Expected: %s\n"
                     "  Actual:   %s\n"
                     "This file does not match the authentic Tetris (USA).nes commercial release.",
                     ROM_EXPECTED_SHA1, assets->sha1_hex);
        }
        return ROM_ERR_HASH_MISMATCH;
    }

    /* Copy raw memory regions */
    memcpy(assets->prg_rom, data + ROM_HEADER_SIZE, ROM_PRG_SIZE);
    memcpy(assets->chr_rom, data + ROM_HEADER_SIZE + ROM_PRG_SIZE, ROM_CHR_SIZE);
    assets->rom_size = ROM_EXPECTED_SIZE;

    /* Extract Authoritative Palettes (PRG offset = CPU address - 0x8000) */
    memcpy(assets->color_table,    assets->prg_rom + (0x984C - 0x8000), sizeof(assets->color_table));
    memcpy(assets->game_palette,   assets->prg_rom + (0xACF3 - 0x8000), sizeof(assets->game_palette));
    memcpy(assets->menu_palette,   assets->prg_rom + (0xAD2B - 0x8000), sizeof(assets->menu_palette));
    memcpy(assets->ending_palette, assets->prg_rom + (0xAD43 - 0x8000), sizeof(assets->ending_palette));
    memcpy(assets->legal_palette,  assets->prg_rom + (0xAD17 - 0x8000), sizeof(assets->legal_palette));

    /* Extract Authoritative Piece & Movement Data */
    memcpy(assets->orientation_table,     assets->prg_rom + (0x8A9C - 0x8000), sizeof(assets->orientation_table));
    memcpy(assets->rotation_table,        assets->prg_rom + (0x88EE - 0x8000), sizeof(assets->rotation_table));
    memcpy(assets->frames_per_drop_table, assets->prg_rom + (0x898E - 0x8000), sizeof(assets->frames_per_drop_table));
    memcpy(assets->tetrimino_type_table,  assets->prg_rom + (0x993B - 0x8000), sizeof(assets->tetrimino_type_table));
    memcpy(assets->spawn_table,           assets->prg_rom + (0x994E - 0x8000), sizeof(assets->spawn_table));
    memcpy(assets->spawn_orient_table,    assets->prg_rom + (0x9956 - 0x8000), sizeof(assets->spawn_orient_table));
    memcpy(assets->points_table,          assets->prg_rom + (0x9CA5 - 0x8000), sizeof(assets->points_table));

    /* Extract Authoritative Sound & Music Data */
    memcpy(assets->music_selection_table, assets->prg_rom + (0x85D2 - 0x8000), sizeof(assets->music_selection_table));
    memcpy(assets->note_to_wave_table,    assets->prg_rom + (0xEB13 - 0x8000), sizeof(assets->note_to_wave_table));
    memcpy(assets->note_duration_table,   assets->prg_rom + (0xEBAF - 0x8000), sizeof(assets->note_duration_table));
    memcpy(assets->sfx_slot0_init_table,  assets->prg_rom + (0xE009 - 0x8000), sizeof(assets->sfx_slot0_init_table));
    memcpy(assets->sfx_slot1_init_table,  assets->prg_rom + (0xE015 - 0x8000), sizeof(assets->sfx_slot1_init_table));
    memcpy(assets->sfx_slot2_init_table,  assets->prg_rom + (0xE045 - 0x8000), sizeof(assets->sfx_slot2_init_table));
    memcpy(assets->sfx_slot3_init_table,  assets->prg_rom + (0xE03D - 0x8000), sizeof(assets->sfx_slot3_init_table));

    /* Extract Authoritative HUD, Text, and Nametable Data */
    memcpy(assets->high_score_char_to_tile, assets->prg_rom + (0xA08C - 0x8000), sizeof(assets->high_score_char_to_tile));
    memcpy(assets->default_high_scores,     assets->prg_rom + (0xAD67 - 0x8000), sizeof(assets->default_high_scores));
    memcpy(assets->game_nametable,          assets->prg_rom + (0xBF3C - 0x8000), sizeof(assets->game_nametable));

    assets->is_loaded = true;

    if (err_msg && err_msg_len > 0) {
        snprintf(err_msg, err_msg_len, "ROM validated successfully (SHA-1: %s).", assets->sha1_hex);
    }

    return ROM_STATUS_OK;
}

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>

EM_JS(int, js_load_host_file_in_node, (const char *path_cstr), {
    if (typeof process !== 'undefined' && process.release && process.release.name === 'node') {
        try {
            var fs = require('fs');
            var path = require('path');
            var p = UTF8ToString(path_cstr);
            if (fs.existsSync(p)) {
                var data = fs.readFileSync(p);
                // Ensure parent directory exists in virtual FS if needed
                var dir = path.dirname(p);
                if (dir && dir !== '.' && dir !== '/') {
                    try { FS.mkdirTree(dir); } catch(e) {}
                }
                FS.writeFile(p, data);
                return 1;
            }
        } catch(e) {}
    }
    return 0;
});
#endif

RomStatus rom_assets_load_from_file(RomAssets *assets, const char *filepath,
                                   char *err_msg, size_t err_msg_len) {
    if (!filepath || filepath[0] == '\0') {
        if (err_msg && err_msg_len > 0) {
            snprintf(err_msg, err_msg_len, "No ROM file path provided.");
        }
        return ROM_ERR_FILE_NOT_FOUND;
    }

#if defined(__EMSCRIPTEN__)
    js_load_host_file_in_node(filepath);
#endif

    FILE *f = fopen(filepath, "rb");
    if (!f) {
        if (err_msg && err_msg_len > 0) {
            snprintf(err_msg, err_msg_len, "Cannot open ROM file '%s'.", filepath);
        }
        return ROM_ERR_FILE_NOT_FOUND;
    }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (sz < (long)ROM_EXPECTED_SIZE) {
        fclose(f);
        if (err_msg && err_msg_len > 0) {
            snprintf(err_msg, err_msg_len,
                     "ROM file '%s' is too small: %ld bytes (expected %u).",
                     filepath, sz, (unsigned int)ROM_EXPECTED_SIZE);
        }
        return ROM_ERR_TOO_SMALL;
    }

    uint8_t *buf = (uint8_t *)malloc((size_t)sz);
    if (!buf) {
        fclose(f);
        if (err_msg && err_msg_len > 0) {
            snprintf(err_msg, err_msg_len, "Memory allocation failure reading '%s'.", filepath);
        }
        return ROM_ERR_READ_FAILED;
    }

    size_t read_bytes = fread(buf, 1, (size_t)sz, f);
    fclose(f);

    if (read_bytes != (size_t)sz) {
        free(buf);
        if (err_msg && err_msg_len > 0) {
            snprintf(err_msg, err_msg_len, "Failed to read complete ROM file '%s'.", filepath);
        }
        return ROM_ERR_READ_FAILED;
    }

    RomStatus st = rom_assets_load_from_buffer(assets, buf, (size_t)sz, err_msg, err_msg_len);
    free(buf);
    return st;
}

RomStatus rom_assets_find_and_load(RomAssets *assets, const char *custom_path,
                                  char *err_msg, size_t err_msg_len) {
    if (custom_path && custom_path[0] != '\0') {
        return rom_assets_load_from_file(assets, custom_path, err_msg, err_msg_len);
    }

    const char *env_path = getenv("TDECOMP_ROM_PATH");
    if (env_path && env_path[0] != '\0') {
        RomStatus st = rom_assets_load_from_file(assets, env_path, err_msg, err_msg_len);
        if (st == ROM_STATUS_OK) return st;
    }

    const char *candidate_paths[] = {
        "Tetris (USA).nes",
        "../Tetris (USA).nes",
        "../../Tetris (USA).nes",
        "assets/Tetris (USA).nes",
        "../assets/Tetris (USA).nes",
        "TetrisNESDisasm/tetris.nes",
        "../TetrisNESDisasm/tetris.nes",
        NULL
    };

    for (int i = 0; candidate_paths[i] != NULL; i++) {
        RomStatus st = rom_assets_load_from_file(assets, candidate_paths[i], err_msg, err_msg_len);
        if (st == ROM_STATUS_OK) {
            return st;
        }
    }

    if (err_msg && err_msg_len > 0) {
        snprintf(err_msg, err_msg_len,
                 "Tetris (USA).nes not found in standard search paths.\n"
                 "Please supply the original ROM via --rom <path> or set TDECOMP_ROM_PATH.");
    }
    return ROM_ERR_FILE_NOT_FOUND;
}

void rom_decode_chr_tile(const uint8_t *chr_bank, uint8_t tile_idx, uint8_t pixels[8][8]) {
    if (!chr_bank) return;
    const uint8_t *tile_data = chr_bank + ((size_t)tile_idx * 16);

    for (int y = 0; y < 8; y++) {
        uint8_t p0 = tile_data[y];
        uint8_t p1 = tile_data[y + 8];
        for (int x = 0; x < 8; x++) {
            uint8_t bit0 = (uint8_t)((p0 >> (7 - x)) & 1);
            uint8_t bit1 = (uint8_t)((p1 >> (7 - x)) & 1);
            pixels[y][x] = (uint8_t)((bit1 << 1) | bit0);
        }
    }
}

const uint8_t *rom_get_chr_bank(const RomAssets *assets, uint8_t bank_idx) {
    if (!assets || bank_idx >= ROM_CHR_BANKS) return NULL;
    return assets->chr_rom + ((size_t)bank_idx * ROM_CHR_BANK_SIZE);
}
