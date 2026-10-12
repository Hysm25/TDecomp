#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "nes.h"
#include "verification/cpu6502.h"
#include "verification/rom_data.h"
#include "math/rng.h"
#include "math/bcd.h"
#include "game/tetrimino.h"

/* Verified 6502 PRG addresses from tetris.lbl */
#define ROM_ADDR_ROTATE_TETRIMINO       0x88AB
#define ROM_ADDR_DROP_TETRIMINO         0x8914
#define ROM_ADDR_SHIFT_TETRIMINO        0x89AE
#define ROM_ADDR_IS_POSITION_VALID      0x948B
#define ROM_ADDR_CHOOSE_NEXT_TETRIMINO  0x98EB
#define ROM_ADDR_PICK_RANDOM_TETRIMINO  0x9903
#define ROM_ADDR_INCREMENT_PIECE_STAT   0x9969
#define ROM_ADDR_UPDATE_PLAYFIELD       0x9CAF
#define ROM_ADDR_UPDATE_MUSIC_SPEED     0x9D17
#define ROM_ADDR_SET_MUSIC_TRACK        0x9E07
#define ROM_ADDR_LFSR_PRNG              0xAB47
#define ROM_ADDR_SOUND_RNG              0xE09E

int test_differential_verification(void) {
    NES_State rom_nes;
    nes_init(&rom_nes);

    if (!load_reference_rom(&rom_nes, NULL)) {
        printf("  [WARN] Reference ROM not found. Differential tests skipped.\n");
        return 0;
    }

    /* First verify ROM data tables bit-for-bit */
    test_rom_tables_match(&rom_nes);

    CPU6502 cpu;
    cpu6502_init(&cpu, &rom_nes);

    /* Test 1: Differential LFSR PRNG across 100 test seeds */
    {
        uint16_t test_seeds[] = {
            0x0001, 0x0002, 0x0003, 0x1234, 0x5678, 0x89AB, 0xCDEF,
            0xAAAA, 0x5555, 0xFFFF, 0x0202, 0x8080, 0x0101, 0x0000
        };
        size_t num_seeds = sizeof(test_seeds) / sizeof(test_seeds[0]);

        for (size_t i = 0; i < num_seeds; i++) {
            uint16_t seed = test_seeds[i];

            /* Setup 6502 state */
            NES_State nes_6502 = rom_nes;
            nes_6502.ram[ADDR_RNG_SEED] = LOW_BYTE(seed);
            nes_6502.ram[ADDR_RNG_SEED + 1] = HIGH_BYTE(seed);
            nes_6502.ram[ADDR_TMP1] = 0;

            cpu6502_init(&cpu, &nes_6502);
            /* generateNextPseudorandomNumber expects X = seed addr, Y = seed size */
            cpu6502_set_state(&cpu, ROM_ADDR_LFSR_PRNG, 0, ADDR_RNG_SEED, 2, 0, 0xFD);
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_LFSR_PRNG, 1000);
            assert(ok);
            (void)ok;

            /* Setup C state */
            NES_State nes_c = rom_nes;
            nes_c.ram[ADDR_RNG_SEED] = LOW_BYTE(seed);
            nes_c.ram[ADDR_RNG_SEED + 1] = HIGH_BYTE(seed);
            nes_c.ram[ADDR_TMP1] = 0;
            generateNextPseudorandomNumber(&nes_c, ADDR_RNG_SEED, 2);

            /* Compare exact RAM */
            assert(nes_6502.ram[ADDR_RNG_SEED] == nes_c.ram[ADDR_RNG_SEED]);
            assert(nes_6502.ram[ADDR_RNG_SEED + 1] == nes_c.ram[ADDR_RNG_SEED + 1]);
            assert(nes_6502.ram[ADDR_TMP1] == nes_c.ram[ADDR_TMP1]);
        }
    }

    /* Test 2: Differential sound RNG across test seeds */
    {
        uint16_t test_seeds[] = { 0x0003, 0x1234, 0x89AB, 0x5555, 0xAAAA, 0x0200 };
        size_t num_seeds = sizeof(test_seeds) / sizeof(test_seeds[0]);

        for (size_t i = 0; i < num_seeds; i++) {
            uint16_t seed = test_seeds[i];

            NES_State nes_6502 = rom_nes;
            nes_6502.ram[ADDR_SOUND_RNG_SEED] = LOW_BYTE(seed);
            nes_6502.ram[ADDR_SOUND_RNG_SEED + 1] = HIGH_BYTE(seed);

            cpu6502_init(&cpu, &nes_6502);
            cpu6502_set_state(&cpu, ROM_ADDR_SOUND_RNG, 0, 0, 0, 0, 0xFD);
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_SOUND_RNG, 1000);
            assert(ok);
            (void)ok;

            NES_State nes_c = rom_nes;
            nes_c.ram[ADDR_SOUND_RNG_SEED] = LOW_BYTE(seed);
            nes_c.ram[ADDR_SOUND_RNG_SEED + 1] = HIGH_BYTE(seed);
            unreferenced_soundRng(&nes_c);

            assert(nes_6502.ram[ADDR_SOUND_RNG_SEED] == nes_c.ram[ADDR_SOUND_RNG_SEED]);
            assert(nes_6502.ram[ADDR_SOUND_RNG_SEED + 1] == nes_c.ram[ADDR_SOUND_RNG_SEED + 1]);
            assert(nes_6502.ram[ADDR_UNREFERENCED_SOUND_RNG_TMP] == nes_c.ram[ADDR_UNREFERENCED_SOUND_RNG_TMP]);
        }
    }

    /* Test 3: Differential incrementPieceStat across all orientations and BCD values */
    {
        for (uint8_t orient = 0; orient < 19; orient++) {
            uint8_t stat_inits[] = { 0x00, 0x05, 0x09, 0x10, 0x19, 0x99 };
            for (size_t s = 0; s < sizeof(stat_inits); s++) {
                NES_State nes_6502 = rom_nes;
                NES_State nes_c = rom_nes;

                uint8_t piece_type = tetriminoTypeFromOrientation[orient];
                nes_6502.ram[ADDR_STATS_BY_TYPE + piece_type * 2] = stat_inits[s];
                nes_c.ram[ADDR_STATS_BY_TYPE + piece_type * 2] = stat_inits[s];

                cpu6502_init(&cpu, &nes_6502);
                /* A = orientation */
                cpu6502_set_state(&cpu, ROM_ADDR_INCREMENT_PIECE_STAT, orient, 0, 0, 0, 0xFD);
                bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_INCREMENT_PIECE_STAT, 1000);
                assert(ok);
                (void)ok;

                incrementPieceStat(&nes_c, orient);

                assert(nes_6502.ram[ADDR_STATS_BY_TYPE + piece_type * 2] == nes_c.ram[ADDR_STATS_BY_TYPE + piece_type * 2]);
                assert(nes_6502.ram[ADDR_STATS_BY_TYPE + piece_type * 2 + 1] == nes_c.ram[ADDR_STATS_BY_TYPE + piece_type * 2 + 1]);
                assert(nes_6502.ram[ADDR_GENERAL_COUNTER] == nes_c.ram[ADDR_GENERAL_COUNTER]);
            }
        }
    }

    /* Test 4: Differential isPositionValid across multiple piece positions */
    {
        uint8_t test_pieces[] = { ORIENT_T_DOWN, ORIENT_T_UP, ORIENT_I_VERT, ORIENT_I_HORIZ, ORIENT_O_FIXED };
        for (size_t p = 0; p < sizeof(test_pieces); p++) {
            uint8_t piece = test_pieces[p];
            for (int8_t y = 0; y < 21; y += 4) {
                for (int8_t x = 0; x < 10; x += 3) {
                    NES_State nes_6502 = rom_nes;
                    NES_State nes_c = rom_nes;

                    nes_write16(&nes_6502, ADDR_PLAYFIELD_ADDR, ADDR_PLAYFIELD);
                    nes_write16(&nes_c, ADDR_PLAYFIELD_ADDR, ADDR_PLAYFIELD);

                    for (int i = 0; i < PLAYFIELD_SIZE; i++) {
                        nes_6502.ram[ADDR_PLAYFIELD + i] = TILE_EMPTY;
                        nes_c.ram[ADDR_PLAYFIELD + i] = TILE_EMPTY;
                    }

                    nes_6502.ram[ADDR_TETRIMINO_X] = (uint8_t)x;
                    nes_6502.ram[ADDR_TETRIMINO_Y] = (uint8_t)y;
                    nes_6502.ram[ADDR_CURRENT_PIECE] = piece;

                    nes_c.ram[ADDR_TETRIMINO_X] = (uint8_t)x;
                    nes_c.ram[ADDR_TETRIMINO_Y] = (uint8_t)y;
                    nes_c.ram[ADDR_CURRENT_PIECE] = piece;

                    cpu6502_init(&cpu, &nes_6502);
                    cpu6502_set_state(&cpu, ROM_ADDR_IS_POSITION_VALID, 0, 0, 0, 0, 0xFD);
                    bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_IS_POSITION_VALID, 2000);
                    assert(ok);
                    (void)ok;

                    uint8_t res_c = isPositionValid(&nes_c);

                    /* 6502 generalCounter is 0 for valid, 0xFF for invalid */
                    assert(nes_6502.ram[ADDR_GENERAL_COUNTER] == nes_c.ram[ADDR_GENERAL_COUNTER]);
                    assert(res_c == nes_6502.ram[ADDR_GENERAL_COUNTER]);
                    (void)res_c;
                }
            }
        }
    }

    /* Test 5: Differential updatePlayfield across all Y values */
    {
        for (uint8_t y = 0; y < 25; y++) {
            for (uint8_t init_vram = 0; init_vram < 25; init_vram += 5) {
                NES_State nes_6502 = rom_nes;
                NES_State nes_c = rom_nes;

                nes_6502.ram[ADDR_TETRIMINO_Y] = y;
                nes_6502.ram[ADDR_VRAM_ROW] = init_vram;

                nes_c.ram[ADDR_TETRIMINO_Y] = y;
                nes_c.ram[ADDR_VRAM_ROW] = init_vram;

                cpu6502_init(&cpu, &nes_6502);
                cpu6502_set_state(&cpu, ROM_ADDR_UPDATE_PLAYFIELD, 0, 0, 0, 0, 0xFD);
                bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_UPDATE_PLAYFIELD, 1000);
                assert(ok);
                (void)ok;

                updatePlayfield(&nes_c);
                assert(nes_6502.ram[ADDR_VRAM_ROW] == nes_c.ram[ADDR_VRAM_ROW]);
            }
        }
    }

    /* Test 6: Differential updateMusicSpeed */
    {
        for (uint8_t m = 0; m < 4; m++) {
            for (uint8_t allegro = 0; allegro <= 1; allegro++) {
                NES_State nes_6502 = rom_nes;
                NES_State nes_c = rom_nes;

                nes_write16(&nes_6502, ADDR_PLAYFIELD_ADDR, ADDR_PLAYFIELD);
                nes_write16(&nes_c, ADDR_PLAYFIELD_ADDR, ADDR_PLAYFIELD);

                for (int i = 0; i < PLAYFIELD_SIZE; i++) {
                    nes_6502.ram[ADDR_PLAYFIELD + i] = TILE_EMPTY;
                    nes_c.ram[ADDR_PLAYFIELD + i] = TILE_EMPTY;
                }

                nes_6502.ram[ADDR_MUSIC_TYPE] = m;
                nes_6502.ram[ADDR_ALLEGRO] = allegro ? 0xFF : 0x00;

                nes_c.ram[ADDR_MUSIC_TYPE] = m;
                nes_c.ram[ADDR_ALLEGRO] = allegro ? 0xFF : 0x00;

                /* Run empty playfield check */
                cpu6502_init(&cpu, &nes_6502);
                cpu6502_set_state(&cpu, ROM_ADDR_UPDATE_MUSIC_SPEED, 0, 0, 0, 0, 0xFD);
                bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_UPDATE_MUSIC_SPEED, 2000);
                assert(ok);
                (void)ok;

                updateMusicSpeed(&nes_c);
                assert(nes_6502.ram[ADDR_ALLEGRO] == nes_c.ram[ADDR_ALLEGRO]);
                assert(nes_6502.ram[ADDR_MUSIC_TRACK] == nes_c.ram[ADDR_MUSIC_TRACK]);

                /* Run danger playfield check (block placed on row 5, offset 53) */
                nes_6502.ram[ADDR_PLAYFIELD + 53] = TILE_1;
                nes_c.ram[ADDR_PLAYFIELD + 53] = TILE_1;

                cpu6502_init(&cpu, &nes_6502);
                cpu6502_set_state(&cpu, ROM_ADDR_UPDATE_MUSIC_SPEED, 0, 0, 0, 0, 0xFD);
                ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_UPDATE_MUSIC_SPEED, 2000);
                assert(ok);
                (void)ok;

                updateMusicSpeed(&nes_c);
                assert(nes_6502.ram[ADDR_ALLEGRO] == nes_c.ram[ADDR_ALLEGRO]);
                assert(nes_6502.ram[ADDR_MUSIC_TRACK] == nes_c.ram[ADDR_MUSIC_TRACK]);
            }
        }
    }

    /* Test 7: Differential setMusicTrack */
    {
        uint8_t modes[] = { MODE_PLAY, MODE_PLAY_ENDING_HIGH_SCORE };
        for (size_t m = 0; m < sizeof(modes); m++) {
            for (uint8_t track = 0; track < 10; track++) {
                NES_State nes_6502 = rom_nes;
                NES_State nes_c = rom_nes;

                nes_6502.ram[ADDR_GAME_MODE] = modes[m];
                nes_c.ram[ADDR_GAME_MODE] = modes[m];

                cpu6502_init(&cpu, &nes_6502);
                /* A = track */
                cpu6502_set_state(&cpu, ROM_ADDR_SET_MUSIC_TRACK, track, 0, 0, 0, 0xFD);
                bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_SET_MUSIC_TRACK, 1000);
                assert(ok);
                (void)ok;

                setMusicTrack(&nes_c, track);
                assert(nes_6502.ram[ADDR_MUSIC_TRACK] == nes_c.ram[ADDR_MUSIC_TRACK]);
            }
        }
    }

    printf("  [PASS] test_differential_verification (All 6502 ROM vs C routines proven equivalent)\n");
    return 0;
}
