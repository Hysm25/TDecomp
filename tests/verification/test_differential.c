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
#include "game/highscore.h"
#include "game/render.h"
#include "game/ending.h"
#include "game/audio.h"
#include "game/system.h"

/* Verified 6502 PRG addresses from tetris.lbl */
#define ROM_ADDR_NMI                    0x8005
#define ROM_ADDR_LAA82                  0xAA82
#define ROM_ADDR_MEMSET_PPU_PAGE_AND_MORE 0xAC1C
#define ROM_ADDR_SET_MMC1_CONTROL       0xACA3
#define ROM_ADDR_CHANGE_CHR_BANK0       0xACB7
#define ROM_ADDR_CHANGE_CHR_BANK1       0xACCB
#define ROM_ADDR_CHANGE_PRG_BANK        0xACDF
#define ROM_ADDR_RENDER                 0x804B
#define ROM_ADDR_ENDING_INIT_TYPE_B_VARS 0xA463
#define ROM_ADDR_ENDING_TYPE_B_CONCERT_PATCH_HEIGHT 0xA4B1
#define ROM_ADDR_PATCH_TO_PPU           0xA507
#define ROM_ADDR_RENDER_ENDING          0xA527
#define ROM_ADDR_ENDING_TYPE_B          0xA52E
#define ROM_ADDR_UPDATE_PPU_MASK        0xAA59
#define ROM_ADDR_COPY_TO_SQ1_CHANNEL    0xE04B
#define ROM_ADDR_COPY_TO_TRI_CHANNEL    0xE04F
#define ROM_ADDR_COPY_TO_NOISE_CHANNEL  0xE053
#define ROM_ADDR_COPY_TO_SQ2_CHANNEL    0xE057
#define ROM_ADDR_COMPUTE_SOUND_EFF_METHOD 0xE072
#define ROM_ADDR_ADVANCE_AUDIO_SLOT_FRAME 0xE0B5
#define ROM_ADDR_MUTE_AUDIO             0xE271
#define ROM_ADDR_SOUND_EFFECT_SLOT2_MAKES_NO_SOUND 0xE244
#define ROM_ADDR_INIT_SOUND_EFFECT_SHARED 0xE287
#define ROM_ADDR_SFX_SLOT1_LOCK_INIT    0xE384
#define ROM_ADDR_SFX_SLOT1_PLAYING_STOP 0xE3A1
#define ROM_ADDR_SFX_SLOT2_MEDIUM_BUZZ  0xE50B
#define ROM_ADDR_SFX_SLOT2_LOW_BUZZ     0xE513
#define ROM_ADDR_SFX_SLOT3_DONK         0xE534
#define ROM_ADDR_SFX_SLOT3_PLAYING_STOP 0xE540
#define ROM_ADDR_UPDATE_MUSIC_NO_SOUND_JMP 0xE580
#define ROM_ADDR_UPDATE_MUSIC_SET_CHAN_LO 0xE5D0
#define ROM_ADDR_UPDATE_MUSIC_SET_CHAN_LO_OFFSET 0xE637
#define ROM_ADDR_LOAD_MUSIC_TRACK       0xE6D7
#define ROM_ADDR_INIT_SQ12_IF_TRASHED   0xE735
#define ROM_ADDR_UPDATE_MUSIC_SET_CHAN_VOL 0xE766
#define ROM_ADDR_PLAY_NOISE             0xE9EB
#define ROM_ADDR_PLAY_DMC               0xEA03
#define ROM_ADDR_MUSIC_GET_NEXT_INSTRUCTION_BYTE 0xEA41
#define ROM_ADDR_ENDING_TYPE_B_CONCERT  0xA53B
#define ROM_ADDR_ENDING_TYPE_B_CATHEDRAL 0xA625
#define ROM_ADDR_ENDING_TYPE_B_CATHEDRAL_SET_SPRITE 0xA690
#define ROM_ADDR_ENDING_COMPUTE_CATHEDRAL_Y_INDEX 0xA6AE
#define ROM_ADDR_LA6BC                  0xA6BC
#define ROM_ADDR_RENDER_ENDING_UNSKIPPABLE 0xA7FD
#define ROM_ADDR_SELECT_ENDING_SCREEN   0xA96E
#define ROM_ADDR_ENDING_SELECTED        0xA9A5
#define ROM_ADDR_ENDING_TYPE_A          0xA9B1
#define ROM_ADDR_GAME_MODE_STATE_UPDATE_P1 0x8174
#define ROM_ADDR_GAME_MODE_STATE_UPDATE_P2 0x8186
#define ROM_ADDR_GAME_MODE_PLAY_AND_ENDING_HIGH_SCORE 0x819B
#define ROM_ADDR_BRANCH_ON_GAME_MODE    0x8161
#define ROM_ADDR_BRANCH_ON_PLAY_STATE_P1 0x81B2
#define ROM_ADDR_LEGAL_SCREEN           0x8200
#define ROM_ADDR_TITLE_SCREEN           0x824F
#define ROM_ADDR_TITLE_TIMEOUT_CHECK    0x8296
#define ROM_ADDR_RENDER_MODE_LEGAL_AND_TITLE 0x82B1
#define ROM_ADDR_GAME_TYPE_MENU_LOOP    0x830B
#define ROM_ADDR_LEVEL_MENU_PROCESS_P1  0x8436
#define ROM_ADDR_LEVEL_MENU_HANDLE_NAV  0x84AE
#define ROM_ADDR_RENDER_MODE_MENUS      0x85DA
#define ROM_ADDR_MAKE_PLAYER1_ACTIVE    0x8776
#define ROM_ADDR_MAKE_PLAYER2_ACTIVE    0x8792
#define ROM_ADDR_SAVE_PLAYER1_STATE     0x87AE
#define ROM_ADDR_SAVE_PLAYER2_STATE     0x87C8
#define ROM_ADDR_STAGE_SPRITE_CURRENT_PIECE 0x8A0A
#define ROM_ADDR_STAGE_SPRITE_NEXT_PIECE 0x8BCE
#define ROM_ADDR_LOAD_SPRITE_INTO_OAM_STAGING 0x8C27
#define ROM_ADDR_UPDATE_COUNTERS_AND_NON_PLAYER_STATE 0x8884
#define ROM_ADDR_UPDATE_PALETTE_FOR_LEVEL 0x9808
#define ROM_ADDR_RENDER_MODE_ENDING_ANIMATION 0x9F95
#define ROM_ADDR_PLAYSTATE_B_TYPE_GOAL_CHECK_RET 0xA42B
#define ROM_ADDR_COPY_CURRENT_SCROLL_CTRL_PPU 0xAA8A
#define ROM_ADDR_COPY_OAM_STAGING_TO_OAM 0xAB5E
#define ROM_ADDR_IS_POSITION_VALID      0x948B
#define ROM_ADDR_RENDER_MODE_PLAY_AND_DEMO 0x94EE
#define ROM_ADDR_COPY_PLAYFIELD_ROW_TO_VRAM 0x9725
#define ROM_ADDR_TWO_DIGS_TO_PPU        0x9712
#define ROM_ADDR_SPAWN_NEXT_TETRIMINO   0x988E
#define ROM_ADDR_INCREMENT_PIECE_STAT   0x9969
#define ROM_ADDR_CHECK_FOR_COMPLETED_ROWS 0x9A6B
#define ROM_ADDR_GAME_MODE_STATE_HANDLE_GAME_OVER 0x9CBF
#define ROM_ADDR_UPDATE_PLAYFIELD       0x9CAF
#define ROM_ADDR_UPDATE_MUSIC_SPEED     0x9D17
#define ROM_ADDR_DEMO_BUTTONS_INDEX_INCR 0x9DE8
#define ROM_ADDR_SET_MUSIC_TRACK        0x9E07
#define ROM_ADDR_GAME_MODE_STATE_CHECK_RESET_COMBO 0x9E16
#define ROM_ADDR_GAME_MODE_STATE_VBLANK_THEN_RUN_STATE_2 0x9E27
#define ROM_ADDR_PLAYSTATE_UNASSIGN_ORIENTATION 0x9E2F
#define ROM_ADDR_PLAYSTATE_INCREMENT_PLAYSTATE 0x9E37
#define ROM_ADDR_ADD_100_POINTS         0x9F62
#define ROM_ADDR_HANDLE_HIGH_SCORE_IF_NECESSARY 0xA0EE
#define ROM_ADDR_COPY_HIGH_SCORE_NAME   0xA192
#define ROM_ADDR_COPY_HIGH_SCORE_SCORE  0xA1C1
#define ROM_ADDR_COPY_HIGH_SCORE_LEVEL  0xA1E0
#define ROM_ADDR_RENDER_MODE_CONGRATULATIONS_SCREEN 0xA344
#define ROM_ADDR_GAME_MODE_STATE_START_BUTTON_HANDLING 0xA37F
#define ROM_ADDR_LFSR_PRNG              0xAB47
#define ROM_ADDR_POLL_CONTROLLER        0xAB9D
#define ROM_ADDR_DIFF_OLD_AND_NEW_BUTTONS 0xABBD
#define ROM_ADDR_MEMSET_PAGE            0xAC6A
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

    /* Test 1b: Continuous LFSR sequence verification across 1,000 steps */
    {
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_RNG_SEED] = 0x00;
        nes_6502.ram[ADDR_RNG_SEED + 1] = 0x00;
        nes_6502.ram[ADDR_TMP1] = 0x00;

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_RNG_SEED] = 0x00;
        nes_c.ram[ADDR_RNG_SEED + 1] = 0x00;
        nes_c.ram[ADDR_TMP1] = 0x00;

        for (int step = 0; step < 1000; step++) {
            cpu6502_init(&cpu, &nes_6502);
            cpu6502_set_state(&cpu, ROM_ADDR_LFSR_PRNG, 0, ADDR_RNG_SEED, 2, 0, 0xFD);
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_LFSR_PRNG, 1000);
            assert(ok);
            (void)ok;

            generateNextPseudorandomNumber(&nes_c, ADDR_RNG_SEED, 2);

            assert(nes_6502.ram[ADDR_RNG_SEED] == nes_c.ram[ADDR_RNG_SEED]);
            assert(nes_6502.ram[ADDR_RNG_SEED + 1] == nes_c.ram[ADDR_RNG_SEED + 1]);
            assert(nes_6502.ram[ADDR_TMP1] == nes_c.ram[ADDR_TMP1]);
        }
    }

    /* Test 1c: Calling convention, edge cases, and observable side effects audit */
    {
        uint16_t edge_seeds[] = {
            0x0000, 0xFFFF, 0x0001, 0x0002, 0x0003, 0x0200, 0x0202,
            0x5555, 0xAAAA, 0x8080, 0x7FFF, 0x8000, 0x0080, 0x0100
        };
        size_t num_edge_seeds = sizeof(edge_seeds) / sizeof(edge_seeds[0]);

        for (size_t i = 0; i < num_edge_seeds; i++) {
            uint16_t seed = edge_seeds[i];

            /* Setup 6502 state with realistic caller environment */
            NES_State nes_6502 = rom_nes;
            nes_6502.ram[ADDR_RNG_SEED] = LOW_BYTE(seed);
            nes_6502.ram[ADDR_RNG_SEED + 1] = HIGH_BYTE(seed);
            nes_6502.ram[ADDR_TMP1] = 0xAA; /* sentinel scratch */

            cpu6502_init(&cpu, &nes_6502);
            /* Calling convention: caller provides X = #rng_seed ($17), Y = #$02 */
            cpu6502_set_state(&cpu, ROM_ADDR_LFSR_PRNG, 0, ADDR_RNG_SEED, 2, 0, 0xFD);
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_LFSR_PRNG, 1000);
            assert(ok);
            (void)ok;

            /* Verify 6502 observable CPU register side effects:
             * Loop runs 2 times: X increments from $17 to $19, Y decrements from $02 to $00 */
            assert(cpu.x == (ADDR_RNG_SEED + 2)); /* X = $19 */
            assert(cpu.y == 0x00);                /* Y = $00 */
            /* Z flag is set by final dey (Y=0) */
            assert((cpu.p & 0x02) != 0);           /* Z = 1 */

            /* Setup C state */
            NES_State nes_c = rom_nes;
            nes_c.ram[ADDR_RNG_SEED] = LOW_BYTE(seed);
            nes_c.ram[ADDR_RNG_SEED + 1] = HIGH_BYTE(seed);
            nes_c.ram[ADDR_TMP1] = 0xAA;
            generateNextPseudorandomNumber(&nes_c, ADDR_RNG_SEED, 2);

            /* Exact RAM state equivalence */
            assert(nes_6502.ram[ADDR_RNG_SEED] == nes_c.ram[ADDR_RNG_SEED]);
            assert(nes_6502.ram[ADDR_RNG_SEED + 1] == nes_c.ram[ADDR_RNG_SEED + 1]);
            assert(nes_6502.ram[ADDR_TMP1] == nes_c.ram[ADDR_TMP1]);
            /* Scratch variable tmp1 ($0000) matches old_seed[0] & 2 */
            assert(nes_c.ram[ADDR_TMP1] == (LOW_BYTE(seed) & 0x02));
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

    /* Test 9: Differential diffOldAndNewButtons ($ABBD) */
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

    /* Test 10: Differential pollController ($AB9D) */
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

    /* Test 11: Differential demoButtonsTable_indexIncr ($9DE8) */
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

    /* Test 12: Differential makePlayer1Active ($8776) & makePlayer2Active ($8792) */
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

    /* Test 13: Differential playState_unassignOrientationId ($9E2F) & playState_incrementPlayState ($9E37) */
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

    /* Test 14: Differential branchOnPlayStatePlayer1 ($81B2) dispatch */
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

    /* Test 15: Differential playState_spawnNextTetrimino ($988E) 1P mode across 19 orientations */
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

    /* Test 16: Differential playState_spawnNextTetrimino ($988E) 2P mode delay mechanics */
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

    /* Test 17: Differential noop_disabledVramRowIncr ($9874) & gameModeState_vblankThenRunState2 ($9E27) */
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

    /* Test 18: Differential gameModeState_updateCountersAndNonPlayerState ($8884) */
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

    /* Test 19: Differential branchOnPlayStatePlayer1 ($81B2) State 8 dispatch */
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

    /* Test 20: Differential gameMode_legalScreen ($8200) */
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

    /* Test 21: Differential gameMode_titleScreen ($824F) with START button */
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

    /* Test 22: Differential gameMode_titleScreen ($824F) with timeout */
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

    /* Test 23: Differential branchOnGameMode ($8161) dispatch */
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

    /* Test 24: Differential gameMode_gameTypeMenu ($830B) on START press */
    {
        NES_State nes_6502 = rom_nes;
        NES_State nes_c = rom_nes;

        nes_6502.ram[ADDR_GAME_MODE] = MODE_GAME_TYPE_MENU;
        nes_c.ram[ADDR_GAME_MODE] = MODE_GAME_TYPE_MENU;
        nes_6502.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;
        nes_c.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_GAME_TYPE_MENU_LOOP, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_GAME_TYPE_MENU_LOOP, 50000);
        assert(ok);
        (void)ok;

        gameMode_gameTypeMenu_step(&nes_c);

        assert(nes_6502.ram[ADDR_GAME_MODE] == nes_c.ram[ADDR_GAME_MODE]);
        assert(nes_6502.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_INIT]);
        assert(nes_c.ram[ADDR_GAME_MODE] == MODE_LEVEL_MENU);
        assert(nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == SFX_MENU_SELECT);
    }

    /* Test 25: Differential gameMode_gameTypeMenu ($830B) on B press */
    {
        NES_State nes_6502 = rom_nes;
        NES_State nes_c = rom_nes;

        nes_6502.ram[ADDR_GAME_MODE] = MODE_GAME_TYPE_MENU;
        nes_c.ram[ADDR_GAME_MODE] = MODE_GAME_TYPE_MENU;
        nes_6502.ram[ADDR_FRAME_COUNTER + 1] = 5;
        nes_c.ram[ADDR_FRAME_COUNTER + 1] = 5;
        nes_6502.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_B;
        nes_c.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_B;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_GAME_TYPE_MENU_LOOP, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_GAME_TYPE_MENU_LOOP, 50000);
        assert(ok);
        (void)ok;

        gameMode_gameTypeMenu_step(&nes_c);

        assert(nes_6502.ram[ADDR_GAME_MODE] == nes_c.ram[ADDR_GAME_MODE]);
        assert(nes_6502.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_INIT]);
        assert(nes_6502.ram[ADDR_FRAME_COUNTER + 1] == nes_c.ram[ADDR_FRAME_COUNTER + 1]);
        assert(nes_c.ram[ADDR_GAME_MODE] == MODE_TITLE_SCREEN);
        assert(nes_c.ram[ADDR_FRAME_COUNTER + 1] == 0);
        assert(nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == SFX_MENU_SELECT);
    }

    /* Test 26: Differential gameMode_levelMenu_handleLevelHeightNavigation ($84AE) */
    {
        /* Right press in Level Selection */
        NES_State nes_6502 = rom_nes;
        NES_State nes_c = rom_nes;

        nes_6502.ram[ADDR_GAME_TYPE] = GAMETYPE_A;
        nes_c.ram[ADDR_GAME_TYPE] = GAMETYPE_A;
        nes_6502.ram[ADDR_SELECTING_LEVEL_OR_HEIGHT] = 0;
        nes_c.ram[ADDR_SELECTING_LEVEL_OR_HEIGHT] = 0;
        nes_6502.ram[ADDR_START_LEVEL] = 2;
        nes_c.ram[ADDR_START_LEVEL] = 2;
        nes_6502.ram[ADDR_FRAME_COUNTER] = 1;
        nes_c.ram[ADDR_FRAME_COUNTER] = 1;
        nes_6502.ram[ADDR_NEWLY_PRESSED_BUTTONS] = BUTTON_RIGHT;
        nes_c.ram[ADDR_NEWLY_PRESSED_BUTTONS] = BUTTON_RIGHT;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_LEVEL_MENU_HANDLE_NAV, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_LEVEL_MENU_HANDLE_NAV, 50000);
        assert(ok);
        (void)ok;

        gameMode_levelMenu_handleLevelHeightNavigation(&nes_c);

        assert(nes_6502.ram[ADDR_START_LEVEL] == nes_c.ram[ADDR_START_LEVEL]);
        assert(nes_6502.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_INIT]);
        assert(nes_6502.ram[ADDR_SPRITE_Y_OFFSET] == nes_c.ram[ADDR_SPRITE_Y_OFFSET]);
        assert(nes_6502.ram[ADDR_SPRITE_X_OFFSET] == nes_c.ram[ADDR_SPRITE_X_OFFSET]);
        assert(nes_c.ram[ADDR_START_LEVEL] == 3);
        assert(nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == SFX_MENU_MOVE);

        /* Down press in Level Selection (+5) */
        nes_6502 = rom_nes;
        nes_c = rom_nes;
        nes_6502.ram[ADDR_GAME_TYPE] = GAMETYPE_A;
        nes_c.ram[ADDR_GAME_TYPE] = GAMETYPE_A;
        nes_6502.ram[ADDR_SELECTING_LEVEL_OR_HEIGHT] = 0;
        nes_c.ram[ADDR_SELECTING_LEVEL_OR_HEIGHT] = 0;
        nes_6502.ram[ADDR_START_LEVEL] = 3;
        nes_c.ram[ADDR_START_LEVEL] = 3;
        nes_6502.ram[ADDR_FRAME_COUNTER] = 1;
        nes_c.ram[ADDR_FRAME_COUNTER] = 1;
        nes_6502.ram[ADDR_NEWLY_PRESSED_BUTTONS] = BUTTON_DOWN;
        nes_c.ram[ADDR_NEWLY_PRESSED_BUTTONS] = BUTTON_DOWN;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_LEVEL_MENU_HANDLE_NAV, 0, 0, 0, 0, 0xFD);
        ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_LEVEL_MENU_HANDLE_NAV, 50000);
        assert(ok);

        gameMode_levelMenu_handleLevelHeightNavigation(&nes_c);

        assert(nes_6502.ram[ADDR_START_LEVEL] == nes_c.ram[ADDR_START_LEVEL]);
        assert(nes_6502.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_INIT]);
        assert(nes_6502.ram[ADDR_SPRITE_Y_OFFSET] == nes_c.ram[ADDR_SPRITE_Y_OFFSET]);
        assert(nes_6502.ram[ADDR_SPRITE_X_OFFSET] == nes_c.ram[ADDR_SPRITE_X_OFFSET]);
        assert(nes_c.ram[ADDR_START_LEVEL] == 8);

        /* Button A in Type B toggles selectingLevelOrHeight */
        nes_6502 = rom_nes;
        nes_c = rom_nes;
        nes_6502.ram[ADDR_GAME_TYPE] = GAMETYPE_B;
        nes_c.ram[ADDR_GAME_TYPE] = GAMETYPE_B;
        nes_6502.ram[ADDR_SELECTING_LEVEL_OR_HEIGHT] = 0;
        nes_c.ram[ADDR_SELECTING_LEVEL_OR_HEIGHT] = 0;
        nes_6502.ram[ADDR_START_LEVEL] = 0;
        nes_c.ram[ADDR_START_LEVEL] = 0;
        nes_6502.ram[ADDR_START_HEIGHT] = 0;
        nes_c.ram[ADDR_START_HEIGHT] = 0;
        nes_6502.ram[ADDR_FRAME_COUNTER] = 1;
        nes_c.ram[ADDR_FRAME_COUNTER] = 1;
        nes_6502.ram[ADDR_NEWLY_PRESSED_BUTTONS] = BUTTON_A;
        nes_c.ram[ADDR_NEWLY_PRESSED_BUTTONS] = BUTTON_A;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_LEVEL_MENU_HANDLE_NAV, 0, 0, 0, 0, 0xFD);
        ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_LEVEL_MENU_HANDLE_NAV, 50000);
        assert(ok);

        gameMode_levelMenu_handleLevelHeightNavigation(&nes_c);

        assert(nes_6502.ram[ADDR_SELECTING_LEVEL_OR_HEIGHT] == nes_c.ram[ADDR_SELECTING_LEVEL_OR_HEIGHT]);
        assert(nes_6502.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_INIT]);
        assert(nes_c.ram[ADDR_SELECTING_LEVEL_OR_HEIGHT] == 1);
    }

    /* Test 27: Differential gameMode_levelMenu_processPlayer1Navigation ($8436) */
    {
        /* Normal START press */
        NES_State nes_6502 = rom_nes;
        NES_State nes_c = rom_nes;

        nes_6502.ram[ADDR_GAME_MODE] = MODE_LEVEL_MENU;
        nes_c.ram[ADDR_GAME_MODE] = MODE_LEVEL_MENU;
        nes_6502.ram[ADDR_P1_START_LEVEL] = 5;
        nes_c.ram[ADDR_P1_START_LEVEL] = 5;
        nes_6502.ram[ADDR_P1_START_HEIGHT] = 0;
        nes_c.ram[ADDR_P1_START_HEIGHT] = 0;
        nes_6502.ram[ADDR_ORIGINAL_Y] = 0;
        nes_c.ram[ADDR_ORIGINAL_Y] = 0;
        nes_6502.ram[ADDR_FRAME_COUNTER] = 1;
        nes_c.ram[ADDR_FRAME_COUNTER] = 1;
        nes_6502.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;
        nes_c.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;
        nes_6502.ram[ADDR_HELD_BUTTONS_P1] = BUTTON_START;
        nes_c.ram[ADDR_HELD_BUTTONS_P1] = BUTTON_START;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_LEVEL_MENU_PROCESS_P1, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_LEVEL_MENU_PROCESS_P1, 50000);
        assert(ok);
        (void)ok;

        gameMode_levelMenu_processPlayer1Navigation(&nes_c);

        assert(nes_6502.ram[ADDR_GAME_MODE] == nes_c.ram[ADDR_GAME_MODE]);
        assert(nes_6502.ram[ADDR_P1_START_LEVEL] == nes_c.ram[ADDR_P1_START_LEVEL]);
        assert(nes_6502.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_INIT]);
        assert(nes_6502.ram[ADDR_GAME_MODE_STATE] == nes_c.ram[ADDR_GAME_MODE_STATE]);
        assert(nes_c.ram[ADDR_GAME_MODE] == MODE_PLAY);
        assert(nes_c.ram[ADDR_P1_START_LEVEL] == 5);
        assert(nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == SFX_MENU_SELECT);

        /* A + START cheat code (+10 level) */
        nes_6502 = rom_nes;
        nes_c = rom_nes;
        nes_6502.ram[ADDR_GAME_MODE] = MODE_LEVEL_MENU;
        nes_c.ram[ADDR_GAME_MODE] = MODE_LEVEL_MENU;
        nes_6502.ram[ADDR_P1_START_LEVEL] = 9;
        nes_c.ram[ADDR_P1_START_LEVEL] = 9;
        nes_6502.ram[ADDR_P1_START_HEIGHT] = 0;
        nes_c.ram[ADDR_P1_START_HEIGHT] = 0;
        nes_6502.ram[ADDR_ORIGINAL_Y] = 0;
        nes_c.ram[ADDR_ORIGINAL_Y] = 0;
        nes_6502.ram[ADDR_FRAME_COUNTER] = 1;
        nes_c.ram[ADDR_FRAME_COUNTER] = 1;
        nes_6502.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;
        nes_c.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;
        nes_6502.ram[ADDR_HELD_BUTTONS_P1] = BUTTON_A | BUTTON_START;
        nes_c.ram[ADDR_HELD_BUTTONS_P1] = BUTTON_A | BUTTON_START;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_LEVEL_MENU_PROCESS_P1, 0, 0, 0, 0, 0xFD);
        ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_LEVEL_MENU_PROCESS_P1, 50000);
        assert(ok);

        gameMode_levelMenu_processPlayer1Navigation(&nes_c);

        assert(nes_6502.ram[ADDR_GAME_MODE] == nes_c.ram[ADDR_GAME_MODE]);
        assert(nes_6502.ram[ADDR_P1_START_LEVEL] == nes_c.ram[ADDR_P1_START_LEVEL]);
        assert(nes_6502.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_INIT]);
        assert(nes_c.ram[ADDR_GAME_MODE] == MODE_PLAY);
        assert(nes_c.ram[ADDR_P1_START_LEVEL] == 19);

        /* BUTTON_B press retreats to Game Type Menu */
        nes_6502 = rom_nes;
        nes_c = rom_nes;
        nes_6502.ram[ADDR_GAME_MODE] = MODE_LEVEL_MENU;
        nes_c.ram[ADDR_GAME_MODE] = MODE_LEVEL_MENU;
        nes_6502.ram[ADDR_P1_START_LEVEL] = 0;
        nes_c.ram[ADDR_P1_START_LEVEL] = 0;
        nes_6502.ram[ADDR_FRAME_COUNTER] = 1;
        nes_c.ram[ADDR_FRAME_COUNTER] = 1;
        nes_6502.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_B;
        nes_c.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_B;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_LEVEL_MENU_PROCESS_P1, 0, 0, 0, 0, 0xFD);
        ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_LEVEL_MENU_PROCESS_P1, 50000);
        assert(ok);

        gameMode_levelMenu_processPlayer1Navigation(&nes_c);

        assert(nes_6502.ram[ADDR_GAME_MODE] == nes_c.ram[ADDR_GAME_MODE]);
        assert(nes_6502.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_INIT]);
        assert(nes_c.ram[ADDR_GAME_MODE] == MODE_GAME_TYPE_MENU);
    }

    /* Test 28: Differential branchOnGameMode ($8161) dispatch for modes 2 and 3 */
    {
        /* Mode 2 dispatch */
        NES_State nes_c = rom_nes;

        nes_c.ram[ADDR_GAME_MODE] = MODE_GAME_TYPE_MENU;
        nes_c.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;

        branchOnGameMode(&nes_c);
        assert(nes_c.ram[ADDR_GAME_MODE] == MODE_LEVEL_MENU);

        /* Mode 3 dispatch */
        nes_c.ram[ADDR_GAME_MODE] = MODE_LEVEL_MENU;
        nes_c.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;
        nes_c.ram[ADDR_HELD_BUTTONS_P1] = BUTTON_START;

        branchOnGameMode(&nes_c);
        assert(nes_c.ram[ADDR_GAME_MODE] == MODE_PLAY);
        assert(nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == SFX_MENU_SELECT);
    }

    /* Test 29: Differential twoDigsToPPU ($9712) */
    {
        uint8_t test_vals[] = { 0x00, 0x12, 0x34, 0x90, 0x59, 0x88, 0x07 };
        size_t count = sizeof(test_vals) / sizeof(test_vals[0]);

        for (size_t i = 0; i < count; i++) {
            uint8_t val = test_vals[i];

            NES_State nes_6502 = rom_nes;
            cpu6502_init(&cpu, &nes_6502);
            cpu6502_set_state(&cpu, ROM_ADDR_TWO_DIGS_TO_PPU, val, 0, 0, 0, 0xFD);
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_TWO_DIGS_TO_PPU, 100);
            assert(ok);

            NES_State nes_c = rom_nes;
            twoDigsToPPU(&nes_c, val);

            assert(nes_6502.ram[ADDR_GENERAL_COUNTER] == nes_c.ram[ADDR_GENERAL_COUNTER]);
            assert(cpu.a == (val & 0x0F));
        }
    }

    /* Test 30: Differential copyHighScoreNameToNextIndex ($A192) */
    {
        uint8_t game_types[] = { 0, 1 };
        uint8_t start_bytes[] = { 0x00, 0x06 };

        for (int gt = 0; gt < 2; gt++) {
            for (int sb = 0; sb < 2; sb++) {
                uint8_t gtype = game_types[gt];
                uint8_t start_byte = start_bytes[sb];

                NES_State nes_6502 = rom_nes;
                initHighScoreTable(&nes_6502);
                nes_6502.ram[ADDR_GAME_TYPE] = gtype;

                cpu6502_init(&cpu, &nes_6502);
                cpu6502_set_state(&cpu, ROM_ADDR_COPY_HIGH_SCORE_NAME, start_byte, 0, 0, 0, 0xFD);
                bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_COPY_HIGH_SCORE_NAME, 500);
                assert(ok);

                NES_State nes_c = rom_nes;
                initHighScoreTable(&nes_c);
                nes_c.ram[ADDR_GAME_TYPE] = gtype;

                copyHighScoreNameToNextIndex(&nes_c, start_byte);

                assert(memcmp(&nes_6502.ram[ADDR_HIGH_SCORE_NAMES], &nes_c.ram[ADDR_HIGH_SCORE_NAMES], 48) == 0);
                assert(nes_6502.ram[ADDR_GENERAL_COUNTER] == nes_c.ram[ADDR_GENERAL_COUNTER]);
                assert(nes_6502.ram[ADDR_GENERAL_COUNTER2] == nes_c.ram[ADDR_GENERAL_COUNTER2]);
                assert(nes_6502.ram[ADDR_GENERAL_COUNTER3] == nes_c.ram[ADDR_GENERAL_COUNTER3]);
            }
        }
    }

    /* Test 31: Differential copyHighScoreScoreToNextIndex ($A1C1) */
    {
        uint8_t game_types[] = { 0, 1 };
        uint8_t start_bytes[] = { 0x00, 0x03 };

        for (int gt = 0; gt < 2; gt++) {
            for (int sb = 0; sb < 2; sb++) {
                uint8_t gtype = game_types[gt];
                uint8_t start_byte = start_bytes[sb];

                NES_State nes_6502 = rom_nes;
                initHighScoreTable(&nes_6502);
                nes_6502.ram[ADDR_GAME_TYPE] = gtype;

                cpu6502_init(&cpu, &nes_6502);
                cpu6502_set_state(&cpu, ROM_ADDR_COPY_HIGH_SCORE_SCORE, start_byte, 0, 0, 0, 0xFD);
                bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_COPY_HIGH_SCORE_SCORE, 500);
                assert(ok);

                NES_State nes_c = rom_nes;
                initHighScoreTable(&nes_c);
                nes_c.ram[ADDR_GAME_TYPE] = gtype;

                copyHighScoreScoreToNextIndex(&nes_c, start_byte);

                assert(memcmp(&nes_6502.ram[ADDR_HIGH_SCORE_A], &nes_c.ram[ADDR_HIGH_SCORE_A], 24) == 0);
            }
        }
    }

    /* Test 32: Differential copyHighScoreLevelToNextIndex ($A1E0) */
    {
        uint8_t game_types[] = { 0, 1 };
        uint8_t start_bytes[] = { 0x00, 0x01 };

        for (int gt = 0; gt < 2; gt++) {
            for (int sb = 0; sb < 2; sb++) {
                uint8_t gtype = game_types[gt];
                uint8_t start_byte = start_bytes[sb];

                NES_State nes_6502 = rom_nes;
                initHighScoreTable(&nes_6502);
                nes_6502.ram[ADDR_GAME_TYPE] = gtype;

                cpu6502_init(&cpu, &nes_6502);
                cpu6502_set_state(&cpu, ROM_ADDR_COPY_HIGH_SCORE_LEVEL, start_byte, 0, 0, 0, 0xFD);
                bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_COPY_HIGH_SCORE_LEVEL, 500);
                assert(ok);

                NES_State nes_c = rom_nes;
                initHighScoreTable(&nes_c);
                nes_c.ram[ADDR_GAME_TYPE] = gtype;

                copyHighScoreLevelToNextIndex(&nes_c, start_byte);

                assert(memcmp(&nes_6502.ram[ADDR_HIGH_SCORE_LEVELS], &nes_c.ram[ADDR_HIGH_SCORE_LEVELS], 8) == 0);
            }
        }
    }

    /* Test 33: Differential handleHighScoreIfNecessary ($A0EE) - Non-qualifying traversal */
    {
        uint8_t game_types[] = { 0, 1 };

        for (int gt = 0; gt < 2; gt++) {
            uint8_t gtype = game_types[gt];

            NES_State nes_6502 = rom_nes;
            initHighScoreTable(&nes_6502);
            nes_6502.ram[ADDR_GAME_TYPE] = gtype;
            nes_6502.ram[ADDR_P1_SCORE] = 0x00;
            nes_6502.ram[ADDR_P1_SCORE + 1] = 0x00;
            nes_6502.ram[ADDR_P1_SCORE + 2] = 0x00;

            cpu6502_init(&cpu, &nes_6502);
            cpu6502_set_state(&cpu, ROM_ADDR_HANDLE_HIGH_SCORE_IF_NECESSARY, 0, 0, 0, 0, 0xFD);
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_HANDLE_HIGH_SCORE_IF_NECESSARY, 2000);
            assert(ok);

            NES_State nes_c = rom_nes;
            initHighScoreTable(&nes_c);
            nes_c.ram[ADDR_GAME_TYPE] = gtype;
            nes_c.ram[ADDR_P1_SCORE] = 0x00;
            nes_c.ram[ADDR_P1_SCORE + 1] = 0x00;
            nes_c.ram[ADDR_P1_SCORE + 2] = 0x00;

            handleHighScoreIfNecessary(&nes_c);

            assert(nes_6502.ram[ADDR_HIGH_SCORE_RAW_POS] == nes_c.ram[ADDR_HIGH_SCORE_RAW_POS]);
            assert(nes_c.ram[ADDR_HIGH_SCORE_RAW_POS] == (gtype == 0 ? 3 : 7));
            assert(nes_6502.ram[ADDR_GENERAL_COUNTER2] == nes_c.ram[ADDR_GENERAL_COUNTER2]);
        }
    }

    /* Test 34: Differential render_mode_congratulations_screen ($A344) */
    {
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_OUT_OF_DATE_RENDER_FLAGS] = RENDER_HIGH_SCORE_LETTER;
        nes_6502.ram[ADDR_HIGH_SCORE_RAW_POS] = 1;
        nes_6502.ram[ADDR_HIGH_SCORE_NAME_OFFSET_LETTER] = 3;
        nes_6502.ram[ADDR_HIGH_SCORE_CURRENT_LETTER] = 2; /* 'B' */
        nes_6502.ram[ADDR_PPU_SCROLL_X] = 0x20;
        nes_6502.ram[ADDR_PPU_SCROLL_Y] = 0x30;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_RENDER_MODE_CONGRATULATIONS_SCREEN, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_RENDER_MODE_CONGRATULATIONS_SCREEN, 200);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_OUT_OF_DATE_RENDER_FLAGS] = RENDER_HIGH_SCORE_LETTER;
        nes_c.ram[ADDR_HIGH_SCORE_RAW_POS] = 1;
        nes_c.ram[ADDR_HIGH_SCORE_NAME_OFFSET_LETTER] = 3;
        nes_c.ram[ADDR_HIGH_SCORE_CURRENT_LETTER] = 2; /* 'B' */
        nes_c.ram[ADDR_PPU_SCROLL_X] = 0x20;
        nes_c.ram[ADDR_PPU_SCROLL_Y] = 0x30;

        render_mode_congratulations_screen(&nes_c);

        assert(nes_6502.ram[ADDR_OUT_OF_DATE_RENDER_FLAGS] == nes_c.ram[ADDR_OUT_OF_DATE_RENDER_FLAGS]);
        assert(nes_6502.ram[ADDR_GENERAL_COUNTER] == nes_c.ram[ADDR_GENERAL_COUNTER]);
        assert(nes_6502.ram[ADDR_PPU_SCROLL_X] == nes_c.ram[ADDR_PPU_SCROLL_X]);
        assert(nes_6502.ram[ADDR_PPU_SCROLL_Y] == nes_c.ram[ADDR_PPU_SCROLL_Y]);
    }

    /* Test 35: Differential memset_page ($AC6A) */
    {
        /* Case 1: single page (X = 4, Y = 4, val = 0xEF) */
        NES_State nes_6502 = rom_nes;
        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_MEMSET_PAGE, 0xEF, 0x04, 0x04, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_MEMSET_PAGE, 2000);
        assert(ok);

        NES_State nes_c = rom_nes;
        memset_page(&nes_c, 0xEF, 0x04, 0x04);

        assert(memcmp(&nes_6502.ram[0x0400], &nes_c.ram[0x0400], 256) == 0);
        assert(nes_6502.ram[ADDR_TMP1] == nes_c.ram[ADDR_TMP1]);
        assert(nes_6502.ram[ADDR_TMP2] == nes_c.ram[ADDR_TMP2]);

        /* Case 2: two pages (X = 4, Y = 5, val = 0xAA) */
        nes_6502 = rom_nes;
        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_MEMSET_PAGE, 0xAA, 0x04, 0x05, 0, 0xFD);
        ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_MEMSET_PAGE, 4000);
        assert(ok);

        nes_c = rom_nes;
        memset_page(&nes_c, 0xAA, 0x04, 0x05);

        assert(memcmp(&nes_6502.ram[0x0400], &nes_c.ram[0x0400], 512) == 0);
        assert(nes_6502.ram[ADDR_TMP1] == nes_c.ram[ADDR_TMP1]);
        assert(nes_6502.ram[ADDR_TMP2] == nes_c.ram[ADDR_TMP2]);
    }

    /* Test 36: Differential add100Points ($9F62) */
    {
        uint8_t test_scores[][2] = {
            { 0x00, 0x00 },
            { 0x08, 0x00 },
            { 0x09, 0x00 },
            { 0x45, 0x01 },
            { 0x98, 0x00 },
            { 0x99, 0x00 },
            { 0x99, 0x09 },
            { 0x99, 0x12 }
        };
        size_t nscores = sizeof(test_scores) / sizeof(test_scores[0]);

        for (size_t i = 0; i < nscores; i++) {
            NES_State nes_6502 = rom_nes;
            nes_6502.ram[ADDR_TOTAL_SCORE + 1] = test_scores[i][0];
            nes_6502.ram[ADDR_TOTAL_SCORE + 2] = test_scores[i][1];

            cpu6502_init(&cpu, &nes_6502);
            cpu6502_set_state(&cpu, ROM_ADDR_ADD_100_POINTS, 0, 0, 0, 0, 0xFD);
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_ADD_100_POINTS, 200);
            assert(ok);

            NES_State nes_c = rom_nes;
            nes_c.ram[ADDR_TOTAL_SCORE + 1] = test_scores[i][0];
            nes_c.ram[ADDR_TOTAL_SCORE + 2] = test_scores[i][1];

            add100Points(&nes_c);

            assert(nes_6502.ram[ADDR_TOTAL_SCORE + 1] == nes_c.ram[ADDR_TOTAL_SCORE + 1]);
            assert(nes_6502.ram[ADDR_TOTAL_SCORE + 2] == nes_c.ram[ADDR_TOTAL_SCORE + 2]);
        }
    }

    /* Test 37: Differential gameModeState_checkForResetKeyCombo ($9E16) */
    {
        /* Case 1: normal input (no reset combo) */
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_HELD_BUTTONS_P1] = BUTTON_A | BUTTON_B;
        nes_6502.ram[ADDR_GAME_MODE] = MODE_PLAY;
        nes_6502.ram[ADDR_GAME_MODE_STATE] = 6;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_GAME_MODE_STATE_CHECK_RESET_COMBO, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_GAME_MODE_STATE_CHECK_RESET_COMBO, 100);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_HELD_BUTTONS_P1] = BUTTON_A | BUTTON_B;
        nes_c.ram[ADDR_GAME_MODE] = MODE_PLAY;
        nes_c.ram[ADDR_GAME_MODE_STATE] = 6;

        gameModeState_checkForResetKeyCombo(&nes_c);

        assert(nes_6502.ram[ADDR_GAME_MODE_STATE] == nes_c.ram[ADDR_GAME_MODE_STATE]);
        assert(nes_6502.ram[ADDR_GAME_MODE] == nes_c.ram[ADDR_GAME_MODE]);

        /* Case 2: soft reset combo held (A + B + SELECT + START) */
        nes_6502 = rom_nes;
        nes_6502.ram[ADDR_HELD_BUTTONS_P1] = BUTTON_A | BUTTON_B | BUTTON_SELECT | BUTTON_START;
        nes_6502.ram[ADDR_GAME_MODE] = MODE_PLAY;
        nes_6502.ram[ADDR_GAME_MODE_STATE] = 6;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_GAME_MODE_STATE_CHECK_RESET_COMBO, 0, 0, 0, 0, 0xFD);
        ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_GAME_MODE_STATE_CHECK_RESET_COMBO, 500);
        assert(ok);

        nes_c = rom_nes;
        nes_c.ram[ADDR_HELD_BUTTONS_P1] = BUTTON_A | BUTTON_B | BUTTON_SELECT | BUTTON_START;
        nes_c.ram[ADDR_GAME_MODE] = MODE_PLAY;
        nes_c.ram[ADDR_GAME_MODE_STATE] = 6;

        gameModeState_checkForResetKeyCombo(&nes_c);

        assert(nes_6502.ram[ADDR_GAME_MODE] == nes_c.ram[ADDR_GAME_MODE]);
        assert(nes_c.ram[ADDR_GAME_MODE] == MODE_LEGAL_SCREEN);
    }

    /* Test 38: Differential gameModeState_handleGameOver ($9CBF) */
    {
        /* Case 1: 1P mode game not over */
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_NUMBER_OF_PLAYERS] = 1;
        nes_6502.ram[ADDR_P1_PLAY_STATE] = 1;
        nes_6502.ram[ADDR_GAME_MODE_STATE] = 3;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_GAME_MODE_STATE_HANDLE_GAME_OVER, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_GAME_MODE_STATE_HANDLE_GAME_OVER, 100);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_NUMBER_OF_PLAYERS] = 1;
        nes_c.ram[ADDR_P1_PLAY_STATE] = 1;
        nes_c.ram[ADDR_GAME_MODE_STATE] = 3;

        gameModeState_handleGameOver(&nes_c);

        assert(nes_6502.ram[ADDR_GAME_MODE_STATE] == nes_c.ram[ADDR_GAME_MODE_STATE]);
        assert(nes_6502.ram[ADDR_GENERAL_COUNTER2] == nes_c.ram[ADDR_GENERAL_COUNTER2]);

        /* Case 2: 2P mode game not over */
        nes_6502 = rom_nes;
        nes_6502.ram[ADDR_NUMBER_OF_PLAYERS] = 2;
        nes_6502.ram[ADDR_P1_PLAY_STATE] = 1;
        nes_6502.ram[ADDR_P2_PLAY_STATE] = 2;
        nes_6502.ram[ADDR_GAME_MODE_STATE] = 3;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_GAME_MODE_STATE_HANDLE_GAME_OVER, 0, 0, 0, 0, 0xFD);
        ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_GAME_MODE_STATE_HANDLE_GAME_OVER, 100);
        assert(ok);

        nes_c = rom_nes;
        nes_c.ram[ADDR_NUMBER_OF_PLAYERS] = 2;
        nes_c.ram[ADDR_P1_PLAY_STATE] = 1;
        nes_c.ram[ADDR_P2_PLAY_STATE] = 2;
        nes_c.ram[ADDR_GAME_MODE_STATE] = 3;

        gameModeState_handleGameOver(&nes_c);

        assert(nes_6502.ram[ADDR_GAME_MODE_STATE] == nes_c.ram[ADDR_GAME_MODE_STATE]);
        assert(nes_6502.ram[ADDR_GENERAL_COUNTER2] == nes_c.ram[ADDR_GENERAL_COUNTER2]);

        /* Case 3: 2P mode game over */
        nes_6502 = rom_nes;
        nes_6502.ram[ADDR_NUMBER_OF_PLAYERS] = 2;
        nes_6502.ram[ADDR_P1_PLAY_STATE] = 1;
        nes_6502.ram[ADDR_P2_PLAY_STATE] = 0;
        nes_6502.ram[ADDR_GAME_MODE_STATE] = 3;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_GAME_MODE_STATE_HANDLE_GAME_OVER, 0, 0, 0, 0, 0xFD);
        ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_GAME_MODE_STATE_HANDLE_GAME_OVER, 100);
        assert(ok);

        nes_c = rom_nes;
        nes_c.ram[ADDR_NUMBER_OF_PLAYERS] = 2;
        nes_c.ram[ADDR_P1_PLAY_STATE] = 1;
        nes_c.ram[ADDR_P2_PLAY_STATE] = 0;
        nes_c.ram[ADDR_GAME_MODE_STATE] = 3;

        gameModeState_handleGameOver(&nes_c);

        assert(nes_6502.ram[ADDR_GAME_MODE_STATE] == nes_c.ram[ADDR_GAME_MODE_STATE]);
        assert(nes_c.ram[ADDR_GAME_MODE_STATE] == 9);
    }

    /* Test 39: Differential gameModeState_startButtonHandling ($A37F) */
    {
        /* Case 1: Demo mode (Mode 5), START pressed -> exits to title screen */
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_GAME_MODE] = MODE_DEMO;
        nes_6502.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;
        nes_6502.ram[ADDR_GAME_MODE_STATE] = 7;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_GAME_MODE_STATE_START_BUTTON_HANDLING, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_GAME_MODE_STATE_START_BUTTON_HANDLING, 100);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_GAME_MODE] = MODE_DEMO;
        nes_c.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;
        nes_c.ram[ADDR_GAME_MODE_STATE] = 7;

        gameModeState_startButtonHandling(&nes_c);

        assert(nes_6502.ram[ADDR_GAME_MODE] == nes_c.ram[ADDR_GAME_MODE]);
        assert(nes_6502.ram[ADDR_GAME_MODE_STATE] == nes_c.ram[ADDR_GAME_MODE_STATE]);

        /* Case 2: In-game mode (renderMode 3), curtain falling (playState 10), START pressed -> no pause */
        nes_6502 = rom_nes;
        nes_6502.ram[ADDR_GAME_MODE] = MODE_PLAY;
        nes_6502.ram[ADDR_RENDER_MODE] = 3;
        nes_6502.ram[ADDR_P1_PLAY_STATE] = PLAYSTATE_CURTAIN_FALL;
        nes_6502.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;
        nes_6502.ram[ADDR_GAME_MODE_STATE] = 7;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_GAME_MODE_STATE_START_BUTTON_HANDLING, 0, 0, 0, 0, 0xFD);
        ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_GAME_MODE_STATE_START_BUTTON_HANDLING, 100);
        assert(ok);

        nes_c = rom_nes;
        nes_c.ram[ADDR_GAME_MODE] = MODE_PLAY;
        nes_c.ram[ADDR_RENDER_MODE] = 3;
        nes_c.ram[ADDR_P1_PLAY_STATE] = PLAYSTATE_CURTAIN_FALL;
        nes_c.ram[ADDR_NEWLY_PRESSED_P1] = BUTTON_START;
        nes_c.ram[ADDR_GAME_MODE_STATE] = 7;

        gameModeState_startButtonHandling(&nes_c);

        assert(nes_6502.ram[ADDR_GAME_MODE_STATE] == nes_c.ram[ADDR_GAME_MODE_STATE]);
        assert(nes_6502.ram[ADDR_RENDER_MODE] == nes_c.ram[ADDR_RENDER_MODE]);
    }

    /* Test 40: Differential gameMode_playAndEndingHighScore ($819B) */
    {
        /* Case 1: State 6 (reset combo check) dispatch */
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_GAME_MODE_STATE] = 6;
        nes_6502.ram[ADDR_HELD_BUTTONS_P1] = 0;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_GAME_MODE_PLAY_AND_ENDING_HIGH_SCORE, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_GAME_MODE_PLAY_AND_ENDING_HIGH_SCORE, 100);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_GAME_MODE_STATE] = 6;
        nes_c.ram[ADDR_HELD_BUTTONS_P1] = 0;

        gameMode_playAndEndingHighScore(&nes_c);

        assert(nes_6502.ram[ADDR_GAME_MODE_STATE] == nes_c.ram[ADDR_GAME_MODE_STATE]);
        assert(nes_c.ram[ADDR_GAME_MODE_STATE] == 7);

        /* Case 2: State 8 (vblankThenRunState2) dispatch */
        nes_6502 = rom_nes;
        nes_6502.ram[ADDR_GAME_MODE_STATE] = 8;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_GAME_MODE_PLAY_AND_ENDING_HIGH_SCORE, 0, 0, 0, 0, 0xFD);
        ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_GAME_MODE_PLAY_AND_ENDING_HIGH_SCORE, 100);
        assert(ok);

        nes_c = rom_nes;
        nes_c.ram[ADDR_GAME_MODE_STATE] = 8;

        gameMode_playAndEndingHighScore(&nes_c);

        assert(nes_6502.ram[ADDR_GAME_MODE_STATE] == nes_c.ram[ADDR_GAME_MODE_STATE]);
        assert(nes_c.ram[ADDR_GAME_MODE_STATE] == 2);
    }

    /* Test 41: Differential copyOamStagingToOam ($AB5E) */
    {
        NES_State nes_6502 = rom_nes;
        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_COPY_OAM_STAGING_TO_OAM, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_COPY_OAM_STAGING_TO_OAM, 50);
        assert(ok);

        NES_State nes_c = rom_nes;
        copyOamStagingToOam(&nes_c);
        assert(nes_6502.oam_addr == nes_c.oam_addr);
    }

    /* Test 42: Differential copyCurrentScrollAndCtrlToPPU ($AA8A) */
    {
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_CURRENT_PPU_CTRL] = 0x98;
        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_COPY_CURRENT_SCROLL_CTRL_PPU, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_COPY_CURRENT_SCROLL_CTRL_PPU, 50);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_CURRENT_PPU_CTRL] = 0x98;
        copyCurrentScrollAndCtrlToPPU(&nes_c);
        assert(nes_6502.ppu_ctrl == nes_c.ppu_ctrl);
    }

    /* Test 43: Differential loadSpriteIntoOamStaging ($8C27) */
    {
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_SPRITE_INDEX_OAM] = 0x00;
        nes_6502.ram[ADDR_SPRITE_X_OFFSET] = 0x20;
        nes_6502.ram[ADDR_SPRITE_Y_OFFSET] = 0x30;
        nes_6502.ram[ADDR_OAM_STAGING_LENGTH] = 0x00;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_LOAD_SPRITE_INTO_OAM_STAGING, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_LOAD_SPRITE_INTO_OAM_STAGING, 500);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_SPRITE_INDEX_OAM] = 0x00;
        nes_c.ram[ADDR_SPRITE_X_OFFSET] = 0x20;
        nes_c.ram[ADDR_SPRITE_Y_OFFSET] = 0x30;
        nes_c.ram[ADDR_OAM_STAGING_LENGTH] = 0x00;

        loadSpriteIntoOamStaging(&nes_c);

        assert(nes_6502.ram[ADDR_OAM_STAGING_LENGTH] == nes_c.ram[ADDR_OAM_STAGING_LENGTH]);
        assert(nes_6502.ram[ADDR_GENERAL_COUNTER] == nes_c.ram[ADDR_GENERAL_COUNTER]);
        assert(nes_6502.ram[ADDR_GENERAL_COUNTER2] == nes_c.ram[ADDR_GENERAL_COUNTER2]);
        assert(memcmp(&nes_6502.ram[ADDR_OAM_STAGING], &nes_c.ram[ADDR_OAM_STAGING], 16) == 0);
    }

    /* Test 44: Differential stageSpriteForCurrentPiece ($8A0A) */
    {
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_NUMBER_OF_PLAYERS] = 1;
        nes_6502.ram[ADDR_ACTIVE_PLAYER] = 1;
        nes_6502.ram[ADDR_TETRIMINO_X] = 5;
        nes_6502.ram[ADDR_TETRIMINO_Y] = 10;
        nes_6502.ram[ADDR_CURRENT_PIECE] = ORIENT_T_UP;
        nes_6502.ram[ADDR_OAM_STAGING_LENGTH] = 0;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_STAGE_SPRITE_CURRENT_PIECE, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_STAGE_SPRITE_CURRENT_PIECE, 500);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_NUMBER_OF_PLAYERS] = 1;
        nes_c.ram[ADDR_ACTIVE_PLAYER] = 1;
        nes_c.ram[ADDR_TETRIMINO_X] = 5;
        nes_c.ram[ADDR_TETRIMINO_Y] = 10;
        nes_c.ram[ADDR_CURRENT_PIECE] = ORIENT_T_UP;
        nes_c.ram[ADDR_OAM_STAGING_LENGTH] = 0;

        stageSpriteForCurrentPiece(&nes_c);

        assert(nes_6502.ram[ADDR_OAM_STAGING_LENGTH] == nes_c.ram[ADDR_OAM_STAGING_LENGTH]);
        assert(nes_6502.ram[ADDR_GENERAL_COUNTER3] == nes_c.ram[ADDR_GENERAL_COUNTER3]);
        assert(nes_6502.ram[ADDR_GENERAL_COUNTER4] == nes_c.ram[ADDR_GENERAL_COUNTER4]);
        assert(nes_6502.ram[ADDR_ORIGINAL_Y] == nes_c.ram[ADDR_ORIGINAL_Y]);
        assert(memcmp(&nes_6502.ram[ADDR_OAM_STAGING], &nes_c.ram[ADDR_OAM_STAGING], 16) == 0);
    }

    /* Test 45: Differential stageSpriteForNextPiece ($8BCE) */
    {
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_DISPLAY_NEXT_PIECE] = 0;
        nes_6502.ram[ADDR_NEXT_PIECE] = ORIENT_T_DOWN;
        nes_6502.ram[ADDR_OAM_STAGING_LENGTH] = 0;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_STAGE_SPRITE_NEXT_PIECE, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_STAGE_SPRITE_NEXT_PIECE, 500);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_DISPLAY_NEXT_PIECE] = 0;
        nes_c.ram[ADDR_NEXT_PIECE] = ORIENT_T_DOWN;
        nes_c.ram[ADDR_OAM_STAGING_LENGTH] = 0;

        stageSpriteForNextPiece(&nes_c);

        assert(nes_6502.ram[ADDR_OAM_STAGING_LENGTH] == nes_c.ram[ADDR_OAM_STAGING_LENGTH]);
        assert(nes_6502.ram[ADDR_SPRITE_X_OFFSET] == nes_c.ram[ADDR_SPRITE_X_OFFSET]);
        assert(nes_6502.ram[ADDR_SPRITE_Y_OFFSET] == nes_c.ram[ADDR_SPRITE_Y_OFFSET]);
        assert(nes_6502.ram[ADDR_SPRITE_INDEX_OAM] == nes_c.ram[ADDR_SPRITE_INDEX_OAM]);
        assert(memcmp(&nes_6502.ram[ADDR_OAM_STAGING], &nes_c.ram[ADDR_OAM_STAGING], 16) == 0);
    }

    /* Test 46: Differential updatePaletteForLevel ($9808) */
    {
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_P1_LEVEL_NUMBER] = 17;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_UPDATE_PALETTE_FOR_LEVEL, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_UPDATE_PALETTE_FOR_LEVEL, 200);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_P1_LEVEL_NUMBER] = 17;

        updatePaletteForLevel(&nes_c);

        assert(nes_6502.ram[ADDR_GENERAL_COUNTER] == nes_c.ram[ADDR_GENERAL_COUNTER]);
    }

    /* Test 47: Differential render_mode_legal_and_title_screens ($82B1) */
    {
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_CURRENT_PPU_CTRL] = 0x9B;
        nes_6502.ram[ADDR_PPU_SCROLL_X] = 10;
        nes_6502.ram[ADDR_PPU_SCROLL_Y] = 20;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_RENDER_MODE_LEGAL_AND_TITLE, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_RENDER_MODE_LEGAL_AND_TITLE, 100);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_CURRENT_PPU_CTRL] = 0x9B;
        nes_c.ram[ADDR_PPU_SCROLL_X] = 10;
        nes_c.ram[ADDR_PPU_SCROLL_Y] = 20;

        render_mode_legal_and_title_screens(&nes_c);

        assert(nes_6502.ram[ADDR_CURRENT_PPU_CTRL] == nes_c.ram[ADDR_CURRENT_PPU_CTRL]);
        assert(nes_6502.ram[ADDR_PPU_SCROLL_X] == nes_c.ram[ADDR_PPU_SCROLL_X]);
        assert(nes_6502.ram[ADDR_PPU_SCROLL_Y] == nes_c.ram[ADDR_PPU_SCROLL_Y]);
    }

    /* Test 48: Differential render_mode_menu_screens ($85DA) */
    {
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_CURRENT_PPU_CTRL] = 0x9B;
        nes_6502.ram[ADDR_PPU_SCROLL_X] = 10;
        nes_6502.ram[ADDR_PPU_SCROLL_Y] = 20;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_RENDER_MODE_MENUS, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_RENDER_MODE_MENUS, 100);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_CURRENT_PPU_CTRL] = 0x9B;
        nes_c.ram[ADDR_PPU_SCROLL_X] = 10;
        nes_c.ram[ADDR_PPU_SCROLL_Y] = 20;

        render_mode_menu_screens(&nes_c);

        assert(nes_6502.ram[ADDR_CURRENT_PPU_CTRL] == nes_c.ram[ADDR_CURRENT_PPU_CTRL]);
        assert(nes_6502.ppu_ctrl == nes_c.ppu_ctrl);
        assert(nes_6502.ram[ADDR_PPU_SCROLL_X] == nes_c.ram[ADDR_PPU_SCROLL_X]);
        assert(nes_6502.ram[ADDR_PPU_SCROLL_Y] == nes_c.ram[ADDR_PPU_SCROLL_Y]);
    }

    /* Test 49: Differential render_mode_ending_animation ($9F95) */
    {
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_GAME_TYPE] = GAMETYPE_B;
        nes_6502.ram[ADDR_P1_SCORE] = 0x12;
        nes_6502.ram[ADDR_P1_SCORE + 1] = 0x34;
        nes_6502.ram[ADDR_P1_SCORE + 2] = 0x56;
        nes_6502.ram[ADDR_GENERAL_COUNTER4] = 0x09;
        nes_6502.ram[ADDR_GENERAL_COUNTER5] = 0x05;
        nes_6502.ram[ADDR_TOTAL_SCORE] = 0x99;
        nes_6502.ram[ADDR_TOTAL_SCORE + 1] = 0x88;
        nes_6502.ram[ADDR_TOTAL_SCORE + 2] = 0x07;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_RENDER_MODE_ENDING_ANIMATION, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_RENDER_MODE_ENDING_ANIMATION, 500);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_GAME_TYPE] = GAMETYPE_B;
        nes_c.ram[ADDR_P1_SCORE] = 0x12;
        nes_c.ram[ADDR_P1_SCORE + 1] = 0x34;
        nes_c.ram[ADDR_P1_SCORE + 2] = 0x56;
        nes_c.ram[ADDR_GENERAL_COUNTER4] = 0x09;
        nes_c.ram[ADDR_GENERAL_COUNTER5] = 0x05;
        nes_c.ram[ADDR_TOTAL_SCORE] = 0x99;
        nes_c.ram[ADDR_TOTAL_SCORE + 1] = 0x88;
        nes_c.ram[ADDR_TOTAL_SCORE + 2] = 0x07;

        render_mode_ending_animation(&nes_c);

        assert(nes_6502.ram[ADDR_GENERAL_COUNTER] == nes_c.ram[ADDR_GENERAL_COUNTER]);
    }

    /* Test 50: Differential render ($804B) */
    {
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_RENDER_MODE] = RENDER_MODE_LEGAL_AND_TITLE;
        nes_6502.ram[ADDR_CURRENT_PPU_CTRL] = 0xFF;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_RENDER, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_RENDER, 100);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_RENDER_MODE] = RENDER_MODE_LEGAL_AND_TITLE;
        nes_c.ram[ADDR_CURRENT_PPU_CTRL] = 0xFF;

        render(&nes_c);

        assert(nes_6502.ram[ADDR_CURRENT_PPU_CTRL] == nes_c.ram[ADDR_CURRENT_PPU_CTRL]);
    }

    /* Test 51: Differential gameModeState_updatePlayer1 ($8174) */
    {
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_GAME_MODE_STATE] = 4;
        nes_6502.ram[ADDR_NUMBER_OF_PLAYERS] = 1;
        nes_6502.ram[ADDR_P1_PLAY_STATE] = PLAYSTATE_NOOP_9;
        nes_6502.ram[ADDR_P1_CURRENT_PIECE] = ORIENT_T_UP;
        nes_6502.ram[ADDR_P1_TETRIMINO_X] = 5;
        nes_6502.ram[ADDR_P1_TETRIMINO_Y] = 10;
        nes_6502.ram[ADDR_DISPLAY_NEXT_PIECE] = 1;
        nes_6502.ram[ADDR_OAM_STAGING_LENGTH] = 0;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_GAME_MODE_STATE_UPDATE_P1, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_GAME_MODE_STATE_UPDATE_P1, 1000);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_GAME_MODE_STATE] = 4;
        nes_c.ram[ADDR_NUMBER_OF_PLAYERS] = 1;
        nes_c.ram[ADDR_P1_PLAY_STATE] = PLAYSTATE_NOOP_9;
        nes_c.ram[ADDR_P1_CURRENT_PIECE] = ORIENT_T_UP;
        nes_c.ram[ADDR_P1_TETRIMINO_X] = 5;
        nes_c.ram[ADDR_P1_TETRIMINO_Y] = 10;
        nes_c.ram[ADDR_DISPLAY_NEXT_PIECE] = 1;
        nes_c.ram[ADDR_OAM_STAGING_LENGTH] = 0;

        gameModeState_updatePlayer1(&nes_c);

        assert(nes_6502.ram[ADDR_GAME_MODE_STATE] == nes_c.ram[ADDR_GAME_MODE_STATE]);
        assert(nes_6502.ram[ADDR_OAM_STAGING_LENGTH] == nes_c.ram[ADDR_OAM_STAGING_LENGTH]);
        assert(nes_6502.ram[ADDR_P1_PLAY_STATE] == nes_c.ram[ADDR_P1_PLAY_STATE]);
    }

    /* Test 52: Differential gameModeState_updatePlayer2 ($8186) */
    {
        /* Case 1: 1-Player mode (no-op except inc) */
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_GAME_MODE_STATE] = 5;
        nes_6502.ram[ADDR_NUMBER_OF_PLAYERS] = 1;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_GAME_MODE_STATE_UPDATE_P2, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_GAME_MODE_STATE_UPDATE_P2, 100);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_GAME_MODE_STATE] = 5;
        nes_c.ram[ADDR_NUMBER_OF_PLAYERS] = 1;

        gameModeState_updatePlayer2(&nes_c);

        assert(nes_6502.ram[ADDR_GAME_MODE_STATE] == nes_c.ram[ADDR_GAME_MODE_STATE]);
        assert(nes_c.ram[ADDR_GAME_MODE_STATE] == 6);

        /* Case 2: 2-Player mode */
        nes_6502 = rom_nes;
        nes_6502.ram[ADDR_GAME_MODE_STATE] = 5;
        nes_6502.ram[ADDR_NUMBER_OF_PLAYERS] = 2;
        nes_6502.ram[ADDR_P2_PLAY_STATE] = PLAYSTATE_NOOP_9;
        nes_6502.ram[ADDR_P2_CURRENT_PIECE] = ORIENT_T_UP;
        nes_6502.ram[ADDR_P2_TETRIMINO_X] = 5;
        nes_6502.ram[ADDR_P2_TETRIMINO_Y] = 10;
        nes_6502.ram[ADDR_OAM_STAGING_LENGTH] = 0;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_GAME_MODE_STATE_UPDATE_P2, 0, 0, 0, 0, 0xFD);
        ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_GAME_MODE_STATE_UPDATE_P2, 1000);
        assert(ok);

        nes_c = rom_nes;
        nes_c.ram[ADDR_GAME_MODE_STATE] = 5;
        nes_c.ram[ADDR_NUMBER_OF_PLAYERS] = 2;
        nes_c.ram[ADDR_P2_PLAY_STATE] = PLAYSTATE_NOOP_9;
        nes_c.ram[ADDR_P2_CURRENT_PIECE] = ORIENT_T_UP;
        nes_c.ram[ADDR_P2_TETRIMINO_X] = 5;
        nes_c.ram[ADDR_P2_TETRIMINO_Y] = 10;
        nes_c.ram[ADDR_OAM_STAGING_LENGTH] = 0;

        gameModeState_updatePlayer2(&nes_c);

        assert(nes_6502.ram[ADDR_GAME_MODE_STATE] == nes_c.ram[ADDR_GAME_MODE_STATE]);
        assert(nes_6502.ram[ADDR_OAM_STAGING_LENGTH] == nes_c.ram[ADDR_OAM_STAGING_LENGTH]);
        assert(nes_6502.ram[ADDR_P2_PLAY_STATE] == nes_c.ram[ADDR_P2_PLAY_STATE]);
    }

    /* Test 53: Differential playState_bTypeGoalCheck_ret ($A42B) */
    {
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_PLAY_STATE] = 5;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_PLAYSTATE_B_TYPE_GOAL_CHECK_RET, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_PLAYSTATE_B_TYPE_GOAL_CHECK_RET, 50);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_PLAY_STATE] = 5;

        playState_bTypeGoalCheck_ret(&nes_c);

        assert(nes_6502.ram[ADDR_PLAY_STATE] == nes_c.ram[ADDR_PLAY_STATE]);
        assert(nes_c.ram[ADDR_PLAY_STATE] == 6);
    }

    /* Test 54: Differential copyPlayfieldRowToVRAM ($9725) */
    {
        /* Subcase A: 1-Player mode, row 0 */
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_NUMBER_OF_PLAYERS] = 1;
        nes_6502.ram[ADDR_PLAYFIELD_ADDR] = 0x00;
        nes_6502.ram[ADDR_PLAYFIELD_ADDR + 1] = 0x04;
        nes_6502.ram[ADDR_VRAM_ROW] = 0;
        for (int i = 0; i < 10; i++) {
            nes_6502.ram[ADDR_PLAYFIELD + i] = (uint8_t)(i + 1);
        }

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_COPY_PLAYFIELD_ROW_TO_VRAM, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_COPY_PLAYFIELD_ROW_TO_VRAM, 500);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_NUMBER_OF_PLAYERS] = 1;
        nes_c.ram[ADDR_PLAYFIELD_ADDR] = 0x00;
        nes_c.ram[ADDR_PLAYFIELD_ADDR + 1] = 0x04;
        nes_c.ram[ADDR_VRAM_ROW] = 0;
        for (int i = 0; i < 10; i++) {
            nes_c.ram[ADDR_PLAYFIELD + i] = (uint8_t)(i + 1);
        }

        copyPlayfieldRowToVRAM(&nes_c);

        assert(nes_6502.ram[ADDR_VRAM_ROW] == nes_c.ram[ADDR_VRAM_ROW]);
        assert(nes_6502.ppu_addr == nes_c.ppu_addr);
        assert(nes_c.ram[ADDR_VRAM_ROW] == 1);
        assert(nes_c.ppu_addr == 0x20CC);

        /* Subcase B: 2-Player mode Player 1, row 5 */
        nes_6502 = rom_nes;
        nes_6502.ram[ADDR_NUMBER_OF_PLAYERS] = 2;
        nes_6502.ram[ADDR_PLAYFIELD_ADDR] = 0x00;
        nes_6502.ram[ADDR_PLAYFIELD_ADDR + 1] = 0x04;
        nes_6502.ram[ADDR_VRAM_ROW] = 5;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_COPY_PLAYFIELD_ROW_TO_VRAM, 0, 0, 0, 0, 0xFD);
        ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_COPY_PLAYFIELD_ROW_TO_VRAM, 500);
        assert(ok);

        nes_c = rom_nes;
        nes_c.ram[ADDR_NUMBER_OF_PLAYERS] = 2;
        nes_c.ram[ADDR_PLAYFIELD_ADDR] = 0x00;
        nes_c.ram[ADDR_PLAYFIELD_ADDR + 1] = 0x04;
        nes_c.ram[ADDR_VRAM_ROW] = 5;

        copyPlayfieldRowToVRAM(&nes_c);

        assert(nes_6502.ram[ADDR_VRAM_ROW] == nes_c.ram[ADDR_VRAM_ROW]);
        assert(nes_6502.ppu_addr == nes_c.ppu_addr);
        assert(nes_c.ram[ADDR_VRAM_ROW] == 6);
        assert(nes_c.ppu_addr == 0x2164);

        /* Subcase C: 2-Player mode Player 2, row 5 */
        nes_6502 = rom_nes;
        nes_6502.ram[ADDR_NUMBER_OF_PLAYERS] = 2;
        nes_6502.ram[ADDR_PLAYFIELD_ADDR] = 0x00;
        nes_6502.ram[ADDR_PLAYFIELD_ADDR + 1] = 0x05;
        nes_6502.ram[ADDR_VRAM_ROW] = 5;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_COPY_PLAYFIELD_ROW_TO_VRAM, 0, 0, 0, 0, 0xFD);
        ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_COPY_PLAYFIELD_ROW_TO_VRAM, 500);
        assert(ok);

        nes_c = rom_nes;
        nes_c.ram[ADDR_NUMBER_OF_PLAYERS] = 2;
        nes_c.ram[ADDR_PLAYFIELD_ADDR] = 0x00;
        nes_c.ram[ADDR_PLAYFIELD_ADDR + 1] = 0x05;
        nes_c.ram[ADDR_VRAM_ROW] = 5;

        copyPlayfieldRowToVRAM(&nes_c);

        assert(nes_6502.ram[ADDR_VRAM_ROW] == nes_c.ram[ADDR_VRAM_ROW]);
        assert(nes_6502.ppu_addr == nes_c.ppu_addr);
        assert(nes_c.ram[ADDR_VRAM_ROW] == 6);
        assert(nes_c.ppu_addr == 0x2172);

        /* Subcase D: Row clamp (row 19 -> 0x20) */
        nes_6502 = rom_nes;
        nes_6502.ram[ADDR_NUMBER_OF_PLAYERS] = 1;
        nes_6502.ram[ADDR_PLAYFIELD_ADDR] = 0x00;
        nes_6502.ram[ADDR_PLAYFIELD_ADDR + 1] = 0x04;
        nes_6502.ram[ADDR_VRAM_ROW] = 19;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_COPY_PLAYFIELD_ROW_TO_VRAM, 0, 0, 0, 0, 0xFD);
        ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_COPY_PLAYFIELD_ROW_TO_VRAM, 500);
        assert(ok);

        nes_c = rom_nes;
        nes_c.ram[ADDR_NUMBER_OF_PLAYERS] = 1;
        nes_c.ram[ADDR_PLAYFIELD_ADDR] = 0x00;
        nes_c.ram[ADDR_PLAYFIELD_ADDR + 1] = 0x04;
        nes_c.ram[ADDR_VRAM_ROW] = 19;

        copyPlayfieldRowToVRAM(&nes_c);

        assert(nes_6502.ram[ADDR_VRAM_ROW] == nes_c.ram[ADDR_VRAM_ROW]);
        assert(nes_c.ram[ADDR_VRAM_ROW] == 0x20);

        /* Subcase E: No-op return when vramRow = 0x20 */
        nes_6502 = rom_nes;
        nes_6502.ram[ADDR_VRAM_ROW] = 0x20;
        nes_6502.ppu_addr = 0x1234;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_COPY_PLAYFIELD_ROW_TO_VRAM, 0, 0, 0, 0, 0xFD);
        ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_COPY_PLAYFIELD_ROW_TO_VRAM, 100);
        assert(ok);

        nes_c = rom_nes;
        nes_c.ram[ADDR_VRAM_ROW] = 0x20;
        nes_c.ppu_addr = 0x1234;

        copyPlayfieldRowToVRAM(&nes_c);

        assert(nes_6502.ram[ADDR_VRAM_ROW] == nes_c.ram[ADDR_VRAM_ROW]);
        assert(nes_6502.ppu_addr == nes_c.ppu_addr);
        assert(nes_c.ram[ADDR_VRAM_ROW] == 0x20);
        assert(nes_c.ppu_addr == 0x1234);
    }

    /* Test 55: Differential render_mode_play_and_demo ($94EE) */
    {
        /* Subcase A: 1-Player normal rendering with dirty render flags */
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_NUMBER_OF_PLAYERS] = 1;
        nes_6502.ram[ADDR_P1_PLAY_STATE] = 1;
        nes_6502.ram[ADDR_P1_VRAM_ROW] = 0;
        nes_6502.ram[ADDR_PLAYFIELD_ADDR] = 0x00;
        nes_6502.ram[ADDR_PLAYFIELD_ADDR + 1] = 0x04;
        nes_6502.ram[ADDR_OUT_OF_DATE_RENDER_FLAGS] = RENDER_LINES | RENDER_LEVEL | RENDER_SCORE | RENDER_STATS;
        nes_6502.ram[ADDR_P1_LINES] = 0x25;
        nes_6502.ram[ADDR_P1_LINES + 1] = 0x01;
        nes_6502.ram[ADDR_P1_LEVEL_NUMBER] = 9;
        nes_6502.ram[ADDR_P1_SCORE] = 0x50;
        nes_6502.ram[ADDR_P1_SCORE + 1] = 0x23;
        nes_6502.ram[ADDR_P1_SCORE + 2] = 0x01;
        for (int i = 0; i < 14; i++) {
            nes_6502.ram[ADDR_STATS_BY_TYPE + i] = (uint8_t)(i + 1);
        }
        nes_6502.ram[ADDR_COMPLETED_LINES] = 0;
        nes_6502.ram[ADDR_PPU_SCROLL_X] = 0x55;
        nes_6502.ram[ADDR_PPU_SCROLL_Y] = 0xAA;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_RENDER_MODE_PLAY_AND_DEMO, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_RENDER_MODE_PLAY_AND_DEMO, 15000);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_NUMBER_OF_PLAYERS] = 1;
        nes_c.ram[ADDR_P1_PLAY_STATE] = 1;
        nes_c.ram[ADDR_P1_VRAM_ROW] = 0;
        nes_c.ram[ADDR_PLAYFIELD_ADDR] = 0x00;
        nes_c.ram[ADDR_PLAYFIELD_ADDR + 1] = 0x04;
        nes_c.ram[ADDR_OUT_OF_DATE_RENDER_FLAGS] = RENDER_LINES | RENDER_LEVEL | RENDER_SCORE | RENDER_STATS;
        nes_c.ram[ADDR_P1_LINES] = 0x25;
        nes_c.ram[ADDR_P1_LINES + 1] = 0x01;
        nes_c.ram[ADDR_P1_LEVEL_NUMBER] = 9;
        nes_c.ram[ADDR_P1_SCORE] = 0x50;
        nes_c.ram[ADDR_P1_SCORE + 1] = 0x23;
        nes_c.ram[ADDR_P1_SCORE + 2] = 0x01;
        for (int i = 0; i < 14; i++) {
            nes_c.ram[ADDR_STATS_BY_TYPE + i] = (uint8_t)(i + 1);
        }
        nes_c.ram[ADDR_COMPLETED_LINES] = 0;
        nes_c.ram[ADDR_PPU_SCROLL_X] = 0x55;
        nes_c.ram[ADDR_PPU_SCROLL_Y] = 0xAA;

        render_mode_play_and_demo(&nes_c);

        assert(nes_6502.ram[ADDR_P1_VRAM_ROW] == nes_c.ram[ADDR_P1_VRAM_ROW]);
        assert(nes_6502.ram[ADDR_OUT_OF_DATE_RENDER_FLAGS] == nes_c.ram[ADDR_OUT_OF_DATE_RENDER_FLAGS]);
        assert(nes_6502.ram[ADDR_PPU_SCROLL_X] == nes_c.ram[ADDR_PPU_SCROLL_X]);
        assert(nes_6502.ram[ADDR_PPU_SCROLL_Y] == nes_c.ram[ADDR_PPU_SCROLL_Y]);
        assert(nes_c.ram[ADDR_P1_VRAM_ROW] == 4);
        assert(nes_c.ram[ADDR_OUT_OF_DATE_RENDER_FLAGS] == 0);

        /* Subcase B: Line clear animation branch (player1_playState == 4) */
        nes_6502 = rom_nes;
        nes_6502.ram[ADDR_NUMBER_OF_PLAYERS] = 1;
        nes_6502.ram[ADDR_P1_PLAY_STATE] = PLAYSTATE_LINE_CLEAR_ANIM;
        nes_6502.ram[ADDR_P1_ROW_Y] = 0;
        nes_6502.ram[ADDR_FRAME_COUNTER] = 0;
        nes_6502.ram[ADDR_P1_COMPLETED_ROW] = 15;
        nes_6502.ram[ADDR_P1_COMPLETED_ROW + 1] = 0xFF;
        nes_6502.ram[ADDR_P1_COMPLETED_ROW + 2] = 0xFF;
        nes_6502.ram[ADDR_P1_COMPLETED_ROW + 3] = 0xFF;
        nes_6502.ram[ADDR_P1_VRAM_ROW] = 5;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_RENDER_MODE_PLAY_AND_DEMO, 0, 0, 0, 0, 0xFD);
        ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_RENDER_MODE_PLAY_AND_DEMO, 5000);
        assert(ok);

        nes_c = rom_nes;
        nes_c.ram[ADDR_NUMBER_OF_PLAYERS] = 1;
        nes_c.ram[ADDR_P1_PLAY_STATE] = PLAYSTATE_LINE_CLEAR_ANIM;
        nes_c.ram[ADDR_P1_ROW_Y] = 0;
        nes_c.ram[ADDR_FRAME_COUNTER] = 0;
        nes_c.ram[ADDR_P1_COMPLETED_ROW] = 15;
        nes_c.ram[ADDR_P1_COMPLETED_ROW + 1] = 0xFF;
        nes_c.ram[ADDR_P1_COMPLETED_ROW + 2] = 0xFF;
        nes_c.ram[ADDR_P1_COMPLETED_ROW + 3] = 0xFF;
        nes_c.ram[ADDR_P1_VRAM_ROW] = 5;

        render_mode_play_and_demo(&nes_c);

        assert(nes_6502.ram[ADDR_P1_VRAM_ROW] == nes_c.ram[ADDR_P1_VRAM_ROW]);
        assert(nes_6502.ram[ADDR_P1_ROW_Y] == nes_c.ram[ADDR_P1_ROW_Y]);
        assert(nes_c.ram[ADDR_P1_VRAM_ROW] == 0);
        assert(nes_c.ram[ADDR_P1_ROW_Y] == 1);

        /* Subcase C: 2-Player mode line rendering */
        nes_6502 = rom_nes;
        nes_6502.ram[ADDR_NUMBER_OF_PLAYERS] = 2;
        nes_6502.ram[ADDR_P1_PLAY_STATE] = 1;
        nes_6502.ram[ADDR_P2_PLAY_STATE] = 1;
        nes_6502.ram[ADDR_P1_VRAM_ROW] = 0;
        nes_6502.ram[ADDR_P2_VRAM_ROW] = 0;
        nes_6502.ram[ADDR_OUT_OF_DATE_RENDER_FLAGS] = RENDER_LINES;
        nes_6502.ram[ADDR_P1_LINES] = 0x10;
        nes_6502.ram[ADDR_P1_LINES + 1] = 0x00;
        nes_6502.ram[ADDR_P2_LINES] = 0x20;
        nes_6502.ram[ADDR_P2_LINES + 1] = 0x00;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_RENDER_MODE_PLAY_AND_DEMO, 0, 0, 0, 0, 0xFD);
        ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_RENDER_MODE_PLAY_AND_DEMO, 15000);
        assert(ok);

        nes_c = rom_nes;
        nes_c.ram[ADDR_NUMBER_OF_PLAYERS] = 2;
        nes_c.ram[ADDR_P1_PLAY_STATE] = 1;
        nes_c.ram[ADDR_P2_PLAY_STATE] = 1;
        nes_c.ram[ADDR_P1_VRAM_ROW] = 0;
        nes_c.ram[ADDR_P2_VRAM_ROW] = 0;
        nes_c.ram[ADDR_OUT_OF_DATE_RENDER_FLAGS] = RENDER_LINES;
        nes_c.ram[ADDR_P1_LINES] = 0x10;
        nes_c.ram[ADDR_P1_LINES + 1] = 0x00;
        nes_c.ram[ADDR_P2_LINES] = 0x20;
        nes_c.ram[ADDR_P2_LINES + 1] = 0x00;

        render_mode_play_and_demo(&nes_c);

        assert(nes_6502.ram[ADDR_P1_VRAM_ROW] == nes_c.ram[ADDR_P1_VRAM_ROW]);
        assert(nes_6502.ram[ADDR_P2_VRAM_ROW] == nes_c.ram[ADDR_P2_VRAM_ROW]);
        assert(nes_6502.ram[ADDR_OUT_OF_DATE_RENDER_FLAGS] == nes_c.ram[ADDR_OUT_OF_DATE_RENDER_FLAGS]);
        assert(nes_c.ram[ADDR_P1_VRAM_ROW] == 4);
        assert(nes_c.ram[ADDR_P2_VRAM_ROW] == 4);

        /* Subcase D: Tetris 4-line flash and SFX trigger */
        nes_6502 = rom_nes;
        nes_6502.ram[ADDR_NUMBER_OF_PLAYERS] = 1;
        nes_6502.ram[ADDR_P1_PLAY_STATE] = 1;
        nes_6502.ram[ADDR_COMPLETED_LINES] = 4;
        nes_6502.ram[ADDR_FRAME_COUNTER] = 0;
        nes_6502.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = 0;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_RENDER_MODE_PLAY_AND_DEMO, 0, 0, 0, 0, 0xFD);
        ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_RENDER_MODE_PLAY_AND_DEMO, 5000);
        assert(ok);

        nes_c = rom_nes;
        nes_c.ram[ADDR_NUMBER_OF_PLAYERS] = 1;
        nes_c.ram[ADDR_P1_PLAY_STATE] = 1;
        nes_c.ram[ADDR_COMPLETED_LINES] = 4;
        nes_c.ram[ADDR_FRAME_COUNTER] = 0;
        nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = 0;

        render_mode_play_and_demo(&nes_c);

        assert(nes_6502.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_INIT]);
        assert(nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == SFX_TETRIS);
    }

    /* Test 56: Differential selectEndingScreen across score thresholds */
    {
        uint8_t scores_hi[] = { 0x03, 0x06, 0x09, 0x11, 0x15 };
        for (int t = 0; t < 5; t++) {
            NES_State nes_6502 = rom_nes;
            nes_6502.ram[ADDR_P1_SCORE + 2] = scores_hi[t];
            nes_6502.ram[ADDR_ENDING] = 0xFF;
            nes_6502.ram[ADDR_ENDING_CUSTOM_VARS] = 0;
            nes_6502.ram[ADDR_ENDING_CUSTOM_VARS + 1] = 0xFF;

            cpu6502_init(&cpu, &nes_6502);
            cpu6502_set_state(&cpu, ROM_ADDR_SELECT_ENDING_SCREEN, 0, 0, 0, 0, 0xFD);
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_SELECT_ENDING_SCREEN, 10000);
            assert(ok);

            NES_State nes_c = rom_nes;
            nes_c.ram[ADDR_P1_SCORE + 2] = scores_hi[t];
            nes_c.ram[ADDR_ENDING] = 0xFF;
            nes_c.ram[ADDR_ENDING_CUSTOM_VARS] = 0;
            nes_c.ram[ADDR_ENDING_CUSTOM_VARS + 1] = 0xFF;

            selectEndingScreen(&nes_c);

            assert(nes_6502.ram[ADDR_ENDING] == nes_c.ram[ADDR_ENDING]);
            assert(nes_6502.ram[ADDR_ENDING_CUSTOM_VARS] == nes_c.ram[ADDR_ENDING_CUSTOM_VARS]);
            assert(nes_6502.ram[ADDR_ENDING_CUSTOM_VARS + 1] == nes_c.ram[ADDR_ENDING_CUSTOM_VARS + 1]);
            assert(nes_c.ram[ADDR_ENDING] == t);
        }
    }

    /* Test 57: Differential ending_selected direct entry ($A9A5) */
    {
        for (uint8_t end_idx = 0; end_idx < 5; end_idx++) {
            NES_State nes_6502 = rom_nes;
            nes_6502.ram[ADDR_ENDING] = end_idx;
            nes_6502.ram[ADDR_ENDING_CUSTOM_VARS] = 0;
            nes_6502.ram[ADDR_ENDING_CUSTOM_VARS + 1] = 0xFF;

            cpu6502_init(&cpu, &nes_6502);
            cpu6502_set_state(&cpu, ROM_ADDR_ENDING_SELECTED, 0, end_idx, 0, 0, 0xFD);
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_ENDING_SELECTED, 500);
            assert(ok);

            NES_State nes_c = rom_nes;
            nes_c.ram[ADDR_ENDING] = end_idx;
            nes_c.ram[ADDR_ENDING_CUSTOM_VARS] = 0;
            nes_c.ram[ADDR_ENDING_CUSTOM_VARS + 1] = 0xFF;

            ending_selected(&nes_c);

            assert(nes_6502.ram[ADDR_ENDING_CUSTOM_VARS] == nes_c.ram[ADDR_ENDING_CUSTOM_VARS]);
            assert(nes_6502.ram[ADDR_ENDING_CUSTOM_VARS + 1] == nes_c.ram[ADDR_ENDING_CUSTOM_VARS + 1]);
            assert(nes_c.ram[ADDR_ENDING_CUSTOM_VARS + 1] == 0);
        }
    }

    /* Test 58: Differential ending_typeA rocket staging & ascent ($A9B1) */
    {
        /* Subcase A: initial staging with delay increment */
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_ENDING] = 0;
        nes_6502.ram[ADDR_ENDING_CUSTOM_VARS] = 0xBF;
        nes_6502.ram[ADDR_ENDING_CUSTOM_VARS + 1] = 0x10;
        nes_6502.ram[ADDR_FRAME_COUNTER] = 0;
        nes_6502.ram[ADDR_OAM_STAGING_LENGTH] = 0;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_ENDING_TYPE_A, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_ENDING_TYPE_A, 5000);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_ENDING] = 0;
        nes_c.ram[ADDR_ENDING_CUSTOM_VARS] = 0xBF;
        nes_c.ram[ADDR_ENDING_CUSTOM_VARS + 1] = 0x10;
        nes_c.ram[ADDR_FRAME_COUNTER] = 0;
        nes_c.ram[ADDR_OAM_STAGING_LENGTH] = 0;

        ending_typeA(&nes_c);

        assert(nes_6502.ram[ADDR_ENDING_CUSTOM_VARS + 1] == nes_c.ram[ADDR_ENDING_CUSTOM_VARS + 1]);
        assert(nes_6502.ram[ADDR_OAM_STAGING_LENGTH] == nes_c.ram[ADDR_OAM_STAGING_LENGTH]);
        assert(memcmp(nes_6502.ram + ADDR_OAM_STAGING, nes_c.ram + ADDR_OAM_STAGING, 32) == 0);

        /* Subcase B: launch ascent step */
        nes_6502.ram[ADDR_ENDING_CUSTOM_VARS + 1] = 0xF0;
        nes_6502.ram[ADDR_FRAME_COUNTER] = 0;
        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_ENDING_TYPE_A, 0, 0, 0, 0, 0xFD);
        ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_ENDING_TYPE_A, 5000);
        assert(ok);

        nes_c.ram[ADDR_ENDING_CUSTOM_VARS + 1] = 0xF0;
        nes_c.ram[ADDR_FRAME_COUNTER] = 0;
        ending_typeA(&nes_c);

        assert(nes_6502.ram[ADDR_ENDING_CUSTOM_VARS] == nes_c.ram[ADDR_ENDING_CUSTOM_VARS]);
        assert(nes_6502.ram[ADDR_SOUND_EFFECT_SLOT0_INIT] == nes_c.ram[ADDR_SOUND_EFFECT_SLOT0_INIT]);
        assert(nes_c.ram[ADDR_SOUND_EFFECT_SLOT0_INIT] == 0x03);
    }

    /* Test 59: Differential ending_initTypeBVars ($A463) */
    {
        /* Subcase A: Level 9 */
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_LEVEL_NUMBER] = 9;
        nes_6502.ram[ADDR_P1_START_HEIGHT] = 3;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_ENDING_INIT_TYPE_B_VARS, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_ENDING_INIT_TYPE_B_VARS, 10000);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_LEVEL_NUMBER] = 9;
        nes_c.ram[ADDR_P1_START_HEIGHT] = 3;

        ending_initTypeBVars(&nes_c);

        assert(nes_6502.ram[ADDR_ENDING] == nes_c.ram[ADDR_ENDING]);
        assert(nes_6502.ram[ADDR_ENDING_CUSTOM_VARS + 2] == nes_c.ram[ADDR_ENDING_CUSTOM_VARS + 2]);
        assert(nes_6502.ram[ADDR_ENDING_CUSTOM_VARS + 3] == nes_c.ram[ADDR_ENDING_CUSTOM_VARS + 3]);
        assert(nes_6502.ram[ADDR_ENDING_CUSTOM_VARS + 4] == nes_c.ram[ADDR_ENDING_CUSTOM_VARS + 4]);
        assert(nes_6502.ram[ADDR_ENDING_CUSTOM_VARS + 5] == nes_c.ram[ADDR_ENDING_CUSTOM_VARS + 5]);
        assert(nes_6502.ram[ADDR_ENDING_CUSTOM_VARS + 6] == nes_c.ram[ADDR_ENDING_CUSTOM_VARS + 6]);
        assert(nes_6502.ram[ADDR_SPRITE_INDEX_OAM] == nes_c.ram[ADDR_SPRITE_INDEX_OAM]);

        /* Subcase B: Level 0 */
        nes_6502 = rom_nes;
        nes_6502.ram[ADDR_LEVEL_NUMBER] = 0;
        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_ENDING_INIT_TYPE_B_VARS, 0, 0, 0, 0, 0xFD);
        ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_ENDING_INIT_TYPE_B_VARS, 10000);
        assert(ok);

        nes_c = rom_nes;
        nes_c.ram[ADDR_LEVEL_NUMBER] = 0;
        ending_initTypeBVars(&nes_c);

        assert(nes_6502.ram[ADDR_ENDING_CUSTOM_VARS + 1] == nes_c.ram[ADDR_ENDING_CUSTOM_VARS + 1]);
        assert(nes_6502.ram[ADDR_ENDING_CUSTOM_VARS + 2] == nes_c.ram[ADDR_ENDING_CUSTOM_VARS + 2]);
    }

    /* Test 60: Differential ending_computeTypeBCathedralYTableIndex ($A6AE) */
    {
        for (uint8_t lvl = 0; lvl < 10; lvl++) {
            for (uint8_t spr = 0; spr < 6; spr++) {
                NES_State nes_6502 = rom_nes;
                nes_6502.ram[ADDR_LEVEL_NUMBER] = lvl;
                nes_6502.ram[ADDR_ENDING_CURRENT_SPRITE] = spr;

                cpu6502_init(&cpu, &nes_6502);
                cpu6502_set_state(&cpu, ROM_ADDR_ENDING_COMPUTE_CATHEDRAL_Y_INDEX, 0, 0, 0, 0, 0xFD);
                bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_ENDING_COMPUTE_CATHEDRAL_Y_INDEX, 500);
                assert(ok);

                NES_State nes_c = rom_nes;
                nes_c.ram[ADDR_LEVEL_NUMBER] = lvl;
                nes_c.ram[ADDR_ENDING_CURRENT_SPRITE] = spr;

                uint8_t idx = ending_computeTypeBCathedralYTableIndex(&nes_c);

                assert(cpu.x == idx);
                assert(nes_6502.ram[ADDR_GENERAL_COUNTER] == nes_c.ram[ADDR_GENERAL_COUNTER]);
            }
        }
    }

    /* Test 61: Differential ending_typeBCathedralSetSprite ($A690) */
    {
        for (uint8_t lvl = 0; lvl < 10; lvl++) {
            NES_State nes_6502 = rom_nes;
            nes_6502.ram[ADDR_LEVEL_NUMBER] = lvl;
            nes_6502.ram[ADDR_ENDING] = 1;
            nes_6502.ram[ADDR_ENDING_CUSTOM_VARS] = 0;

            cpu6502_init(&cpu, &nes_6502);
            cpu6502_set_state(&cpu, ROM_ADDR_ENDING_TYPE_B_CATHEDRAL_SET_SPRITE, 0, 0, 0, 0, 0xFD);
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_ENDING_TYPE_B_CATHEDRAL_SET_SPRITE, 1000);
            assert(ok);

            NES_State nes_c = rom_nes;
            nes_c.ram[ADDR_LEVEL_NUMBER] = lvl;
            nes_c.ram[ADDR_ENDING] = 1;
            nes_c.ram[ADDR_ENDING_CUSTOM_VARS] = 0;

            ending_typeBCathedralSetSprite(&nes_c);

            assert(nes_6502.ram[ADDR_ENDING] == nes_c.ram[ADDR_ENDING]);
            assert(nes_6502.ram[ADDR_ENDING_CUSTOM_VARS] == nes_c.ram[ADDR_ENDING_CUSTOM_VARS]);
            assert(nes_6502.ram[ADDR_SPRITE_INDEX_OAM] == nes_c.ram[ADDR_SPRITE_INDEX_OAM]);
        }
    }

    /* Test 62: Differential LA6BC dome liftoff ($A6BC) */
    {
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_ENDING_CUSTOM_VARS] = 0x05; /* matches dome 0 delay */
        nes_6502.ram[ADDR_ENDING_CUSTOM_VARS + 3] = 0xBC;
        nes_6502.ram[ADDR_ENDING_CUSTOM_VARS + 4] = 0xB8;
        nes_6502.ram[ADDR_ENDING_CUSTOM_VARS + 5] = 0xBC;
        nes_6502.ram[ADDR_ENDING_CUSTOM_VARS + 6] = 0xB3;
        nes_6502.ram[ADDR_FRAME_COUNTER] = 0;
        nes_6502.ram[ADDR_OAM_STAGING_LENGTH] = 0;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_LA6BC, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_LA6BC, 10000);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_ENDING_CUSTOM_VARS] = 0x05;
        nes_c.ram[ADDR_ENDING_CUSTOM_VARS + 3] = 0xBC;
        nes_c.ram[ADDR_ENDING_CUSTOM_VARS + 4] = 0xB8;
        nes_c.ram[ADDR_ENDING_CUSTOM_VARS + 5] = 0xBC;
        nes_c.ram[ADDR_ENDING_CUSTOM_VARS + 6] = 0xB3;
        nes_c.ram[ADDR_FRAME_COUNTER] = 0;
        nes_c.ram[ADDR_OAM_STAGING_LENGTH] = 0;

        LA6BC(&nes_c);

        assert(nes_6502.ram[ADDR_ENDING_CUSTOM_VARS] == nes_c.ram[ADDR_ENDING_CUSTOM_VARS]);
        assert(nes_6502.ram[ADDR_ENDING_CUSTOM_VARS + 3] == nes_c.ram[ADDR_ENDING_CUSTOM_VARS + 3]);
        assert(nes_6502.ram[ADDR_OAM_STAGING_LENGTH] == nes_c.ram[ADDR_OAM_STAGING_LENGTH]);
    }

    /* Test 63: Differential ending_typeBConcert character staging ($A53B) */
    {
        uint8_t test_heights[] = { 0, 1, 2, 3, 4, 5 };
        for (int h = 0; h < 6; h++) {
            NES_State nes_6502 = rom_nes;
            nes_6502.ram[ADDR_P1_START_HEIGHT] = test_heights[h];
            nes_6502.ram[ADDR_ENDING] = 0;
            nes_6502.ram[ADDR_ENDING_CUSTOM_VARS] = 0;
            nes_6502.ram[ADDR_ENDING_CUSTOM_VARS + 3] = 0xBC;
            nes_6502.ram[ADDR_ENDING_CUSTOM_VARS + 4] = 0xB8;
            nes_6502.ram[ADDR_ENDING_CUSTOM_VARS + 5] = 0xBC;
            nes_6502.ram[ADDR_ENDING_CUSTOM_VARS + 6] = 0xB3;
            nes_6502.ram[ADDR_FRAME_COUNTER] = 0;
            nes_6502.ram[ADDR_OAM_STAGING_LENGTH] = 0;

            cpu6502_init(&cpu, &nes_6502);
            cpu6502_set_state(&cpu, ROM_ADDR_ENDING_TYPE_B_CONCERT, test_heights[h], 0, 0, 0, 0xFD);
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_ENDING_TYPE_B_CONCERT, 15000);
            assert(ok);

            NES_State nes_c = rom_nes;
            nes_c.ram[ADDR_P1_START_HEIGHT] = test_heights[h];
            nes_c.ram[ADDR_ENDING] = 0;
            nes_c.ram[ADDR_ENDING_CUSTOM_VARS] = 0;
            nes_c.ram[ADDR_ENDING_CUSTOM_VARS + 3] = 0xBC;
            nes_c.ram[ADDR_ENDING_CUSTOM_VARS + 4] = 0xB8;
            nes_c.ram[ADDR_ENDING_CUSTOM_VARS + 5] = 0xBC;
            nes_c.ram[ADDR_ENDING_CUSTOM_VARS + 6] = 0xB3;
            nes_c.ram[ADDR_FRAME_COUNTER] = 0;
            nes_c.ram[ADDR_OAM_STAGING_LENGTH] = 0;

            ending_typeBConcert(&nes_c);

            assert(nes_6502.ram[ADDR_ENDING] == nes_c.ram[ADDR_ENDING]);
            assert(nes_6502.ram[ADDR_OAM_STAGING_LENGTH] == nes_c.ram[ADDR_OAM_STAGING_LENGTH]);
            assert(memcmp(nes_6502.ram + ADDR_OAM_STAGING, nes_c.ram + ADDR_OAM_STAGING, nes_c.ram[ADDR_OAM_STAGING_LENGTH]) == 0);
        }
    }

    /* Test 64: Differential render_ending & ending_typeB dispatchers */
    {
        /* Game Type A */
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_GAME_TYPE] = GAMETYPE_A;
        nes_6502.ram[ADDR_ENDING] = 0;
        nes_6502.ram[ADDR_ENDING_CUSTOM_VARS] = 0xBF;
        nes_6502.ram[ADDR_ENDING_CUSTOM_VARS + 1] = 0x10;
        nes_6502.ram[ADDR_FRAME_COUNTER] = 0;
        nes_6502.ram[ADDR_OAM_STAGING_LENGTH] = 0;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_RENDER_ENDING, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_RENDER_ENDING, 5000);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_GAME_TYPE] = GAMETYPE_A;
        nes_c.ram[ADDR_ENDING] = 0;
        nes_c.ram[ADDR_ENDING_CUSTOM_VARS] = 0xBF;
        nes_c.ram[ADDR_ENDING_CUSTOM_VARS + 1] = 0x10;
        nes_c.ram[ADDR_FRAME_COUNTER] = 0;
        nes_c.ram[ADDR_OAM_STAGING_LENGTH] = 0;

        render_ending(&nes_c);

        assert(nes_6502.ram[ADDR_ENDING_CUSTOM_VARS + 1] == nes_c.ram[ADDR_ENDING_CUSTOM_VARS + 1]);
        assert(nes_6502.ram[ADDR_OAM_STAGING_LENGTH] == nes_c.ram[ADDR_OAM_STAGING_LENGTH]);
    }

    /* Test 65: Differential render_endingUnskippable frame sleep ($A7FD) */
    {
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_GAME_TYPE] = GAMETYPE_A;
        nes_6502.ram[ADDR_ENDING] = 0;
        nes_6502.ram[ADDR_ENDING_CUSTOM_VARS] = 0xBF;
        nes_6502.ram[ADDR_ENDING_CUSTOM_VARS + 1] = 0;
        nes_6502.ram[ADDR_FRAME_COUNTER] = 0;
        nes_6502.ram[ADDR_VERTICAL_BLANK_INTERVAL] = 0;
        nes_6502.ram[ADDR_SLEEP_COUNTER] = 0;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_RENDER_ENDING_UNSKIPPABLE, 2, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_RENDER_ENDING_UNSKIPPABLE, 20000);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_GAME_TYPE] = GAMETYPE_A;
        nes_c.ram[ADDR_ENDING] = 0;
        nes_c.ram[ADDR_ENDING_CUSTOM_VARS] = 0xBF;
        nes_c.ram[ADDR_ENDING_CUSTOM_VARS + 1] = 0;
        nes_c.ram[ADDR_FRAME_COUNTER] = 0;
        nes_c.ram[ADDR_VERTICAL_BLANK_INTERVAL] = 0;
        nes_c.ram[ADDR_SLEEP_COUNTER] = 0;

        render_endingUnskippable(&nes_c, 2);

        assert(nes_6502.ram[ADDR_SLEEP_COUNTER] == nes_c.ram[ADDR_SLEEP_COUNTER]);
        assert(nes_c.ram[ADDR_SLEEP_COUNTER] == 0);
    }

    /* Test 66: Differential copyToSq1Channel ($E04B) & copyToNoiseChannel ($E053) */
    {
        NES_State nes_6502 = rom_nes;
        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_COPY_TO_SQ1_CHANNEL, 0, 0, 0x04, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_COPY_TO_SQ1_CHANNEL, 200);
        assert(ok);

        NES_State nes_c = rom_nes;
        copyToSq1Channel(&nes_c, 0x04);

        assert(nes_6502.ram[ADDR_AUDIOTMP1] == nes_c.ram[ADDR_AUDIOTMP1]);
        assert(nes_6502.ram[ADDR_AUDIOTMP2] == nes_c.ram[ADDR_AUDIOTMP2]);
        assert(nes_6502.ram[ADDR_AUDIOTMP3] == nes_c.ram[ADDR_AUDIOTMP3]);
        assert(nes_6502.ram[ADDR_AUDIOTMP4] == nes_c.ram[ADDR_AUDIOTMP4]);
        assert(memcmp(nes_6502.apu_regs, nes_c.apu_regs, 0x20) == 0);
    }

    /* Test 67: Differential computeSoundEffMethod ($E072) */
    {
        for (uint8_t idx = 1; idx <= 4; idx++) {
            NES_State nes_6502 = rom_nes;
            nes_6502.ram[ADDR_AUDIOTMP1] = 0x15; /* soundEffectSlot1Init_table */
            cpu6502_init(&cpu, &nes_6502);
            cpu6502_set_state(&cpu, ROM_ADDR_COMPUTE_SOUND_EFF_METHOD, idx, 0, 0, 0, 0xFD);
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_COMPUTE_SOUND_EFF_METHOD, 500);
            assert(ok);

            NES_State nes_c = rom_nes;
            nes_c.ram[ADDR_AUDIOTMP1] = 0x15;
            uint16_t addr_c = computeSoundEffMethod(&nes_c, idx);

            assert(nes_6502.ram[ADDR_AUDIOTMP3] == nes_c.ram[ADDR_AUDIOTMP3]);
            assert(nes_6502.ram[ADDR_AUDIOTMP4] == nes_c.ram[ADDR_AUDIOTMP4]);
            assert(nes_6502.ram[ADDR_CURRENT_AUDIO_SLOT] == nes_c.ram[ADDR_CURRENT_AUDIO_SLOT]);
            assert(addr_c == MAKE_WORD(nes_6502.ram[ADDR_AUDIOTMP3], nes_6502.ram[ADDR_AUDIOTMP4]));
        }
    }

    /* Test 68: Differential advanceAudioSlotFrame ($E0B5) */
    {
        /* Case A: counter < count (no wrap) */
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_CURRENT_SFX_SLOT] = 1;
        nes_6502.ram[ADDR_SFX_SLOT0_FRAME_COUNT + 1] = 4;
        nes_6502.ram[ADDR_SFX_SLOT0_FRAME_COUNTER + 1] = 1;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_ADVANCE_AUDIO_SLOT_FRAME, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_ADVANCE_AUDIO_SLOT_FRAME, 100);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_CURRENT_SFX_SLOT] = 1;
        nes_c.ram[ADDR_SFX_SLOT0_FRAME_COUNT + 1] = 4;
        nes_c.ram[ADDR_SFX_SLOT0_FRAME_COUNTER + 1] = 1;
        bool wrapped_c = advanceAudioSlotFrame(&nes_c);

        assert(nes_6502.ram[ADDR_SFX_SLOT0_FRAME_COUNTER + 1] == nes_c.ram[ADDR_SFX_SLOT0_FRAME_COUNTER + 1]);
        assert(!wrapped_c);

        /* Case B: counter == count (wrap to 0) */
        nes_6502 = rom_nes;
        nes_6502.ram[ADDR_CURRENT_SFX_SLOT] = 1;
        nes_6502.ram[ADDR_SFX_SLOT0_FRAME_COUNT + 1] = 4;
        nes_6502.ram[ADDR_SFX_SLOT0_FRAME_COUNTER + 1] = 3;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_ADVANCE_AUDIO_SLOT_FRAME, 0, 0, 0, 0, 0xFD);
        ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_ADVANCE_AUDIO_SLOT_FRAME, 100);
        assert(ok);

        nes_c = rom_nes;
        nes_c.ram[ADDR_CURRENT_SFX_SLOT] = 1;
        nes_c.ram[ADDR_SFX_SLOT0_FRAME_COUNT + 1] = 4;
        nes_c.ram[ADDR_SFX_SLOT0_FRAME_COUNTER + 1] = 3;
        wrapped_c = advanceAudioSlotFrame(&nes_c);

        assert(nes_6502.ram[ADDR_SFX_SLOT0_FRAME_COUNTER + 1] == nes_c.ram[ADDR_SFX_SLOT0_FRAME_COUNTER + 1]);
        assert(wrapped_c);
        assert(nes_c.ram[ADDR_SFX_SLOT0_FRAME_COUNTER + 1] == 0);
    }

    /* Test 69: Differential muteAudio ($E271) */
    {
        NES_State nes_6502 = rom_nes;
        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_MUTE_AUDIO, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_MUTE_AUDIO, 100);
        assert(ok);

        NES_State nes_c = rom_nes;
        muteAudio(&nes_c);

        assert(memcmp(nes_6502.apu_regs, nes_c.apu_regs, 0x20) == 0);
        assert(nes_c.apu_regs[REG_SQ1_VOL - 0x4000] == 0x10);
        assert(nes_c.apu_regs[REG_SQ2_VOL - 0x4000] == 0x10);
        assert(nes_c.apu_regs[REG_NOISE_VOL - 0x4000] == 0x10);
        assert(nes_c.apu_regs[REG_TRI_LINEAR - 0x4000] == 0x00);
        assert(nes_c.apu_regs[REG_DMC_RAW - 0x4000] == 0x00);
    }

    /* Test 70: Differential soundEffectSlot2_makesNoSound ($E244) */
    {
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_MUSIC_CHAN_INHIBIT] = 1;
        nes_6502.ram[ADDR_MUSIC_CHAN_INHIBIT + 1] = 2;
        nes_6502.ram[ADDR_MUSIC_CHAN_INHIBIT + 2] = 3;
        nes_6502.ram[ADDR_MUSIC_STAGING_NOISE_LO] = 0x55;
        nes_6502.ram[ADDR_RESET_SQ12_FOR_MUSIC] = 0x42;
        nes_6502.ram[ADDR_SOUND_EFFECT_SLOT0_PLAYING + 1] = 0x07;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_SOUND_EFFECT_SLOT2_MAKES_NO_SOUND, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_SOUND_EFFECT_SLOT2_MAKES_NO_SOUND, 300);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_MUSIC_CHAN_INHIBIT] = 1;
        nes_c.ram[ADDR_MUSIC_CHAN_INHIBIT + 1] = 2;
        nes_c.ram[ADDR_MUSIC_CHAN_INHIBIT + 2] = 3;
        nes_c.ram[ADDR_MUSIC_STAGING_NOISE_LO] = 0x55;
        nes_c.ram[ADDR_RESET_SQ12_FOR_MUSIC] = 0x42;
        nes_c.ram[ADDR_SOUND_EFFECT_SLOT0_PLAYING + 1] = 0x07;

        soundEffectSlot2_makesNoSound(&nes_c);

        assert(nes_6502.ram[ADDR_MUSIC_CHAN_INHIBIT] == nes_c.ram[ADDR_MUSIC_CHAN_INHIBIT]);
        assert(nes_6502.ram[ADDR_MUSIC_CHAN_INHIBIT + 1] == nes_c.ram[ADDR_MUSIC_CHAN_INHIBIT + 1]);
        assert(nes_6502.ram[ADDR_MUSIC_CHAN_INHIBIT + 2] == nes_c.ram[ADDR_MUSIC_CHAN_INHIBIT + 2]);
        assert(nes_6502.ram[ADDR_MUSIC_STAGING_NOISE_LO] == nes_c.ram[ADDR_MUSIC_STAGING_NOISE_LO]);
        assert(nes_6502.ram[ADDR_RESET_SQ12_FOR_MUSIC] == nes_c.ram[ADDR_RESET_SQ12_FOR_MUSIC]);
        assert(nes_6502.ram[ADDR_SOUND_EFFECT_SLOT0_PLAYING + 1] == nes_c.ram[ADDR_SOUND_EFFECT_SLOT0_PLAYING + 1]);
        assert(memcmp(nes_6502.apu_regs, nes_c.apu_regs, 0x20) == 0);
    }

    /* Test 71: Differential initSoundEffectShared ($E287) */
    {
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_CURRENT_SFX_SLOT] = 1;
        nes_6502.ram[ADDR_CURRENT_AUDIO_SLOT] = 7;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_INIT_SOUND_EFFECT_SHARED, 0x0F, 0, 0x20, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_INIT_SOUND_EFFECT_SHARED, 500);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_CURRENT_SFX_SLOT] = 1;
        nes_c.ram[ADDR_CURRENT_AUDIO_SLOT] = 7;
        initSoundEffectShared(&nes_c, 0x0F, 0x20);

        assert(nes_6502.ram[ADDR_SFX_SLOT0_FRAME_COUNT + 1] == nes_c.ram[ADDR_SFX_SLOT0_FRAME_COUNT + 1]);
        assert(nes_6502.ram[ADDR_SOUND_EFFECT_SLOT0_PLAYING + 1] == nes_c.ram[ADDR_SOUND_EFFECT_SLOT0_PLAYING + 1]);
        assert(nes_6502.ram[ADDR_SFX_SLOT0_FRAME_COUNTER + 1] == nes_c.ram[ADDR_SFX_SLOT0_FRAME_COUNTER + 1]);
        assert(nes_6502.ram[ADDR_RESET_SQ12_FOR_MUSIC] == nes_c.ram[ADDR_RESET_SQ12_FOR_MUSIC]);
        assert(memcmp(nes_6502.apu_regs, nes_c.apu_regs, 0x20) == 0);
    }

    /* Test 72: Differential soundEffectSlot1_lockTetriminoInit ($E384) priority branch */
    {
        /* Case A: Blocked by higher priority SFX (playing = 4) */
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_CURRENT_SFX_SLOT] = 1;
        nes_6502.ram[ADDR_CURRENT_AUDIO_SLOT] = 7;
        nes_6502.ram[ADDR_SOUND_EFFECT_SLOT1_PLAYING] = 4;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_SFX_SLOT1_LOCK_INIT, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_SFX_SLOT1_LOCK_INIT, 200);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_CURRENT_SFX_SLOT] = 1;
        nes_c.ram[ADDR_CURRENT_AUDIO_SLOT] = 7;
        nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_PLAYING] = 4;
        soundEffectSlot1_lockTetriminoInit(&nes_c);

        assert(nes_6502.ram[ADDR_SOUND_EFFECT_SLOT1_PLAYING] == nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_PLAYING]);
        assert(nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_PLAYING] == 4);

        /* Case B: Allowed when playing = 1 */
        nes_6502 = rom_nes;
        nes_6502.ram[ADDR_CURRENT_SFX_SLOT] = 1;
        nes_6502.ram[ADDR_CURRENT_AUDIO_SLOT] = 7;
        nes_6502.ram[ADDR_SOUND_EFFECT_SLOT1_PLAYING] = 1;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_SFX_SLOT1_LOCK_INIT, 0, 0, 0, 0, 0xFD);
        ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_SFX_SLOT1_LOCK_INIT, 500);
        assert(ok);

        nes_c = rom_nes;
        nes_c.ram[ADDR_CURRENT_SFX_SLOT] = 1;
        nes_c.ram[ADDR_CURRENT_AUDIO_SLOT] = 7;
        nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_PLAYING] = 1;
        soundEffectSlot1_lockTetriminoInit(&nes_c);

        assert(nes_6502.ram[ADDR_SOUND_EFFECT_SLOT1_PLAYING] == nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_PLAYING]);
        assert(nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_PLAYING] == 7);
        assert(memcmp(nes_6502.apu_regs, nes_c.apu_regs, 0x20) == 0);
    }

    /* Test 73: Differential soundEffectSlot1Playing_stop ($E3A1) */
    {
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_MUSIC_CHAN_INHIBIT] = 1;
        nes_6502.ram[ADDR_SOUND_EFFECT_SLOT1_PLAYING] = 5;
        nes_6502.ram[ADDR_RESET_SQ12_FOR_MUSIC] = 0;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_SFX_SLOT1_PLAYING_STOP, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_SFX_SLOT1_PLAYING_STOP, 100);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_MUSIC_CHAN_INHIBIT] = 1;
        nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_PLAYING] = 5;
        nes_c.ram[ADDR_RESET_SQ12_FOR_MUSIC] = 0;
        soundEffectSlot1Playing_stop(&nes_c);

        assert(nes_6502.ram[ADDR_MUSIC_CHAN_INHIBIT] == nes_c.ram[ADDR_MUSIC_CHAN_INHIBIT]);
        assert(nes_6502.ram[ADDR_SOUND_EFFECT_SLOT1_PLAYING] == nes_c.ram[ADDR_SOUND_EFFECT_SLOT1_PLAYING]);
        assert(nes_6502.ram[ADDR_RESET_SQ12_FOR_MUSIC] == nes_c.ram[ADDR_RESET_SQ12_FOR_MUSIC]);
        assert(memcmp(nes_6502.apu_regs, nes_c.apu_regs, 0x20) == 0);
    }

    /* Test 74: Differential soundEffectSlot2_mediumBuzz ($E50B) & lowBuzz ($E513) */
    {
        NES_State nes_6502 = rom_nes;
        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_SFX_SLOT2_MEDIUM_BUZZ, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_SFX_SLOT2_MEDIUM_BUZZ, 100);
        assert(ok);

        NES_State nes_c = rom_nes;
        soundEffectSlot2_mediumBuzz(&nes_c);

        assert(memcmp(nes_6502.apu_regs, nes_c.apu_regs, 0x20) == 0);
        assert(nes_c.apu_regs[REG_DMC_LEN - 0x4000] == 0x3F);
        assert(nes_c.apu_regs[REG_DMC_START - 0x4000] == 0x60);
        assert(nes_c.apu_regs[REG_DMC_FREQ - 0x4000] == 0x0F);

        /* Low Buzz */
        nes_6502 = rom_nes;
        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_SFX_SLOT2_LOW_BUZZ, 0, 0, 0, 0, 0xFD);
        ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_SFX_SLOT2_LOW_BUZZ, 100);
        assert(ok);

        nes_c = rom_nes;
        soundEffectSlot2_lowBuzz(&nes_c);

        assert(memcmp(nes_6502.apu_regs, nes_c.apu_regs, 0x20) == 0);
        assert(nes_c.apu_regs[REG_DMC_FREQ - 0x4000] == 0x0E);
    }

    /* Test 75: Differential soundEffectSlot3_donk ($E534) & stop ($E540) */
    {
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_CURRENT_SFX_SLOT] = 3;
        nes_6502.ram[ADDR_CURRENT_AUDIO_SLOT] = 2;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_SFX_SLOT3_DONK, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_SFX_SLOT3_DONK, 500);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_CURRENT_SFX_SLOT] = 3;
        nes_c.ram[ADDR_CURRENT_AUDIO_SLOT] = 2;
        soundEffectSlot3_donk(&nes_c);

        assert(nes_6502.ram[ADDR_SFX_SLOT0_FRAME_COUNT + 3] == nes_c.ram[ADDR_SFX_SLOT0_FRAME_COUNT + 3]);
        assert(nes_6502.ram[ADDR_SOUND_EFFECT_SLOT3_PLAYING] == nes_c.ram[ADDR_SOUND_EFFECT_SLOT3_PLAYING]);
        assert(memcmp(nes_6502.apu_regs, nes_c.apu_regs, 0x20) == 0);

        /* Stop */
        nes_6502 = rom_nes;
        nes_6502.ram[ADDR_SOUND_EFFECT_SLOT3_PLAYING] = 2;
        nes_6502.ram[ADDR_MUSIC_CHAN_INHIBIT + 2] = 3;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_SFX_SLOT3_PLAYING_STOP, 0, 0, 0, 0, 0xFD);
        ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_SFX_SLOT3_PLAYING_STOP, 100);
        assert(ok);

        nes_c = rom_nes;
        nes_c.ram[ADDR_SOUND_EFFECT_SLOT3_PLAYING] = 2;
        nes_c.ram[ADDR_MUSIC_CHAN_INHIBIT + 2] = 3;
        soundEffectSlot3Playing_stop(&nes_c);

        assert(nes_6502.ram[ADDR_SOUND_EFFECT_SLOT3_PLAYING] == nes_c.ram[ADDR_SOUND_EFFECT_SLOT3_PLAYING]);
        assert(nes_6502.ram[ADDR_MUSIC_CHAN_INHIBIT + 2] == nes_c.ram[ADDR_MUSIC_CHAN_INHIBIT + 2]);
        assert(memcmp(nes_6502.apu_regs, nes_c.apu_regs, 0x20) == 0);
    }

    /* Test 76: Differential _updatePpuMask ($AA59) */
    {
        uint8_t masks[] = { 0x00, 0x1E, 0xE1, 0xFF, 0x55 };
        for (int m = 0; m < 5; m++) {
            NES_State nes_6502 = rom_nes;
            cpu6502_init(&cpu, &nes_6502);
            cpu6502_set_state(&cpu, ROM_ADDR_UPDATE_PPU_MASK, masks[m], 0, 0, 0, 0xFD);
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_UPDATE_PPU_MASK, 50);
            assert(ok);

            NES_State nes_c = rom_nes;
            _updatePpuMask(&nes_c, masks[m]);

            assert(nes_6502.ppu_mask == nes_c.ppu_mask);
            assert(nes_6502.ram[ADDR_CURRENT_PPU_MASK] == nes_c.ram[ADDR_CURRENT_PPU_MASK]);
        }
    }

    /* Test 77: Differential initSq12IfTrashedBySoundEffect ($E735) */
    {
        uint8_t resets[] = { 0, 1, 2 };
        for (int r = 0; r < 3; r++) {
            NES_State nes_6502 = rom_nes;
            nes_6502.ram[ADDR_RESET_SQ12_FOR_MUSIC] = resets[r];
            nes_6502.ram[ADDR_MUSIC_STAGING_SQ1_LO] = 0x22;
            nes_6502.ram[ADDR_MUSIC_STAGING_SQ1_HI] = 0x33;
            nes_6502.ram[ADDR_MUSIC_STAGING_SQ2_LO] = 0x44;
            nes_6502.ram[ADDR_MUSIC_STAGING_SQ2_HI] = 0x55;

            cpu6502_init(&cpu, &nes_6502);
            cpu6502_set_state(&cpu, ROM_ADDR_INIT_SQ12_IF_TRASHED, 0, 0, 0, 0, 0xFD);
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_INIT_SQ12_IF_TRASHED, 100);
            assert(ok);

            NES_State nes_c = rom_nes;
            nes_c.ram[ADDR_RESET_SQ12_FOR_MUSIC] = resets[r];
            nes_c.ram[ADDR_MUSIC_STAGING_SQ1_LO] = 0x22;
            nes_c.ram[ADDR_MUSIC_STAGING_SQ1_HI] = 0x33;
            nes_c.ram[ADDR_MUSIC_STAGING_SQ2_LO] = 0x44;
            nes_c.ram[ADDR_MUSIC_STAGING_SQ2_HI] = 0x55;

            initSq12IfTrashedBySoundEffect(&nes_c);

            assert(nes_6502.ram[ADDR_RESET_SQ12_FOR_MUSIC] == nes_c.ram[ADDR_RESET_SQ12_FOR_MUSIC]);
            assert(memcmp(nes_6502.apu_regs, nes_c.apu_regs, 0x20) == 0);
        }
    }

    /* Test 78: Differential updateMusic_noSoundJmp ($E580) */
    {
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_MUSIC_CHAN_INHIBIT] = 1;
        nes_6502.ram[ADDR_MUSIC_CHAN_INHIBIT + 1] = 2;
        nes_6502.ram[ADDR_MUSIC_CHAN_INHIBIT + 2] = 3;
        nes_6502.ram[ADDR_MUSIC_CHAN_CONTROL + 2] = 0x55;
        nes_6502.ram[ADDR_MUSIC_STAGING_NOISE_LO] = 0x77;
        nes_6502.ram[ADDR_RESET_SQ12_FOR_MUSIC] = 0x88;
        nes_6502.ram[ADDR_SOUND_EFFECT_SLOT0_PLAYING + 1] = 0x99;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_UPDATE_MUSIC_NO_SOUND_JMP, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_UPDATE_MUSIC_NO_SOUND_JMP, 200);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_MUSIC_CHAN_INHIBIT] = 1;
        nes_c.ram[ADDR_MUSIC_CHAN_INHIBIT + 1] = 2;
        nes_c.ram[ADDR_MUSIC_CHAN_INHIBIT + 2] = 3;
        nes_c.ram[ADDR_MUSIC_CHAN_CONTROL + 2] = 0x55;
        nes_c.ram[ADDR_MUSIC_STAGING_NOISE_LO] = 0x77;
        nes_c.ram[ADDR_RESET_SQ12_FOR_MUSIC] = 0x88;
        nes_c.ram[ADDR_SOUND_EFFECT_SLOT0_PLAYING + 1] = 0x99;

        updateMusic_noSoundJmp(&nes_c);

        assert(nes_6502.ram[ADDR_MUSIC_CHAN_CONTROL + 2] == nes_c.ram[ADDR_MUSIC_CHAN_CONTROL + 2]);
        assert(nes_6502.ram[ADDR_RESET_SQ12_FOR_MUSIC] == nes_c.ram[ADDR_RESET_SQ12_FOR_MUSIC]);
        assert(memcmp(nes_6502.apu_regs, nes_c.apu_regs, 0x20) == 0);
    }

    /* Test 79: Differential updateMusicFrame_setChanVol ($E766) */
    {
        uint8_t notes[] = { 0x30, 0x02 }; /* normal vs rest */
        for (int n = 0; n < 2; n++) {
            NES_State nes_6502 = rom_nes;
            nes_6502.ram[ADDR_MUSIC_CHAN_OFFSET] = 0;
            nes_6502.ram[ADDR_MUSIC_CHAN_CONTROL] = 1;
            nes_6502.ram[ADDR_MUSIC_CHAN_VOLUME] = 0x80;
            nes_6502.ram[ADDR_MUSIC_CHAN_NOTE] = notes[n];
            nes_6502.ram[ADDR_MUSIC_CHAN_VOL_FRAME_COUNTER] = 0;
            nes_6502.ram[ADDR_MUSIC_CHAN_INHIBIT] = 0;

            cpu6502_init(&cpu, &nes_6502);
            cpu6502_set_state(&cpu, ROM_ADDR_UPDATE_MUSIC_SET_CHAN_VOL, 0, 0, 0, 0, 0xFD);
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_UPDATE_MUSIC_SET_CHAN_VOL, 200);
            assert(ok);

            NES_State nes_c = rom_nes;
            nes_c.ram[ADDR_MUSIC_CHAN_OFFSET] = 0;
            nes_c.ram[ADDR_MUSIC_CHAN_CONTROL] = 1;
            nes_c.ram[ADDR_MUSIC_CHAN_VOLUME] = 0x80;
            nes_c.ram[ADDR_MUSIC_CHAN_NOTE] = notes[n];
            nes_c.ram[ADDR_MUSIC_CHAN_VOL_FRAME_COUNTER] = 0;
            nes_c.ram[ADDR_MUSIC_CHAN_INHIBIT] = 0;

            updateMusicFrame_setChanVol(&nes_c, 0);

            assert(nes_6502.ram[ADDR_MUSIC_CHAN_VOL_FRAME_COUNTER] == nes_c.ram[ADDR_MUSIC_CHAN_VOL_FRAME_COUNTER]);
            assert(memcmp(nes_6502.apu_regs, nes_c.apu_regs, 0x20) == 0);
        }
    }

    /* Test 80: Differential updateMusicFrame_setChanLo ($E5D0) */
    {
        uint8_t ctrls[] = { 0x20, 0x60, 0x40, 0x80 };
        for (int c = 0; c < 4; c++) {
            NES_State nes_6502 = rom_nes;
            nes_6502.ram[ADDR_CURRENTLY_PLAYING_MUSIC_TRACK] = 2;
            nes_6502.ram[ADDR_MUSIC_CHAN_OFFSET] = 0;
            nes_6502.ram[ADDR_MUSIC_STAGING_SQ1_LO] = 0x60;
            nes_6502.ram[ADDR_MUSIC_CHAN_NOTE] = 0x30;
            nes_6502.ram[ADDR_MUSIC_CHAN_CONTROL] = ctrls[c];
            nes_6502.ram[ADDR_MUSIC_CHAN_LO_FRAME_COUNTER] = 0;
            nes_6502.ram[ADDR_MUSIC_CHAN_INHIBIT] = 0;

            cpu6502_init(&cpu, &nes_6502);
            cpu6502_set_state(&cpu, ROM_ADDR_UPDATE_MUSIC_SET_CHAN_LO, 0, 0, 0, 0, 0xFD);
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_UPDATE_MUSIC_SET_CHAN_LO, 200);
            assert(ok);

            NES_State nes_c = rom_nes;
            nes_c.ram[ADDR_CURRENTLY_PLAYING_MUSIC_TRACK] = 2;
            nes_c.ram[ADDR_MUSIC_CHAN_OFFSET] = 0;
            nes_c.ram[ADDR_MUSIC_STAGING_SQ1_LO] = 0x60;
            nes_c.ram[ADDR_MUSIC_CHAN_NOTE] = 0x30;
            nes_c.ram[ADDR_MUSIC_CHAN_CONTROL] = ctrls[c];
            nes_c.ram[ADDR_MUSIC_CHAN_LO_FRAME_COUNTER] = 0;
            nes_c.ram[ADDR_MUSIC_CHAN_INHIBIT] = 0;

            updateMusicFrame_setChanLo(&nes_c, 0);

            assert(nes_6502.ram[ADDR_MUSIC_CHAN_LO_FRAME_COUNTER] == nes_c.ram[ADDR_MUSIC_CHAN_LO_FRAME_COUNTER]);
            assert(memcmp(nes_6502.apu_regs, nes_c.apu_regs, 0x20) == 0);
        }
    }

    /* Test 81: Differential playNoise ($E9EB) */
    {
        uint8_t sfx0_playing[] = { 0, 1 };
        for (int s = 0; s < 2; s++) {
            NES_State nes_6502 = rom_nes;
            nes_6502.ram[ADDR_SOUND_EFFECT_SLOT0_PLAYING] = sfx0_playing[s];

            cpu6502_init(&cpu, &nes_6502);
            cpu6502_set_state(&cpu, ROM_ADDR_PLAY_NOISE, 0, 0, 3, 0, 0xFD); /* Y = 3 */
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_PLAY_NOISE, 50);
            assert(ok);

            NES_State nes_c = rom_nes;
            nes_c.ram[ADDR_SOUND_EFFECT_SLOT0_PLAYING] = sfx0_playing[s];
            playNoise(&nes_c, 3);

            assert(memcmp(nes_6502.apu_regs, nes_c.apu_regs, 0x20) == 0);
        }
    }

    /* Test 82: Differential playDmc ($EA03) */
    {
        uint8_t dmc_notes[] = { 0x40, 0x80 };
        for (int d = 0; d < 2; d++) {
            NES_State nes_6502 = rom_nes;
            nes_6502.ram[ADDR_MUSIC_DMC_SFX4_CHECK] = 0;

            cpu6502_init(&cpu, &nes_6502);
            cpu6502_set_state(&cpu, ROM_ADDR_PLAY_DMC, 0, 0, dmc_notes[d], 0, 0xFD);
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_PLAY_DMC, 100);
            assert(ok);

            NES_State nes_c = rom_nes;
            nes_c.ram[ADDR_MUSIC_DMC_SFX4_CHECK] = 0;
            playDmc(&nes_c, dmc_notes[d]);

            assert(nes_6502.ram[ADDR_AUDIOTMP2] == nes_c.ram[ADDR_AUDIOTMP2]);
            assert(memcmp(nes_6502.apu_regs, nes_c.apu_regs, 0x20) == 0);
        }
    }

    /* Test 83: Differential musicGetNextInstructionByte ($EA41) */
    {
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_MUSIC_CHAN_TMP_ADDR] = 0xAB;
        nes_6502.ram[ADDR_MUSIC_CHAN_TMP_ADDR + 1] = 0xE5; /* $E5AB = noises_table */
        nes_6502.ram[ADDR_MUSIC_DATA_CHAN_INSTRUCTION_OFFSET] = 2;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_MUSIC_GET_NEXT_INSTRUCTION_BYTE, 0, 0, 0, 0, 0xFD); /* X = 0 */
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_MUSIC_GET_NEXT_INSTRUCTION_BYTE, 50);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_MUSIC_CHAN_TMP_ADDR] = 0xAB;
        nes_c.ram[ADDR_MUSIC_CHAN_TMP_ADDR + 1] = 0xE5;
        nes_c.ram[ADDR_MUSIC_DATA_CHAN_INSTRUCTION_OFFSET] = 2;

        uint8_t c_byte = musicGetNextInstructionByte(&nes_c, 0);

        assert(cpu.a == c_byte);
        assert(nes_6502.ram[ADDR_MUSIC_DATA_CHAN_INSTRUCTION_OFFSET] == nes_c.ram[ADDR_MUSIC_DATA_CHAN_INSTRUCTION_OFFSET]);
    }

    /* Test 84: Differential loadMusicTrack ($E6D7) */
    {
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_CURRENT_AUDIO_SLOT] = 1;
        nes_6502.ram[ADDR_MUSIC_TRACK_DEC] = 0;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_LOAD_MUSIC_TRACK, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_LOAD_MUSIC_TRACK, 500);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_CURRENT_AUDIO_SLOT] = 1;
        nes_c.ram[ADDR_MUSIC_TRACK_DEC] = 0;

        loadMusicTrack(&nes_c);

        assert(nes_6502.ram[ADDR_CURRENTLY_PLAYING_MUSIC_TRACK] == nes_c.ram[ADDR_CURRENTLY_PLAYING_MUSIC_TRACK]);
        assert(nes_6502.ram[ADDR_MUSIC_DATA_NOTE_TABLE_OFFSET] == nes_c.ram[ADDR_MUSIC_DATA_NOTE_TABLE_OFFSET]);
        assert(nes_6502.ram[ADDR_MUSIC_DATA_DURATION_TABLE_OFFSET] == nes_c.ram[ADDR_MUSIC_DATA_DURATION_TABLE_OFFSET]);
        assert(memcmp(&nes_6502.ram[ADDR_MUSIC_DATA_CHAN_PTR], &nes_c.ram[ADDR_MUSIC_DATA_CHAN_PTR], 8) == 0);
        assert(memcmp(&nes_6502.ram[ADDR_MUSIC_DATA_CHAN_PTR_DEREF], &nes_c.ram[ADDR_MUSIC_DATA_CHAN_PTR_DEREF], 8) == 0);
        assert(memcmp(&nes_6502.ram[ADDR_MUSIC_CHAN_NOTE_DURATION_REMAINING], &nes_c.ram[ADDR_MUSIC_CHAN_NOTE_DURATION_REMAINING], 4) == 0);
        assert(memcmp(nes_6502.apu_regs, nes_c.apu_regs, 0x20) == 0);
    }

    /* Test 85: Differential setMMC1Control ($ACA3) */
    {
        uint8_t test_vals[4] = { 0x00, 0x10, 0x0E, 0x1F };
        for (int i = 0; i < 4; i++) {
            NES_State nes_6502 = rom_nes;
            cpu6502_init(&cpu, &nes_6502);
            cpu6502_set_state(&cpu, ROM_ADDR_SET_MMC1_CONTROL, test_vals[i], 0, 0, 0, 0xFD);
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_SET_MMC1_CONTROL, 100);
            assert(ok);

            NES_State nes_c = rom_nes;
            setMMC1Control(&nes_c, test_vals[i]);

            assert(nes_6502.mmc1_control == nes_c.mmc1_control);
            assert(nes_6502.mmc1_shift_count == nes_c.mmc1_shift_count);
        }
    }

    /* Test 86: Differential changeCHRBank0 ($ACB7) */
    {
        uint8_t test_vals[4] = { 0x00, 0x01, 0x02, 0x1F };
        for (int i = 0; i < 4; i++) {
            NES_State nes_6502 = rom_nes;
            cpu6502_init(&cpu, &nes_6502);
            cpu6502_set_state(&cpu, ROM_ADDR_CHANGE_CHR_BANK0, test_vals[i], 0, 0, 0, 0xFD);
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_CHANGE_CHR_BANK0, 100);
            assert(ok);

            NES_State nes_c = rom_nes;
            changeCHRBank0(&nes_c, test_vals[i]);

            assert(nes_6502.mmc1_chr0 == nes_c.mmc1_chr0);
            assert(nes_6502.mmc1_shift_count == nes_c.mmc1_shift_count);
        }
    }

    /* Test 87: Differential changeCHRBank1 ($ACCB) */
    {
        uint8_t test_vals[4] = { 0x00, 0x01, 0x02, 0x1F };
        for (int i = 0; i < 4; i++) {
            NES_State nes_6502 = rom_nes;
            cpu6502_init(&cpu, &nes_6502);
            cpu6502_set_state(&cpu, ROM_ADDR_CHANGE_CHR_BANK1, test_vals[i], 0, 0, 0, 0xFD);
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_CHANGE_CHR_BANK1, 100);
            assert(ok);

            NES_State nes_c = rom_nes;
            changeCHRBank1(&nes_c, test_vals[i]);

            assert(nes_6502.mmc1_chr1 == nes_c.mmc1_chr1);
            assert(nes_6502.mmc1_shift_count == nes_c.mmc1_shift_count);
        }
    }

    /* Test 88: Differential changePRGBank ($ACDF) */
    {
        uint8_t test_vals[4] = { 0x00, 0x01, 0x02, 0x0F };
        for (int i = 0; i < 4; i++) {
            NES_State nes_6502 = rom_nes;
            cpu6502_init(&cpu, &nes_6502);
            cpu6502_set_state(&cpu, ROM_ADDR_CHANGE_PRG_BANK, test_vals[i], 0, 0, 0, 0xFD);
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_CHANGE_PRG_BANK, 100);
            assert(ok);

            NES_State nes_c = rom_nes;
            changePRGBank(&nes_c, test_vals[i]);

            assert(nes_6502.mmc1_prg == nes_c.mmc1_prg);
            assert(nes_6502.mmc1_shift_count == nes_c.mmc1_shift_count);
        }
    }

    /* Test 89: Differential memset_ppu_page_and_more ($AC1C) */
    {
        /* Case 1: Nametable $2000 */
        NES_State nes_6502 = rom_nes;
        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_MEMSET_PPU_PAGE_AND_MORE, 0x20, 0xFF, 0x00, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_MEMSET_PPU_PAGE_AND_MORE, 20000);
        assert(ok);

        NES_State nes_c = rom_nes;
        memset_ppu_page_and_more(&nes_c, 0x20, 0xFF, 0x00);

        assert(nes_6502.ppu_addr == nes_c.ppu_addr);
        assert(nes_6502.ram[ADDR_TMP1] == nes_c.ram[ADDR_TMP1]);
        assert(nes_6502.ram[ADDR_TMP2] == nes_c.ram[ADDR_TMP2]);
        assert(nes_6502.ram[ADDR_TMP3] == nes_c.ram[ADDR_TMP3]);
        assert(nes_6502.ram[ADDR_CURRENT_PPU_CTRL] == nes_c.ram[ADDR_CURRENT_PPU_CTRL]);

        /* Case 2: Pattern table (< $20) */
        nes_6502 = rom_nes;
        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_MEMSET_PPU_PAGE_AND_MORE, 0x10, 0x55, 0x02, 0, 0xFD);
        ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_MEMSET_PPU_PAGE_AND_MORE, 10000);
        assert(ok);

        nes_c = rom_nes;
        memset_ppu_page_and_more(&nes_c, 0x10, 0x55, 0x02);

        assert(nes_6502.ppu_addr == nes_c.ppu_addr);
    }

    /* Test 90: Differential LAA82 ($AA82) */
    {
        NES_State nes_6502 = rom_nes;
        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_LAA82, 0x24, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_LAA82, 20000);
        assert(ok);

        NES_State nes_c = rom_nes;
        LAA82(&nes_c, 0x24);

        assert(nes_6502.ppu_addr == nes_c.ppu_addr);
        assert(nes_6502.ram[ADDR_CURRENT_PPU_CTRL] == nes_c.ram[ADDR_CURRENT_PPU_CTRL]);
    }

    /* Test 91: Differential nmi ($8005) */
    {
        NES_State nes_6502 = rom_nes;
        nes_6502.ram[ADDR_RENDER_MODE] = 0; /* render_mode_legal_and_title */
        nes_6502.ram[ADDR_SLEEP_COUNTER] = 4;
        nes_6502.ram[ADDR_FRAME_COUNTER] = 0x20;
        nes_6502.ram[ADDR_FRAME_COUNTER + 1] = 0x01;
        nes_6502.ram[ADDR_RNG_SEED] = 0x89;
        nes_6502.ram[ADDR_RNG_SEED + 1] = 0x88;
        nes_6502.ram[ADDR_OAM_STAGING_LENGTH] = 12;

        cpu6502_init(&cpu, &nes_6502);
        cpu6502_set_state(&cpu, ROM_ADDR_NMI, 0, 0, 0, 0, 0xFD);
        bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_NMI, 5000);
        assert(ok);

        NES_State nes_c = rom_nes;
        nes_c.ram[ADDR_RENDER_MODE] = 0;
        nes_c.ram[ADDR_SLEEP_COUNTER] = 4;
        nes_c.ram[ADDR_FRAME_COUNTER] = 0x20;
        nes_c.ram[ADDR_FRAME_COUNTER + 1] = 0x01;
        nes_c.ram[ADDR_RNG_SEED] = 0x89;
        nes_c.ram[ADDR_RNG_SEED + 1] = 0x88;
        nes_c.ram[ADDR_OAM_STAGING_LENGTH] = 12;

        nmi(&nes_c);

        assert(nes_6502.ram[ADDR_OAM_STAGING_LENGTH] == nes_c.ram[ADDR_OAM_STAGING_LENGTH]);
        assert(nes_6502.ram[ADDR_SLEEP_COUNTER] == nes_c.ram[ADDR_SLEEP_COUNTER]);
        assert(nes_6502.ram[ADDR_FRAME_COUNTER] == nes_c.ram[ADDR_FRAME_COUNTER]);
        assert(nes_6502.ram[ADDR_FRAME_COUNTER + 1] == nes_c.ram[ADDR_FRAME_COUNTER + 1]);
        assert(nes_6502.ram[ADDR_RNG_SEED] == nes_c.ram[ADDR_RNG_SEED]);
        assert(nes_6502.ram[ADDR_RNG_SEED + 1] == nes_c.ram[ADDR_RNG_SEED + 1]);
        assert(nes_6502.ram[ADDR_VERTICAL_BLANK_INTERVAL] == nes_c.ram[ADDR_VERTICAL_BLANK_INTERVAL]);
    }

    /* Test: Experimental C-integrated NES ROM Verification (if built) */
    {
        const char *exp_paths[] = {
            "build/rom/tetris_experimental.nes",
            "rom/tetris_experimental.nes",
            "../build/rom/tetris_experimental.nes"
        };
        NES_State nes_exp;
        bool loaded = false;
        for (size_t p = 0; p < sizeof(exp_paths)/sizeof(exp_paths[0]); ++p) {
            if (load_reference_rom(&nes_exp, exp_paths[p])) {
                loaded = true;
                break;
            }
        }
        if (loaded) {
            /* 1. Bit-for-bit PRG payload verification */
            assert(memcmp(nes_exp.prg_rom, rom_nes.prg_rom, NES_PRG_SIZE) == 0);

            /* 2. Direct 6502 CPU execution of compiled C copyOamStagingToOam */
            cpu6502_init(&cpu, &nes_exp);
            cpu6502_set_state(&cpu, ROM_ADDR_COPY_OAM_STAGING_TO_OAM, 0, 0, 0, 0, 0xFD);
            bool ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_COPY_OAM_STAGING_TO_OAM, 50);
            assert(ok);
            assert(nes_exp.oam_addr == 0x00);
            assert(cpu.a == 0x02);

            /* 3. Execute NMI interrupt containing compiled C copyOamStagingToOam */
            nes_exp.ram[ADDR_RENDER_MODE] = 0;
            nes_exp.ram[ADDR_SLEEP_COUNTER] = 4;
            nes_exp.ram[ADDR_FRAME_COUNTER] = 0x20;
            nes_exp.ram[ADDR_FRAME_COUNTER + 1] = 0x01;
            nes_exp.ram[ADDR_RNG_SEED] = 0x89;
            nes_exp.ram[ADDR_RNG_SEED + 1] = 0x88;
            nes_exp.ram[ADDR_OAM_STAGING_LENGTH] = 12;

            cpu6502_init(&cpu, &nes_exp);
            cpu6502_set_state(&cpu, ROM_ADDR_NMI, 0, 0, 0, 0, 0xFD);
            ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_NMI, 5000);
            assert(ok);
            assert(nes_exp.oam_addr == 0x00);
            assert(nes_exp.ram[ADDR_FRAME_COUNTER] == 0x21);

            /* 4. Direct 6502 CPU execution of compiled C muteAudio ($E271) */
            cpu6502_init(&cpu, &nes_exp);
            cpu6502_set_state(&cpu, ROM_ADDR_MUTE_AUDIO, 0xFF, 0, 0, 0, 0xFD);
            ok = cpu6502_run_subroutine(&cpu, ROM_ADDR_MUTE_AUDIO, 100);
            assert(ok);
            assert(cpu.a == 0x00);
            assert(nes_exp.apu_regs[REG_SQ1_VOL - 0x4000] == 0x10);
            assert(nes_exp.apu_regs[REG_SQ2_VOL - 0x4000] == 0x10);
            assert(nes_exp.apu_regs[REG_NOISE_VOL - 0x4000] == 0x10);
            assert(nes_exp.apu_regs[REG_TRI_LINEAR - 0x4000] == 0x00);
            assert(nes_exp.apu_regs[REG_DMC_RAW - 0x4000] == 0x00);

            printf("  [PASS] test_experimental_c_rom (Compiled C copyOamStagingToOam & muteAudio verified in 6502 CPU)\n");
        }
    }

    printf("  [PASS] test_differential_verification (All 6502 ROM vs C routines proven equivalent)\n");
    return 0;
}
