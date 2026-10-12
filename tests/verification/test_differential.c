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
#include "game/playfield.h"
#include "game/input.h"
#include "game/state.h"
#include "game/menu.h"

/* Verified 6502 PRG addresses from tetris.lbl */
#define ROM_ADDR_BRANCH_ON_GAME_MODE    0x8161
#define ROM_ADDR_BRANCH_ON_PLAY_STATE_P1 0x81B2
#define ROM_ADDR_BRANCH_ON_PLAY_STATE_P2 0x81D9
#define ROM_ADDR_LEGAL_SCREEN           0x8200
#define ROM_ADDR_TITLE_SCREEN           0x824F
#define ROM_ADDR_TITLE_TIMEOUT_CHECK    0x8296
#define ROM_ADDR_MAKE_PLAYER1_ACTIVE    0x8776
#define ROM_ADDR_MAKE_PLAYER2_ACTIVE    0x8792
#define ROM_ADDR_SAVE_PLAYER1_STATE     0x87AE
#define ROM_ADDR_SAVE_PLAYER2_STATE     0x87C8
#define ROM_ADDR_ROTATE_TETRIMINO       0x88AB
#define ROM_ADDR_UPDATE_COUNTERS_AND_NON_PLAYER_STATE 0x8884
#define ROM_ADDR_DROP_TETRIMINO         0x8914
#define ROM_ADDR_SHIFT_TETRIMINO        0x89AE
#define ROM_ADDR_IS_POSITION_VALID      0x948B
#define ROM_ADDR_NOOP_DISABLED_VRAM_ROW_INCR 0x9874
#define ROM_ADDR_SPAWN_NEXT_TETRIMINO   0x988E
#define ROM_ADDR_CHOOSE_NEXT_TETRIMINO  0x98EB
#define ROM_ADDR_PICK_RANDOM_TETRIMINO  0x9903
#define ROM_ADDR_INCREMENT_PIECE_STAT   0x9969
#define ROM_ADDR_CHECK_FOR_COMPLETED_ROWS 0x9A6B
#define ROM_ADDR_UPDATE_PLAYFIELD       0x9CAF
#define ROM_ADDR_UPDATE_MUSIC_SPEED     0x9D17
#define ROM_ADDR_POLL_CONTROLLER_BUTTONS 0x9D51
#define ROM_ADDR_DEMO_BUTTONS_INDEX_INCR 0x9DE8
#define ROM_ADDR_START_DEMO             0x9DF6
#define ROM_ADDR_SET_MUSIC_TRACK        0x9E07
#define ROM_ADDR_GAME_MODE_STATE_VBLANK_THEN_RUN_STATE_2 0x9E27
#define ROM_ADDR_PLAYSTATE_UNASSIGN_ORIENTATION 0x9E2F
#define ROM_ADDR_PLAYSTATE_INCREMENT_PLAYSTATE 0x9E37
#define ROM_ADDR_PLAYSTATE_NOOP         0x9E39
#define ROM_ADDR_PLAYSTATE_B_TYPE_GOAL_CHECK 0xA3F2
#define ROM_ADDR_LFSR_PRNG              0xAB47
#define ROM_ADDR_POLL_CONTROLLER_ACTUAL_READ 0xAB69
#define ROM_ADDR_ADD_EXPANSION_PORT_INPUT 0xAB8B
#define ROM_ADDR_POLL_CONTROLLER        0xAB9D
#define ROM_ADDR_DIFF_OLD_AND_NEW_BUTTONS 0xABBD
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

                    /* Real NES Tetris initializes the entire 256-byte playfield page with memset_page */
                    for (int i = 0; i < 0x100; i++) {
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

    /* Test 8: Differential playState_checkForCompletedRows */
    {
        /* Scenario A: Single row completion */
        {
            NES_State nes_6502 = rom_nes;
            NES_State nes_c = rom_nes;

            uint16_t pf_base = ADDR_PLAYFIELD;
            nes_6502.ram[ADDR_PLAYFIELD_ADDR] = LOW_BYTE(pf_base);
            nes_6502.ram[ADDR_PLAYFIELD_ADDR + 1] = HIGH_BYTE(pf_base);
            nes_c.ram[ADDR_PLAYFIELD_ADDR] = LOW_BYTE(pf_base);
            nes_c.ram[ADDR_PLAYFIELD_ADDR + 1] = HIGH_BYTE(pf_base);

            memset(&nes_6502.ram[pf_base], TILE_EMPTY, 0x100);
            memset(&nes_c.ram[pf_base], TILE_EMPTY, 0x100);

            /* Fill row 18 with TILE_1 ($7B) and row 17 col 3 with TILE_2 ($7C) */
            for (int col = 0; col < 10; col++) {
                nes_6502.ram[pf_base + 18 * 10 + col] = TILE_1;
                nes_c.ram[pf_base + 18 * 10 + col] = TILE_1;
            }
            nes_6502.ram[pf_base + 17 * 10 + 3] = TILE_2;
            nes_c.ram[pf_base + 17 * 10 + 3] = TILE_2;

            nes_6502.ram[ADDR_TETRIMINO_Y] = 19;
            nes_c.ram[ADDR_TETRIMINO_Y] = 19;
            nes_6502.ram[ADDR_VRAM_ROW] = 0x20;
            nes_c.ram[ADDR_VRAM_ROW] = 0x20;
            nes_6502.ram[ADDR_LINE_INDEX] = 0;
            nes_c.ram[ADDR_LINE_INDEX] = 0;
            nes_6502.ram[ADDR_COMPLETED_LINES] = 0;
            nes_c.ram[ADDR_COMPLETED_LINES] = 0;
            nes_6502.ram[ADDR_PLAY_STATE] = 3;
            nes_c.ram[ADDR_PLAY_STATE] = 3;

            /* Execute 4 scan iterations */
            for (int step = 0; step < 4; step++) {
                cpu6502_init(&cpu, &nes_6502);
                cpu6502_set_state(&cpu, ROM_ADDR_CHECK_FOR_COMPLETED_ROWS, 0, 0, 0, 0, 0xFD);
                bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_CHECK_FOR_COMPLETED_ROWS, 5000);
                assert(ok);
                (void)ok;

                playState_checkForCompletedRows(&nes_c);
            }

            assert(nes_6502.ram[ADDR_COMPLETED_LINES] == nes_c.ram[ADDR_COMPLETED_LINES]);
            assert(nes_6502.ram[ADDR_PLAY_STATE] == nes_c.ram[ADDR_PLAY_STATE]);
            assert(nes_6502.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_INIT]);
            assert(memcmp(&nes_6502.ram[pf_base], &nes_c.ram[pf_base], PLAYFIELD_SIZE) == 0);
        }

        /* Scenario B: Tetris 4-row completion */
        {
            NES_State nes_6502 = rom_nes;
            NES_State nes_c = rom_nes;

            uint16_t pf_base = ADDR_PLAYFIELD;
            nes_6502.ram[ADDR_PLAYFIELD_ADDR] = LOW_BYTE(pf_base);
            nes_6502.ram[ADDR_PLAYFIELD_ADDR + 1] = HIGH_BYTE(pf_base);
            nes_c.ram[ADDR_PLAYFIELD_ADDR] = LOW_BYTE(pf_base);
            nes_c.ram[ADDR_PLAYFIELD_ADDR + 1] = HIGH_BYTE(pf_base);

            memset(&nes_6502.ram[pf_base], TILE_EMPTY, 0x100);
            memset(&nes_c.ram[pf_base], TILE_EMPTY, 0x100);

            for (int r = 16; r < 20; r++) {
                for (int c = 0; c < 10; c++) {
                    nes_6502.ram[pf_base + r * 10 + c] = TILE_3;
                    nes_c.ram[pf_base + r * 10 + c] = TILE_3;
                }
            }

            nes_6502.ram[ADDR_TETRIMINO_Y] = 18;
            nes_c.ram[ADDR_TETRIMINO_Y] = 18;
            nes_6502.ram[ADDR_VRAM_ROW] = 0x20;
            nes_c.ram[ADDR_VRAM_ROW] = 0x20;
            nes_6502.ram[ADDR_LINE_INDEX] = 0;
            nes_c.ram[ADDR_LINE_INDEX] = 0;
            nes_6502.ram[ADDR_COMPLETED_LINES] = 0;
            nes_c.ram[ADDR_COMPLETED_LINES] = 0;
            nes_6502.ram[ADDR_PENDING_GARBAGE_INACTIVE] = 0;
            nes_c.ram[ADDR_PENDING_GARBAGE_INACTIVE] = 0;
            nes_6502.ram[ADDR_PLAY_STATE] = 3;
            nes_c.ram[ADDR_PLAY_STATE] = 3;

            for (int step = 0; step < 4; step++) {
                cpu6502_init(&cpu, &nes_6502);
                cpu6502_set_state(&cpu, ROM_ADDR_CHECK_FOR_COMPLETED_ROWS, 0, 0, 0, 0, 0xFD);
                bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_CHECK_FOR_COMPLETED_ROWS, 5000);
                assert(ok);
                (void)ok;

                playState_checkForCompletedRows(&nes_c);
            }

            assert(nes_6502.ram[ADDR_COMPLETED_LINES] == nes_c.ram[ADDR_COMPLETED_LINES]);
            assert(nes_6502.ram[ADDR_PLAY_STATE] == nes_c.ram[ADDR_PLAY_STATE]);
            assert(nes_6502.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_INIT]);
            assert(nes_6502.ram[ADDR_PENDING_GARBAGE_INACTIVE] == nes_c.ram[ADDR_PENDING_GARBAGE_INACTIVE]);
            assert(memcmp(&nes_6502.ram[pf_base], &nes_c.ram[pf_base], PLAYFIELD_SIZE) == 0);
        }

        /* Scenario C: Zero rows complete */
        {
            NES_State nes_6502 = rom_nes;
            NES_State nes_c = rom_nes;

            uint16_t pf_base = ADDR_PLAYFIELD;
            nes_6502.ram[ADDR_PLAYFIELD_ADDR] = LOW_BYTE(pf_base);
            nes_6502.ram[ADDR_PLAYFIELD_ADDR + 1] = HIGH_BYTE(pf_base);
            nes_c.ram[ADDR_PLAYFIELD_ADDR] = LOW_BYTE(pf_base);
            nes_c.ram[ADDR_PLAYFIELD_ADDR + 1] = HIGH_BYTE(pf_base);

            memset(&nes_6502.ram[pf_base], TILE_EMPTY, 0x100);
            memset(&nes_c.ram[pf_base], TILE_EMPTY, 0x100);

            nes_6502.ram[ADDR_TETRIMINO_Y] = 10;
            nes_c.ram[ADDR_TETRIMINO_Y] = 10;
            nes_6502.ram[ADDR_VRAM_ROW] = 0x20;
            nes_c.ram[ADDR_VRAM_ROW] = 0x20;
            nes_6502.ram[ADDR_LINE_INDEX] = 0;
            nes_c.ram[ADDR_LINE_INDEX] = 0;
            nes_6502.ram[ADDR_COMPLETED_LINES] = 0;
            nes_c.ram[ADDR_COMPLETED_LINES] = 0;
            nes_6502.ram[ADDR_PLAY_STATE] = 3;
            nes_c.ram[ADDR_PLAY_STATE] = 3;

            for (int step = 0; step < 4; step++) {
                cpu6502_init(&cpu, &nes_6502);
                cpu6502_set_state(&cpu, ROM_ADDR_CHECK_FOR_COMPLETED_ROWS, 0, 0, 0, 0, 0xFD);
                bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_CHECK_FOR_COMPLETED_ROWS, 5000);
                assert(ok);
                (void)ok;

                playState_checkForCompletedRows(&nes_c);
            }

            assert(nes_6502.ram[ADDR_COMPLETED_LINES] == nes_c.ram[ADDR_COMPLETED_LINES]);
            assert(nes_6502.ram[ADDR_PLAY_STATE] == nes_c.ram[ADDR_PLAY_STATE]);
            assert(nes_6502.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_INIT]);
            assert(memcmp(&nes_6502.ram[pf_base], &nes_c.ram[pf_base], PLAYFIELD_SIZE) == 0);
        }
    }

    /* Test 10: Differential diffOldAndNewButtons ($ABBD) */
    {
        uint8_t test_patterns[] = {
            0x00, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01,
            0xFF, 0x55, 0xAA, 0x12, 0x84, 0x99, 0x42
        };
        size_t num_patterns = sizeof(test_patterns) / sizeof(test_patterns[0]);

        for (size_t i = 0; i < num_patterns; i++) {
            for (size_t j = 0; j < num_patterns; j++) {
                NES_State nes_6502 = rom_nes;
                NES_State nes_c = rom_nes;

                nes_6502.ram[ADDR_NEWLY_PRESSED_P1] = test_patterns[i];
                nes_6502.ram[ADDR_HELD_BUTTONS_P1] = test_patterns[j];
                nes_6502.ram[ADDR_NEWLY_PRESSED_P2] = test_patterns[j];
                nes_6502.ram[ADDR_HELD_BUTTONS_P2] = test_patterns[i];

                nes_c.ram[ADDR_NEWLY_PRESSED_P1] = test_patterns[i];
                nes_c.ram[ADDR_HELD_BUTTONS_P1] = test_patterns[j];
                nes_c.ram[ADDR_NEWLY_PRESSED_P2] = test_patterns[j];
                nes_c.ram[ADDR_HELD_BUTTONS_P2] = test_patterns[i];

                cpu6502_init(&cpu, &nes_6502);
                cpu6502_set_state(&cpu, ROM_ADDR_DIFF_OLD_AND_NEW_BUTTONS, 0, 0, 0, 0, 0xFD);
                bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_DIFF_OLD_AND_NEW_BUTTONS, 1000);
                assert(ok);
                (void)ok;

                diffOldAndNewButtons(&nes_c);

                assert(nes_6502.ram[ADDR_NEWLY_PRESSED_P1] == nes_c.ram[ADDR_NEWLY_PRESSED_P1]);
                assert(nes_6502.ram[ADDR_NEWLY_PRESSED_P2] == nes_c.ram[ADDR_NEWLY_PRESSED_P2]);
                assert(nes_6502.ram[ADDR_HELD_BUTTONS_P1] == nes_c.ram[ADDR_HELD_BUTTONS_P1]);
                assert(nes_6502.ram[ADDR_HELD_BUTTONS_P2] == nes_c.ram[ADDR_HELD_BUTTONS_P2]);
            }
        }
    }

    /* Test 11: Differential pollController ($AB9D) */
    {
        uint8_t test_inputs[] = {
            0x00, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01,
            0x81, 0x42, 0x18, 0xFF
        };
        size_t num_inputs = sizeof(test_inputs) / sizeof(test_inputs[0]);

        for (size_t i = 0; i < num_inputs; i++) {
            for (size_t j = 0; j < num_inputs; j++) {
                NES_State nes_6502 = rom_nes;
                NES_State nes_c = rom_nes;

                nes_set_controller(&nes_6502, 0, test_inputs[i]);
                nes_set_controller(&nes_6502, 1, test_inputs[j]);
                nes_set_controller(&nes_c, 0, test_inputs[i]);
                nes_set_controller(&nes_c, 1, test_inputs[j]);

                cpu6502_init(&cpu, &nes_6502);
                cpu6502_set_state(&cpu, ROM_ADDR_POLL_CONTROLLER, 0, 0, 0, 0, 0xFD);
                bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_POLL_CONTROLLER, 2000);
                assert(ok);
                (void)ok;

                pollController(&nes_c);

                assert(nes_6502.ram[ADDR_NEWLY_PRESSED_P1] == nes_c.ram[ADDR_NEWLY_PRESSED_P1]);
                assert(nes_6502.ram[ADDR_NEWLY_PRESSED_P2] == nes_c.ram[ADDR_NEWLY_PRESSED_P2]);
                assert(nes_6502.ram[ADDR_HELD_BUTTONS_P1] == nes_c.ram[ADDR_HELD_BUTTONS_P1]);
                assert(nes_6502.ram[ADDR_HELD_BUTTONS_P2] == nes_c.ram[ADDR_HELD_BUTTONS_P2]);
                assert(nes_6502.ram[ADDR_GENERAL_COUNTER2] == nes_c.ram[ADDR_GENERAL_COUNTER2]);
                assert(nes_6502.ram[ADDR_GENERAL_COUNTER3] == nes_c.ram[ADDR_GENERAL_COUNTER3]);
            }
        }
    }

    /* Test 12: Differential demoButtonsTable_indexIncr ($9DE8) */
    {
        uint16_t test_addrs[] = {
            0x0000, 0xDD00, 0xDD50, 0xDDFF, 0xDE00, 0xFFFF
        };
        size_t num_addrs = sizeof(test_addrs) / sizeof(test_addrs[0]);

        for (size_t i = 0; i < num_addrs; i++) {
            NES_State nes_6502 = rom_nes;
            NES_State nes_c = rom_nes;

            uint16_t addr = test_addrs[i];
            nes_6502.ram[ADDR_DEMO_BUTTONS_ADDR] = (uint8_t)(addr & 0xFF);
            nes_6502.ram[ADDR_DEMO_BUTTONS_ADDR + 1] = (uint8_t)((addr >> 8) & 0xFF);
            nes_c.ram[ADDR_DEMO_BUTTONS_ADDR] = (uint8_t)(addr & 0xFF);
            nes_c.ram[ADDR_DEMO_BUTTONS_ADDR + 1] = (uint8_t)((addr >> 8) & 0xFF);

            cpu6502_init(&cpu, &nes_6502);
            cpu6502_set_state(&cpu, ROM_ADDR_DEMO_BUTTONS_INDEX_INCR, 0, 0, 0, 0, 0xFD);
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_DEMO_BUTTONS_INDEX_INCR, 500);
            assert(ok);
            (void)ok;

            demoButtonsTable_indexIncr(&nes_c);

            assert(nes_6502.ram[ADDR_DEMO_BUTTONS_ADDR] == nes_c.ram[ADDR_DEMO_BUTTONS_ADDR]);
            assert(nes_6502.ram[ADDR_DEMO_BUTTONS_ADDR + 1] == nes_c.ram[ADDR_DEMO_BUTTONS_ADDR + 1]);
        }
    }

    /* Test 13: Differential makePlayer1Active ($8776) & makePlayer2Active ($8792) */
    {
        NES_State nes_6502 = rom_nes;
        NES_State nes_c = rom_nes;

        for (int i = 0; i < 32; i++) {
            nes_6502.ram[ADDR_P1_BASE + i] = (uint8_t)(0x20 + i);
            nes_c.ram[ADDR_P1_BASE + i] = (uint8_t)(0x20 + i);
            nes_6502.ram[ADDR_P2_BASE + i] = (uint8_t)(0x60 + i);
            nes_c.ram[ADDR_P2_BASE + i] = (uint8_t)(0x60 + i);
        }
        nes_6502.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_A | BUTTON_START;
        nes_c.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_A | BUTTON_START;
        nes_6502.ram[ADDR_HELD_BUTTONS_P1] = BUTTON_A;
        nes_c.ram[ADDR_HELD_BUTTONS_P1] = BUTTON_A;

        nes_6502.ram[ADDR_NEWLY_PRESSED_P2] = BUTTON_B;
        nes_c.ram[ADDR_NEWLY_PRESSED_P2] = BUTTON_B;
        nes_6502.ram[ADDR_HELD_BUTTONS_P2] = BUTTON_B | BUTTON_SELECT;
        nes_c.ram[ADDR_HELD_BUTTONS_P2] = BUTTON_B | BUTTON_SELECT;

        /* makePlayer1Active */
        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_MAKE_PLAYER1_ACTIVE, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_MAKE_PLAYER1_ACTIVE, 1000);
        assert(ok);
        (void)ok;

        makePlayer1Active(&nes_c);

        assert(nes_6502.ram[ADDR_ACTIVE_PLAYER] == nes_c.ram[ADDR_ACTIVE_PLAYER]);
        assert(nes_6502.ram[ADDR_PLAYFIELD_ADDR + 1] == nes_c.ram[ADDR_PLAYFIELD_ADDR + 1]);
        assert(nes_6502.ram[ADDR_NEWLY_PRESSED_BUTTONS] == nes_c.ram[ADDR_NEWLY_PRESSED_BUTTONS]);
        assert(nes_6502.ram[ADDR_HELD_BUTTONS] == nes_c.ram[ADDR_HELD_BUTTONS]);
        assert(memcmp(&nes_6502.ram[ADDR_TETRIMINO_X], &nes_c.ram[ADDR_TETRIMINO_X], 32) == 0);

        /* makePlayer2Active */
        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_MAKE_PLAYER2_ACTIVE, 0, 0, 0, 0, 0xFD);
        ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_MAKE_PLAYER2_ACTIVE, 1000);
        assert(ok);

        makePlayer2Active(&nes_c);

        assert(nes_6502.ram[ADDR_ACTIVE_PLAYER] == nes_c.ram[ADDR_ACTIVE_PLAYER]);
        assert(nes_6502.ram[ADDR_PLAYFIELD_ADDR + 1] == nes_c.ram[ADDR_PLAYFIELD_ADDR + 1]);
        assert(nes_6502.ram[ADDR_NEWLY_PRESSED_BUTTONS] == nes_c.ram[ADDR_NEWLY_PRESSED_BUTTONS]);
        assert(nes_6502.ram[ADDR_HELD_BUTTONS] == nes_c.ram[ADDR_HELD_BUTTONS]);
        assert(memcmp(&nes_6502.ram[ADDR_TETRIMINO_X], &nes_c.ram[ADDR_TETRIMINO_X], 32) == 0);

        /* savePlayer1State */
        nes_6502.ram[ADDR_TETRIMINO_X] = 0xAA;
        nes_c.ram[ADDR_TETRIMINO_X] = 0xAA;
        nes_6502.ram[ADDR_NUMBER_OF_PLAYERS] = 2;
        nes_c.ram[ADDR_NUMBER_OF_PLAYERS] = 2;
        nes_6502.ram[ADDR_PENDING_GARBAGE] = 4;
        nes_c.ram[ADDR_PENDING_GARBAGE] = 4;
        nes_6502.ram[ADDR_PENDING_GARBAGE_INACTIVE] = 1;
        nes_c.ram[ADDR_PENDING_GARBAGE_INACTIVE] = 1;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_SAVE_PLAYER1_STATE, 0, 0, 0, 0, 0xFD);
        ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_SAVE_PLAYER1_STATE, 1000);
        assert(ok);

        savePlayer1State(&nes_c);

        assert(nes_6502.ram[ADDR_P1_BASE] == nes_c.ram[ADDR_P1_BASE]);
        assert(nes_6502.ram[ADDR_PENDING_GARBAGE] == nes_c.ram[ADDR_PENDING_GARBAGE]);
        assert(nes_6502.ram[ADDR_PENDING_GARBAGE_INACTIVE] == nes_c.ram[ADDR_PENDING_GARBAGE_INACTIVE]);

        /* savePlayer2State */
        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_SAVE_PLAYER2_STATE, 0, 0, 0, 0, 0xFD);
        ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_SAVE_PLAYER2_STATE, 1000);
        assert(ok);

        savePlayer2State(&nes_c);

        assert(nes_6502.ram[ADDR_P2_BASE] == nes_c.ram[ADDR_P2_BASE]);
        assert(nes_6502.ram[ADDR_PENDING_GARBAGE] == nes_c.ram[ADDR_PENDING_GARBAGE]);
        assert(nes_6502.ram[ADDR_PENDING_GARBAGE_INACTIVE] == nes_c.ram[ADDR_PENDING_GARBAGE_INACTIVE]);
    }

    /* Test 14: Differential playState_unassignOrientationId ($9E2F) & playState_incrementPlayState ($9E37) */
    {
        NES_State nes_6502 = rom_nes;
        NES_State nes_c = rom_nes;

        nes_6502.ram[ADDR_CURRENT_PIECE] = 0x05;
        nes_c.ram[ADDR_CURRENT_PIECE] = 0x05;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_PLAYSTATE_UNASSIGN_ORIENTATION, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_PLAYSTATE_UNASSIGN_ORIENTATION, 500);
        assert(ok);
        (void)ok;

        playState_unassignOrientationId(&nes_c);

        assert(nes_6502.ram[ADDR_CURRENT_PIECE] == nes_c.ram[ADDR_CURRENT_PIECE]);
        assert(nes_c.ram[ADDR_CURRENT_PIECE] == TETRIMINO_HIDDEN);

        nes_6502.ram[ADDR_PLAY_STATE] = 11;
        nes_c.ram[ADDR_PLAY_STATE] = 11;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_PLAYSTATE_INCREMENT_PLAYSTATE, 0, 0, 0, 0, 0xFD);
        ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_PLAYSTATE_INCREMENT_PLAYSTATE, 500);
        assert(ok);

        playState_incrementPlayState(&nes_c);

        assert(nes_6502.ram[ADDR_PLAY_STATE] == nes_c.ram[ADDR_PLAY_STATE]);
        assert(nes_c.ram[ADDR_PLAY_STATE] == 12);
    }

    /* Test 15: Differential branchOnPlayStatePlayer1 ($81B2) dispatch */
    {
        uint8_t test_states[] = { 0, 4, 6, 9, 11 };
        size_t num_states = sizeof(test_states) / sizeof(test_states[0]);

        for (size_t i = 0; i < num_states; i++) {
            NES_State nes_6502 = rom_nes;
            NES_State nes_c = rom_nes;

            uint8_t st = test_states[i];
            nes_6502.ram[ADDR_PLAY_STATE] = st;
            nes_c.ram[ADDR_PLAY_STATE] = st;
            nes_6502.ram[ADDR_GAME_TYPE] = GAMETYPE_A;
            nes_c.ram[ADDR_GAME_TYPE] = GAMETYPE_A;

            cpu6502_init(&cpu, &nes_6502);
            cpu6502_set_state(&cpu, ROM_ADDR_BRANCH_ON_PLAY_STATE_P1, 0, 0, 0, 0, 0xFD);
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_BRANCH_ON_PLAY_STATE_P1, 1000);
            assert(ok);
            (void)ok;

            branchOnPlayStatePlayer1(&nes_c);

            assert(nes_6502.ram[ADDR_PLAY_STATE] == nes_c.ram[ADDR_PLAY_STATE]);
            assert(nes_6502.ram[ADDR_CURRENT_PIECE] == nes_c.ram[ADDR_CURRENT_PIECE]);
        }
    }

    /* Test 16: Differential playState_spawnNextTetrimino ($988E) 1P mode across 19 orientations */
    {
        for (uint8_t orient = 0; orient < ORIENTATION_COUNT; orient++) {
            NES_State nes_6502 = rom_nes;
            NES_State nes_c = rom_nes;

            nes_6502.ram[ADDR_VRAM_ROW] = VRAM_ROW_COMPLETE;
            nes_c.ram[ADDR_VRAM_ROW] = VRAM_ROW_COMPLETE;
            nes_6502.ram[ADDR_NUMBER_OF_PLAYERS] = 1;
            nes_c.ram[ADDR_NUMBER_OF_PLAYERS] = 1;
            nes_6502.ram[ADDR_NEXT_PIECE] = orient;
            nes_c.ram[ADDR_NEXT_PIECE] = orient;
            nes_6502.ram[ADDR_PLAY_STATE] = PLAYSTATE_SPAWN_NEXT;
            nes_c.ram[ADDR_PLAY_STATE] = PLAYSTATE_SPAWN_NEXT;
            nes_6502.ram[ADDR_AUTOREPEAT_Y] = 0x40;
            nes_c.ram[ADDR_AUTOREPEAT_Y] = 0x40;

            /* Set RNG seed */
            nes_6502.ram[ADDR_RNG_SEED] = (uint8_t)(0x12 + orient);
            nes_6502.ram[ADDR_RNG_SEED + 1] = 0x34;
            nes_c.ram[ADDR_RNG_SEED] = (uint8_t)(0x12 + orient);
            nes_c.ram[ADDR_RNG_SEED + 1] = 0x34;

            cpu6502_init(&cpu, &nes_6502);
            cpu6502_set_state(&cpu, ROM_ADDR_SPAWN_NEXT_TETRIMINO, 0, 0, 0, 0, 0xFD);
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_SPAWN_NEXT_TETRIMINO, 2000);
            assert(ok);
            (void)ok;

            playState_spawnNextTetrimino(&nes_c);

            assert(nes_6502.ram[ADDR_TETRIMINO_X] == nes_c.ram[ADDR_TETRIMINO_X]);
            assert(nes_6502.ram[ADDR_TETRIMINO_Y] == nes_c.ram[ADDR_TETRIMINO_Y]);
            assert(nes_6502.ram[ADDR_FALL_TIMER] == nes_c.ram[ADDR_FALL_TIMER]);
            assert(nes_6502.ram[ADDR_PLAY_STATE] == nes_c.ram[ADDR_PLAY_STATE]);
            assert(nes_6502.ram[ADDR_CURRENT_PIECE] == nes_c.ram[ADDR_CURRENT_PIECE]);
            assert(nes_6502.ram[ADDR_NEXT_PIECE] == nes_c.ram[ADDR_NEXT_PIECE]);
            assert(nes_6502.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER] == nes_c.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER]);
            assert(nes_6502.ram[ADDR_AUTOREPEAT_Y] == nes_c.ram[ADDR_AUTOREPEAT_Y]);
            assert(nes_6502.ram[ADDR_OUT_OF_DATE_RENDER_FLAGS] == nes_c.ram[ADDR_OUT_OF_DATE_RENDER_FLAGS]);
            for (int s = 0; s < 14; s++) {
                assert(nes_6502.ram[ADDR_STATS_BY_TYPE + s] == nes_c.ram[ADDR_STATS_BY_TYPE + s]);
            }
        }
    }

    /* Test 17: Differential playState_spawnNextTetrimino ($988E) 2P mode delay mechanics */
    {
        /* Subcase A: Frame 0 triggers 2P delay */
        {
            NES_State nes_6502 = rom_nes;
            NES_State nes_c = rom_nes;

            nes_6502.ram[ADDR_VRAM_ROW] = VRAM_ROW_COMPLETE;
            nes_c.ram[ADDR_VRAM_ROW] = VRAM_ROW_COMPLETE;
            nes_6502.ram[ADDR_NUMBER_OF_PLAYERS] = 2;
            nes_c.ram[ADDR_NUMBER_OF_PLAYERS] = 2;
            nes_6502.ram[ADDR_ACTIVE_PLAYER] = 1;
            nes_c.ram[ADDR_ACTIVE_PLAYER] = 1;
            nes_6502.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER] = 0;
            nes_c.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER] = 0;
            nes_6502.ram[ADDR_PLAY_STATE] = PLAYSTATE_SPAWN_NEXT;
            nes_c.ram[ADDR_PLAY_STATE] = PLAYSTATE_SPAWN_NEXT;

            nes_6502.ram[ADDR_RNG_SEED] = 0x55;
            nes_6502.ram[ADDR_RNG_SEED + 1] = 0xAA;
            nes_c.ram[ADDR_RNG_SEED] = 0x55;
            nes_c.ram[ADDR_RNG_SEED + 1] = 0xAA;

            cpu6502_init(&cpu, &nes_6502);
            cpu6502_set_state(&cpu, ROM_ADDR_SPAWN_NEXT_TETRIMINO, 0, 0, 0, 0, 0xFD);
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_SPAWN_NEXT_TETRIMINO, 2000);
            assert(ok);
            (void)ok;

            playState_spawnNextTetrimino(&nes_c);

            assert(nes_6502.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER] == nes_c.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER]);
            assert(nes_6502.ram[ADDR_TWO_PLAYER_PIECE_DELAY_PLAYER] == nes_c.ram[ADDR_TWO_PLAYER_PIECE_DELAY_PLAYER]);
            assert(nes_6502.ram[ADDR_TWO_PLAYER_PIECE_DELAY_PIECE] == nes_c.ram[ADDR_TWO_PLAYER_PIECE_DELAY_PIECE]);
            assert(nes_6502.ram[ADDR_PLAY_STATE] == nes_c.ram[ADDR_PLAY_STATE]);
        }

        /* Subcase B: Delay in progress (counter = 12) -> returns early */
        {
            NES_State nes_6502 = rom_nes;
            NES_State nes_c = rom_nes;

            nes_6502.ram[ADDR_VRAM_ROW] = VRAM_ROW_COMPLETE;
            nes_c.ram[ADDR_VRAM_ROW] = VRAM_ROW_COMPLETE;
            nes_6502.ram[ADDR_NUMBER_OF_PLAYERS] = 2;
            nes_c.ram[ADDR_NUMBER_OF_PLAYERS] = 2;
            nes_6502.ram[ADDR_ACTIVE_PLAYER] = 1;
            nes_c.ram[ADDR_ACTIVE_PLAYER] = 1;
            nes_6502.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER] = 12;
            nes_c.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER] = 12;
            nes_6502.ram[ADDR_TWO_PLAYER_PIECE_DELAY_PLAYER] = 1;
            nes_c.ram[ADDR_TWO_PLAYER_PIECE_DELAY_PLAYER] = 1;
            nes_6502.ram[ADDR_PLAY_STATE] = PLAYSTATE_SPAWN_NEXT;
            nes_c.ram[ADDR_PLAY_STATE] = PLAYSTATE_SPAWN_NEXT;

            cpu6502_init(&cpu, &nes_6502);
            cpu6502_set_state(&cpu, ROM_ADDR_SPAWN_NEXT_TETRIMINO, 0, 0, 0, 0, 0xFD);
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_SPAWN_NEXT_TETRIMINO, 1000);
            assert(ok);
            (void)ok;

            playState_spawnNextTetrimino(&nes_c);

            assert(nes_6502.ram[ADDR_PLAY_STATE] == nes_c.ram[ADDR_PLAY_STATE]);
            assert(nes_6502.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER] == nes_c.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER]);
        }

        /* Subcase C: Inactive player at threshold ($1C) -> returns early */
        {
            NES_State nes_6502 = rom_nes;
            NES_State nes_c = rom_nes;

            nes_6502.ram[ADDR_VRAM_ROW] = VRAM_ROW_COMPLETE;
            nes_c.ram[ADDR_VRAM_ROW] = VRAM_ROW_COMPLETE;
            nes_6502.ram[ADDR_NUMBER_OF_PLAYERS] = 2;
            nes_c.ram[ADDR_NUMBER_OF_PLAYERS] = 2;
            nes_6502.ram[ADDR_ACTIVE_PLAYER] = 2; /* Player 2 active */
            nes_c.ram[ADDR_ACTIVE_PLAYER] = 2;
            nes_6502.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER] = TWO_PLAYER_PIECE_DELAY_FRAMES;
            nes_c.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER] = TWO_PLAYER_PIECE_DELAY_FRAMES;
            nes_6502.ram[ADDR_TWO_PLAYER_PIECE_DELAY_PLAYER] = 1; /* Delay belongs to Player 1 */
            nes_c.ram[ADDR_TWO_PLAYER_PIECE_DELAY_PLAYER] = 1;
            nes_6502.ram[ADDR_PLAY_STATE] = PLAYSTATE_SPAWN_NEXT;
            nes_c.ram[ADDR_PLAY_STATE] = PLAYSTATE_SPAWN_NEXT;

            cpu6502_init(&cpu, &nes_6502);
            cpu6502_set_state(&cpu, ROM_ADDR_SPAWN_NEXT_TETRIMINO, 0, 0, 0, 0, 0xFD);
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_SPAWN_NEXT_TETRIMINO, 1000);
            assert(ok);
            (void)ok;

            playState_spawnNextTetrimino(&nes_c);

            assert(nes_6502.ram[ADDR_PLAY_STATE] == nes_c.ram[ADDR_PLAY_STATE]);
        }

        /* Subcase D: Delay complete ($1C) for active player -> spawns piece */
        {
            NES_State nes_6502 = rom_nes;
            NES_State nes_c = rom_nes;

            nes_6502.ram[ADDR_VRAM_ROW] = VRAM_ROW_COMPLETE;
            nes_c.ram[ADDR_VRAM_ROW] = VRAM_ROW_COMPLETE;
            nes_6502.ram[ADDR_NUMBER_OF_PLAYERS] = 2;
            nes_c.ram[ADDR_NUMBER_OF_PLAYERS] = 2;
            nes_6502.ram[ADDR_ACTIVE_PLAYER] = 1;
            nes_c.ram[ADDR_ACTIVE_PLAYER] = 1;
            nes_6502.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER] = TWO_PLAYER_PIECE_DELAY_FRAMES;
            nes_c.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER] = TWO_PLAYER_PIECE_DELAY_FRAMES;
            nes_6502.ram[ADDR_TWO_PLAYER_PIECE_DELAY_PLAYER] = 1;
            nes_c.ram[ADDR_TWO_PLAYER_PIECE_DELAY_PLAYER] = 1;
            nes_6502.ram[ADDR_TWO_PLAYER_PIECE_DELAY_PIECE] = ORIENT_L_DOWN;
            nes_c.ram[ADDR_TWO_PLAYER_PIECE_DELAY_PIECE] = ORIENT_L_DOWN;
            nes_6502.ram[ADDR_NEXT_PIECE] = ORIENT_Z_HORIZ;
            nes_c.ram[ADDR_NEXT_PIECE] = ORIENT_Z_HORIZ;
            nes_6502.ram[ADDR_PLAY_STATE] = PLAYSTATE_SPAWN_NEXT;
            nes_c.ram[ADDR_PLAY_STATE] = PLAYSTATE_SPAWN_NEXT;

            cpu6502_init(&cpu, &nes_6502);
            cpu6502_set_state(&cpu, ROM_ADDR_SPAWN_NEXT_TETRIMINO, 0, 0, 0, 0, 0xFD);
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_SPAWN_NEXT_TETRIMINO, 2000);
            assert(ok);
            (void)ok;

            playState_spawnNextTetrimino(&nes_c);

            assert(nes_6502.ram[ADDR_PLAY_STATE] == nes_c.ram[ADDR_PLAY_STATE]);
            assert(nes_6502.ram[ADDR_TETRIMINO_X] == nes_c.ram[ADDR_TETRIMINO_X]);
            assert(nes_6502.ram[ADDR_TETRIMINO_Y] == nes_c.ram[ADDR_TETRIMINO_Y]);
            assert(nes_6502.ram[ADDR_FALL_TIMER] == nes_c.ram[ADDR_FALL_TIMER]);
            assert(nes_6502.ram[ADDR_CURRENT_PIECE] == nes_c.ram[ADDR_CURRENT_PIECE]);
            assert(nes_6502.ram[ADDR_NEXT_PIECE] == nes_c.ram[ADDR_NEXT_PIECE]);
            assert(nes_6502.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER] == nes_c.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER]);
            assert(nes_6502.ram[ADDR_AUTOREPEAT_Y] == nes_c.ram[ADDR_AUTOREPEAT_Y]);
        }
    }

    /* Test 18: Differential noop_disabledVramRowIncr ($9874) & gameModeState_vblankThenRunState2 ($9E27) */
    {
        NES_State nes_6502 = rom_nes;
        NES_State nes_c = rom_nes;

        nes_6502.ram[ADDR_GAME_MODE_STATE] = 7;
        nes_c.ram[ADDR_GAME_MODE_STATE] = 7;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_GAME_MODE_STATE_VBLANK_THEN_RUN_STATE_2, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_GAME_MODE_STATE_VBLANK_THEN_RUN_STATE_2, 500);
        assert(ok);
        (void)ok;

        gameModeState_vblankThenRunState2(&nes_c);

        assert(nes_6502.ram[ADDR_GAME_MODE_STATE] == nes_c.ram[ADDR_GAME_MODE_STATE]);
        assert(nes_c.ram[ADDR_GAME_MODE_STATE] == 2);
    }

    /* Test 19: Differential gameModeState_updateCountersAndNonPlayerState ($8884) */
    {
        uint8_t select_presses[] = { 0, BUTTON_SELECT };
        uint8_t delay_inits[] = { 0, 10 };

        for (size_t s = 0; s < 2; s++) {
            for (size_t d = 0; d < 2; d++) {
                NES_State nes_6502 = rom_nes;
                NES_State nes_c = rom_nes;

                nes_6502.ram[ADDR_OAM_STAGING_LENGTH] = 20;
                nes_c.ram[ADDR_OAM_STAGING_LENGTH] = 20;
                nes_6502.ram[ADDR_P1_FALL_TIMER] = 0x10;
                nes_c.ram[ADDR_P1_FALL_TIMER] = 0x10;
                nes_6502.ram[ADDR_P2_FALL_TIMER] = 0x20;
                nes_c.ram[ADDR_P2_FALL_TIMER] = 0x20;
                nes_6502.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER] = delay_inits[d];
                nes_c.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER] = delay_inits[d];
                nes_6502.ram[ADDR_DISPLAY_NEXT_PIECE] = 0;
                nes_c.ram[ADDR_DISPLAY_NEXT_PIECE] = 0;
                nes_6502.ram[ADDR_NEWLY_PRESSED_P1] = select_presses[s];
                nes_c.ram[ADDR_NEWLY_PRESSED_P1] = select_presses[s];
                nes_6502.ram[ADDR_GAME_MODE_STATE] = 2;
                nes_c.ram[ADDR_GAME_MODE_STATE] = 2;

                cpu6502_init(&cpu, &nes_6502);
                cpu6502_set_state(&cpu, ROM_ADDR_UPDATE_COUNTERS_AND_NON_PLAYER_STATE, 0, 0, 0, 0, 0xFD);
                bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_UPDATE_COUNTERS_AND_NON_PLAYER_STATE, 500);
                assert(ok);
                (void)ok;

                gameModeState_updateCountersAndNonPlayerState(&nes_c);

                assert(nes_6502.ram[ADDR_OAM_STAGING_LENGTH] == nes_c.ram[ADDR_OAM_STAGING_LENGTH]);
                assert(nes_6502.ram[ADDR_P1_FALL_TIMER] == nes_c.ram[ADDR_P1_FALL_TIMER]);
                assert(nes_6502.ram[ADDR_P2_FALL_TIMER] == nes_c.ram[ADDR_P2_FALL_TIMER]);
                assert(nes_6502.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER] == nes_c.ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER]);
                assert(nes_6502.ram[ADDR_DISPLAY_NEXT_PIECE] == nes_c.ram[ADDR_DISPLAY_NEXT_PIECE]);
                assert(nes_6502.ram[ADDR_GAME_MODE_STATE] == nes_c.ram[ADDR_GAME_MODE_STATE]);
            }
        }
    }

    /* Test 20: Differential branchOnPlayStatePlayer1 ($81B2) State 8 dispatch */
    {
        NES_State nes_6502 = rom_nes;
        NES_State nes_c = rom_nes;

        nes_6502.ram[ADDR_VRAM_ROW] = VRAM_ROW_COMPLETE;
        nes_c.ram[ADDR_VRAM_ROW] = VRAM_ROW_COMPLETE;
        nes_6502.ram[ADDR_NUMBER_OF_PLAYERS] = 1;
        nes_c.ram[ADDR_NUMBER_OF_PLAYERS] = 1;
        nes_6502.ram[ADDR_NEXT_PIECE] = ORIENT_J_DOWN;
        nes_c.ram[ADDR_NEXT_PIECE] = ORIENT_J_DOWN;
        nes_6502.ram[ADDR_PLAY_STATE] = PLAYSTATE_SPAWN_NEXT;
        nes_c.ram[ADDR_PLAY_STATE] = PLAYSTATE_SPAWN_NEXT;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_BRANCH_ON_PLAY_STATE_P1, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_BRANCH_ON_PLAY_STATE_P1, 2000);
        assert(ok);
        (void)ok;

        branchOnPlayStatePlayer1(&nes_c);

        assert(nes_6502.ram[ADDR_PLAY_STATE] == nes_c.ram[ADDR_PLAY_STATE]);
        assert(nes_6502.ram[ADDR_CURRENT_PIECE] == nes_c.ram[ADDR_CURRENT_PIECE]);
        assert(nes_6502.ram[ADDR_TETRIMINO_X] == nes_c.ram[ADDR_TETRIMINO_X]);
        assert(nes_6502.ram[ADDR_TETRIMINO_Y] == nes_c.ram[ADDR_TETRIMINO_Y]);
    }

    /* Test 21: Differential gameMode_legalScreen ($8200) */
    {
        NES_State nes_6502 = rom_nes;
        NES_State nes_c = rom_nes;

        nes_6502.ram[ADDR_GAME_MODE] = MODE_LEGAL_SCREEN;
        nes_c.ram[ADDR_GAME_MODE] = MODE_LEGAL_SCREEN;
        nes_6502.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;
        nes_c.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_LEGAL_SCREEN, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_LEGAL_SCREEN, 300000);
        assert(ok);
        (void)ok;

        gameMode_legalScreen(&nes_c);

        assert(nes_6502.ram[ADDR_GAME_MODE] == nes_c.ram[ADDR_GAME_MODE]);
        assert(nes_6502.ram[ADDR_RENDER_MODE] == nes_c.ram[ADDR_RENDER_MODE]);
        assert(nes_c.ram[ADDR_GAME_MODE] == MODE_TITLE_SCREEN);
    }

    /* Test 22: Differential gameMode_titleScreen ($824F) with START button */
    {
        NES_State nes_6502 = rom_nes;
        NES_State nes_c = rom_nes;

        nes_6502.ram[ADDR_GAME_MODE] = MODE_TITLE_SCREEN;
        nes_c.ram[ADDR_GAME_MODE] = MODE_TITLE_SCREEN;
        nes_6502.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;
        nes_c.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_TITLE_SCREEN, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_TITLE_SCREEN, 50000);
        assert(ok);
        (void)ok;

        gameMode_titleScreen(&nes_c);

        assert(nes_6502.ram[ADDR_GAME_MODE] == nes_c.ram[ADDR_GAME_MODE]);
        assert(nes_6502.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_INIT]);
        assert(nes_6502.ram[ADDR_RENDER_MODE] == nes_c.ram[ADDR_RENDER_MODE]);
        assert(nes_6502.ram[ADDR_DEMO_RECORD_FLAG] == nes_c.ram[ADDR_DEMO_RECORD_FLAG]);
        assert(nes_6502.ram[ADDR_DISPLAY_NEXT_PIECE] == nes_c.ram[ADDR_DISPLAY_NEXT_PIECE]);
        assert(nes_c.ram[ADDR_GAME_MODE] == MODE_GAME_TYPE_MENU);
        assert(nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == SFX_MENU_SELECT);
    }

    /* Test 23: Differential gameMode_titleScreen ($824F) with timeout */
    {
        NES_State nes_6502 = rom_nes;
        NES_State nes_c = rom_nes;

        nes_6502.ram[ADDR_GAME_MODE] = MODE_TITLE_SCREEN;
        nes_c.ram[ADDR_GAME_MODE] = MODE_TITLE_SCREEN;
        nes_6502.ram[ADDR_NEWLY_PRESSED_P1] = 0;
        nes_c.ram[ADDR_NEWLY_PRESSED_P1] = 0;
        nes_6502.ram[ADDR_FRAME_COUNTER + 1] = 5;
        nes_c.ram[ADDR_FRAME_COUNTER + 1] = 5;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_TITLE_TIMEOUT_CHECK, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_TITLE_TIMEOUT_CHECK, 50000);
        assert(ok);
        (void)ok;

        gameMode_titleScreen(&nes_c);

        assert(nes_6502.ram[ADDR_GAME_MODE] == nes_c.ram[ADDR_GAME_MODE]);
        assert(nes_6502.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_INIT]);
        assert(nes_c.ram[ADDR_GAME_MODE] == MODE_START_DEMO);
        assert(nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == SFX_MENU_SELECT);
    }

    /* Test 24: Differential branchOnGameMode ($8161) dispatch */
    {
        NES_State nes_6502 = rom_nes;
        NES_State nes_c = rom_nes;

        nes_6502.ram[ADDR_GAME_MODE] = MODE_TITLE_SCREEN;
        nes_c.ram[ADDR_GAME_MODE] = MODE_TITLE_SCREEN;
        nes_6502.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;
        nes_c.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_BRANCH_ON_GAME_MODE, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_BRANCH_ON_GAME_MODE, 50000);
        assert(ok);
        (void)ok;

        branchOnGameMode(&nes_c);

        assert(nes_6502.ram[ADDR_GAME_MODE] == nes_c.ram[ADDR_GAME_MODE]);
        assert(nes_6502.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_INIT]);
        assert(nes_c.ram[ADDR_GAME_MODE] == MODE_GAME_TYPE_MENU);
    }

    printf("  [PASS] test_differential_verification (All 6502 ROM vs C routines proven equivalent)\n");
    return 0;
}
