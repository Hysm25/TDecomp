#ifndef TDECOMP_ROM_ASSETS_H
#define TDECOMP_ROM_ASSETS_H

#include "common.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Authoritative ROM dimensions */
#define ROM_EXPECTED_SIZE       49168   /* 16-byte header + 32KB PRG + 16KB CHR */
#define ROM_HEADER_SIZE         16
#define ROM_PRG_SIZE            0x8000  /* 32768 bytes */
#define ROM_CHR_SIZE            0x4000  /* 16384 bytes */
#define ROM_CHR_BANK_SIZE       0x1000  /* 4096 bytes (256 tiles) */
#define ROM_CHR_BANKS           4
#define ROM_TILES_PER_BANK      256
#define ROM_TILE_BYTES          16      /* 8x8 pixels, 2 bitplanes */

/* Expected SHA-1 hash for Tetris (USA).nes */
#define ROM_EXPECTED_SHA1       "3026d28b63d94c921fe58364f8b0659d10b5a0ac"

/* ROM Status & Error Codes */
typedef enum RomStatus {
    ROM_STATUS_OK = 0,
    ROM_ERR_NULL_PTR = -1,
    ROM_ERR_FILE_NOT_FOUND = -2,
    ROM_ERR_READ_FAILED = -3,
    ROM_ERR_TOO_SMALL = -4,
    ROM_ERR_INVALID_MAGIC = -5,
    ROM_ERR_INVALID_HEADER = -6,
    ROM_ERR_HASH_MISMATCH = -7,
    ROM_ERR_EXTRACTION_FAILED = -8
} RomStatus;

/* Extracted game assets and data bundle */
typedef struct RomAssets {
    bool is_loaded;
    char sha1_hex[41];
    size_t rom_size;

    /* Raw ROM memory images */
    uint8_t prg_rom[ROM_PRG_SIZE];
    uint8_t chr_rom[ROM_CHR_SIZE];

    /* Palettes extracted from PRG ROM */
    uint8_t color_table[40];            /* 10 levels x 4 colors ($984C) */
    uint8_t game_palette[36];           /* In-game background & sprites ($ACF3) */
    uint8_t menu_palette[24];           /* Menu & title screens ($AD2B) */
    uint8_t ending_palette[36];         /* Ending screens ($AD43) */
    uint8_t legal_palette[20];          /* Legal disclaimer screen ($AD17) */

    /* Piece & gameplay lookup tables */
    uint8_t orientation_table[228];     /* 19 orientations x 4 minos x 3 bytes ($8A9C) */
    uint8_t rotation_table[38];         /* Rotation transitions CCW/CW ($88EE) */
    uint8_t frames_per_drop_table[29];  /* Gravity drop speed ($898E) */
    uint8_t tetrimino_type_table[7];    /* Base piece types ($993B) */
    uint8_t spawn_table[7];             /* Spawn positions ($994E) */
    uint8_t spawn_orient_table[7];      /* Spawn orientations ($9956) */
    uint8_t points_table[8];            /* Line clear scoring ($9CA5) */

    /* Sound and music tables */
    uint8_t music_selection_table[8];   /* Track pointers ($85D2) */
    uint8_t note_to_wave_table[156];    /* Note frequencies ($EB13) */
    uint8_t note_duration_table[116];   /* Note durations ($EBAF) */
    uint8_t sfx_slot0_init_table[6];    /* SFX slot 0 init ($E009) */
    uint8_t sfx_slot1_init_table[20];   /* SFX slot 1 init ($E015) */
    uint8_t sfx_slot2_init_table[189];  /* SFX slot 2 init ($E045) */
    uint8_t sfx_slot3_init_table[4];    /* SFX slot 3 init ($E03D) */

    /* High score & text */
    uint8_t high_score_char_to_tile[48];/* Character mapping ($A08C) */
    uint8_t default_high_scores[24];    /* Factory high scores ($AD67) */
    uint8_t game_nametable[1121];       /* Playfield background layout ($BF3C) */
} RomAssets;

/*
 * Validates iNES header and SHA-1 hash of provided buffer, then extracts
 * all graphics, palettes, audio, and gameplay tables.
 * Returns ROM_STATUS_OK on success, or error status code on failure.
 * If err_msg is non-NULL, writes a human-readable explanation of any failure.
 */
RomStatus rom_assets_load_from_buffer(RomAssets *assets, const uint8_t *data, size_t size,
                                     char *err_msg, size_t err_msg_len);

/*
 * Reads ROM from the specified file path, validates header and SHA-1,
 * and extracts all assets.
 */
RomStatus rom_assets_load_from_file(RomAssets *assets, const char *filepath,
                                   char *err_msg, size_t err_msg_len);

/*
 * Searches standard candidate paths (custom path, TDECOMP_ROM_PATH env,
 * current directory, parent directory, assets/) for Tetris (USA).nes,
 * validates header and SHA-1, and extracts all assets.
 */
RomStatus rom_assets_find_and_load(RomAssets *assets, const char *custom_path,
                                  char *err_msg, size_t err_msg_len);

/*
 * Decodes an 8x8 2bpp NES CHR tile from a 4KB bank into an 8x8 array of
 * 2-bit color index values (0, 1, 2, 3).
 */
void rom_decode_chr_tile(const uint8_t *chr_bank, uint8_t tile_idx, uint8_t pixels[8][8]);

/*
 * Returns a pointer to the start of the given CHR bank (0..3).
 * Bank 0 = CHR_TITLE_MENU
 * Bank 1 = CHR_TYPEB_ENDING
 * Bank 2 = CHR_TYPEA_ENDING
 * Bank 3 = CHR_GAME
 */
const uint8_t *rom_get_chr_bank(const RomAssets *assets, uint8_t bank_idx);

/*
 * Returns a human-readable status string for RomStatus error codes.
 */
const char *rom_status_to_string(RomStatus status);

/*
 * Standalone SHA-1 computation utilities (RFC 3174 compliant).
 */
void sha1_compute(const uint8_t *data, size_t len, uint8_t hash[20]);
void sha1_to_hex(const uint8_t hash[20], char hex_out[41]);

#ifdef __cplusplus
}
#endif

#endif /* TDECOMP_ROM_ASSETS_H */
