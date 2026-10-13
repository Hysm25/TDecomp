#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "rom_assets.h"
#include "constants/gameplay.h"

static void test_sha1_implementation(void) {
    /* Test vector: empty string */
    uint8_t hash[20];
    char hex[41];
    sha1_compute((const uint8_t *)"", 0, hash);
    sha1_to_hex(hash, hex);
    assert(strcmp(hex, "da39a3ee5e6b4b0d3255bfef95601890afd80709") == 0);

    /* Test vector: "abc" */
    sha1_compute((const uint8_t *)"abc", 3, hash);
    sha1_to_hex(hash, hex);
    assert(strcmp(hex, "a9993e364706816aba3e25717850c26c9cd0d89d") == 0);

    /* Test vector: "message digest" */
    sha1_compute((const uint8_t *)"message digest", 14, hash);
    sha1_to_hex(hash, hex);
    assert(strcmp(hex, "c12252ceda8be8994d5fa0290a47231c1d16aae3") == 0);

    printf("  [PASS] test_sha1_implementation (RFC 3174 test vectors)\n");
}

static void test_rom_validation_rejections(void) {
    RomAssets assets;
    char err[256];

    /* 1. Null arguments */
    assert(rom_assets_load_from_buffer(NULL, NULL, 0, err, sizeof(err)) == ROM_ERR_NULL_PTR);

    /* 2. File too small */
    uint8_t small_buf[100];
    memset(small_buf, 0, sizeof(small_buf));
    assert(rom_assets_load_from_buffer(&assets, small_buf, sizeof(small_buf), err, sizeof(err)) == ROM_ERR_TOO_SMALL);
    assert(strstr(err, "too small") != NULL);

    /* 3. Corrupted iNES magic */
    uint8_t dummy_rom[ROM_EXPECTED_SIZE];
    memset(dummy_rom, 0, sizeof(dummy_rom));
    memcpy(dummy_rom, "CORR", 4);
    assert(rom_assets_load_from_buffer(&assets, dummy_rom, sizeof(dummy_rom), err, sizeof(err)) == ROM_ERR_INVALID_MAGIC);
    assert(strstr(err, "magic") != NULL);

    /* 4. Invalid PRG / CHR units */
    memcpy(dummy_rom, "NES\x1A", 4);
    dummy_rom[4] = 1; /* 1 instead of 2 */
    dummy_rom[5] = 2;
    assert(rom_assets_load_from_buffer(&assets, dummy_rom, sizeof(dummy_rom), err, sizeof(err)) == ROM_ERR_INVALID_HEADER);
    assert(strstr(err, "geometry") != NULL);

    /* 5. Hash mismatch */
    dummy_rom[4] = 2;
    dummy_rom[5] = 2;
    assert(rom_assets_load_from_buffer(&assets, dummy_rom, sizeof(dummy_rom), err, sizeof(err)) == ROM_ERR_HASH_MISMATCH);
    assert(strstr(err, "mismatch") != NULL);
    assert(strstr(err, ROM_EXPECTED_SHA1) != NULL);

    printf("  [PASS] test_rom_validation_rejections (strict format & hash rejection)\n");
}

static void test_rom_extraction_authoritative(void) {
    RomAssets assets;
    char err[256];

    RomStatus st = rom_assets_find_and_load(&assets, NULL, err, sizeof(err));
    assert(st == ROM_STATUS_OK);
    assert(assets.is_loaded);
    assert(strcmp(assets.sha1_hex, ROM_EXPECTED_SHA1) == 0);
    assert(assets.rom_size == ROM_EXPECTED_SIZE);

    /* Test extracted color table (Level 0: 0x0F, 0x30, 0x21, 0x12) */
    assert(assets.color_table[0] == 0x0F);
    assert(assets.color_table[1] == 0x30);
    assert(assets.color_table[2] == 0x21);
    assert(assets.color_table[3] == 0x12);

    /* Test extracted orientation table (first mino of orientation 0) */
    assert(assets.orientation_table[0] == 0x00);
    assert(assets.orientation_table[1] == 0x7B); /* TILE_1 */
    assert(assets.orientation_table[2] == 0xFF);

    /* Test extracted rotation table */
    assert(assets.rotation_table[0] == 0x03);
    assert(assets.rotation_table[1] == 0x01);

    /* Test extracted frames per drop (Level 0 gravity: 48 frames / $30) */
    assert(assets.frames_per_drop_table[0] == 48);

    /* Test CHR ROM tile decoding for TILE_1 (0x7B) */
    const uint8_t *game_bank = rom_get_chr_bank(&assets, CHR_GAME);
    assert(game_bank != NULL);

    uint8_t pixels[8][8];
    rom_decode_chr_tile(game_bank, TILE_1, pixels);
    /* In TILE_1, pixel (0,0) is shine color 1 */
    assert(pixels[0][0] == 1);
    assert(pixels[0][1] == 3);

    /* Test CHR ROM tile decoding for TILE_2 (0x7C) */
    rom_decode_chr_tile(game_bank, TILE_2, pixels);
    assert(pixels[0][0] == 1);
    assert(pixels[0][1] == 2);

    printf("  [PASS] test_rom_extraction_authoritative (SHA-1 match & table verification)\n");
}

int test_rom_assets_subsystem(void) {
    printf("--- Testing ROM Assets & Extraction Subsystem ---\n");
    test_sha1_implementation();
    test_rom_validation_rejections();
    test_rom_extraction_authoritative();
    return 0;
}
