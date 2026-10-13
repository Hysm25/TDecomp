#include "game/state.h"
#include "game/tetrimino.h"
#include "game/playfield.h"
#include "game/input.h"
#include "game/highscore.h"
#include "math/bcd.h"
#include "math/rng.h"
#include <string.h>

/* Type B Success Graphic placed into playfield upon achieving line goal ($A42E) */
const uint8_t typebSuccessGraphic[31] = {
    0x38, 0x39, 0x39, 0x39, 0x39, 0x39, 0x39, 0x39,
    0x39, 0x3A, 0x3B, 0x1C, 0x1E, 0x0C, 0x0C, 0x0E,
    0x1C, 0x1C, 0x28, 0x3C, 0x3D, 0x3E, 0x3E, 0x3E,
    0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3F, 0x80
};

/*
 * Copies Player 1 zero-page state ($0060 - $007F) into the active player
 * work area ($0040 - $005F), sets playfieldAddr to $0400, and mirrors inputs.
 * Source address: $8776 (main.asm lines 1269-1285).
 */
void makePlayer1Active(NES_State *nes) {
    if (!nes) return;

    nes->ram[ADDR_ACTIVE_PLAYER] = 1;
    nes->ram[ADDR_PLAYFIELD_ADDR + 1] = HIGH_BYTE(ADDR_PLAYFIELD);
    nes->ram[ADDR_NEWLY_PRESSED_BUTTONS] = nes->ram[ADDR_NEWLY_PRESSED_P1];
    nes->ram[ADDR_HELD_BUTTONS] = nes->ram[ADDR_HELD_BUTTONS_P1];

    for (int i = 0; i < 32; i++) {
        nes->ram[ADDR_TETRIMINO_X + i] = nes->ram[ADDR_P1_BASE + i];
    }
}

/*
 * Copies Player 2 zero-page state ($0080 - $009F) into the active player
 * work area ($0040 - $005F), sets playfieldAddr to $0500, and mirrors inputs.
 * Source address: $8792 (main.asm lines 1288-1304).
 */
void makePlayer2Active(NES_State *nes) {
    if (!nes) return;

    nes->ram[ADDR_ACTIVE_PLAYER] = 2;
    nes->ram[ADDR_PLAYFIELD_ADDR + 1] = HIGH_BYTE(ADDR_PLAYFIELD_P2);
    nes->ram[ADDR_NEWLY_PRESSED_BUTTONS] = nes->ram[ADDR_NEWLY_PRESSED_P2];
    nes->ram[ADDR_HELD_BUTTONS] = nes->ram[ADDR_HELD_BUTTONS_P2];

    for (int i = 0; i < 32; i++) {
        nes->ram[ADDR_TETRIMINO_X + i] = nes->ram[ADDR_P2_BASE + i];
    }
}

/*
 * Saves active player state ($0040 - $005F) back to Player 1 mirror ($0060 - $007F).
 * In 2-player mode, swaps pendingGarbage and pendingGarbageInactivePlayer.
 * Source address: $87AE (main.asm lines 1307-1322).
 */
void savePlayer1State(NES_State *nes) {
    if (!nes) return;

    for (int i = 0; i < 32; i++) {
        nes->ram[ADDR_P1_BASE + i] = nes->ram[ADDR_TETRIMINO_X + i];
    }

    if (nes->ram[ADDR_NUMBER_OF_PLAYERS] != 1) {
        uint8_t pending = nes->ram[ADDR_PENDING_GARBAGE];
        nes->ram[ADDR_PENDING_GARBAGE] = nes->ram[ADDR_PENDING_GARBAGE_INACTIVE];
        nes->ram[ADDR_PENDING_GARBAGE_INACTIVE] = pending;
    }
}

/*
 * Saves active player state ($0040 - $005F) back to Player 2 mirror ($0080 - $009F).
 * Swaps pendingGarbage and pendingGarbageInactivePlayer.
 * Source address: $87C8 (main.asm lines 1325-1337).
 */
void savePlayer2State(NES_State *nes) {
    if (!nes) return;

    for (int i = 0; i < 32; i++) {
        nes->ram[ADDR_P2_BASE + i] = nes->ram[ADDR_TETRIMINO_X + i];
    }

    uint8_t pending = nes->ram[ADDR_PENDING_GARBAGE];
    nes->ram[ADDR_PENDING_GARBAGE] = nes->ram[ADDR_PENDING_GARBAGE_INACTIVE];
    nes->ram[ADDR_PENDING_GARBAGE_INACTIVE] = pending;
}

/*
 * Sets currentPiece to TETRIMINO_HIDDEN (0x13).
 * Source address: $9E2F (main.asm lines 3777-3780).
 */
void playState_unassignOrientationId(NES_State *nes) {
    if (!nes) return;
    nes->ram[ADDR_CURRENT_PIECE] = TETRIMINO_HIDDEN;
}

/*
 * Increments playState by 1.
 * Source address: $9E37 (main.asm lines 3785-3786).
 */
void playState_incrementPlayState(NES_State *nes) {
    if (!nes) return;
    nes->ram[ADDR_PLAY_STATE]++;
}

/*
 * No-operation state handler.
 * Source address: $9E39 (main.asm lines 3787-3788).
 */
void playState_noop(NES_State *nes) {
    (void)nes;
}

/*
 * Handles player 1 active piece control (shift, rotate, drop).
 * Source address: $81CF (main.asm lines 489-493).
 */
void playState_playerControlsActiveTetrimino(NES_State *nes) {
    if (!nes) return;
    shift_tetrimino(nes);
    rotate_tetrimino(nes);
    drop_tetrimino(nes);
}

/*
 * Handles player 2 active piece control (shift, rotate, drop).
 * Source address: $81F6 (main.asm lines 510-514).
 */
void playState_player2ControlsActiveTetrimino(NES_State *nes) {
    if (!nes) return;
    shift_tetrimino(nes);
    rotate_tetrimino(nes);
    drop_tetrimino(nes);
}

/*
 * Checks Type B line goal; increments playState or triggers goal completion.
 * Source address: $A3F2 (main.asm lines 4545-4587).
 */
void playState_bTypeGoalCheck(NES_State *nes) {
    if (!nes) return;

    if (nes->ram[ADDR_GAME_TYPE] == GAMETYPE_A) {
        nes->ram[ADDR_PLAY_STATE]++;
        return;
    }

    /* Type B: check remaining lines */
    if (nes->ram[ADDR_LINES] != 0) {
        nes->ram[ADDR_PLAY_STATE]++;
        return;
    }

    /* Goal completed in Type B */
    setMusicTrack(nes, 0x02);

    /* Copy success graphic to playfield starting at offset 0x46 (row 7, col 0) */
    uint16_t pf_addr = nes_read16(nes, ADDR_PLAYFIELD_ADDR);
    for (size_t x = 0; typebSuccessGraphic[x] != 0x80; x++) {
        nes_write8(nes, (uint16_t)(pf_addr + 0x46 + x), typebSuccessGraphic[x]);
    }

    nes->ram[ADDR_P1_VRAM_ROW] = 0;
    nes->ram[ADDR_RENDER_MODE] = 0;
    nes->ram[ADDR_PLAY_STATE] = 0;
    nes->ram[ADDR_GAME_MODE_STATE]++;
}

/*
 * Unused / disabled VRAM row increment routine (single RTS).
 * Source address: $9874 (main.asm lines 2878-2895).
 */
void noop_disabledVramRowIncr(NES_State *nes) {
    (void)nes;
}

/*
 * Spawns the next tetromino, resets fall/autorepeat counters, and selects next piece.
 * Handles 2-player piece spawn delay (28 frames) and updates piece statistics.
 * Source address: $988E (main.asm lines 2896-2947).
 */
void playState_spawnNextTetrimino(NES_State *nes) {
    if (!nes) return;

    /*
     * VRAM row guard:
     * lda vramRow
     * cmp #$20
     * bmi @ret
     * If vramRow < 0x20 (signed branch bmi), return early to let VRAM copy complete.
     */
    uint8_t vram_row = nes->ram[ADDR_VRAM_ROW];
    if ((uint8_t)(vram_row - VRAM_ROW_COMPLETE) & 0x80) {
        return;
    }

    /*
     * Check number of players:
     * lda numberOfPlayers
     * cmp #$01
     * beq @spawnPiece
     */
    uint8_t num_players = nes->ram[ADDR_NUMBER_OF_PLAYERS];
    if (num_players != 1) {
        uint8_t delay_counter = nes->ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER];
        if (delay_counter == 0) {
            /* Start 2-player piece delay */
            nes->ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER]++;
            nes->ram[ADDR_TWO_PLAYER_PIECE_DELAY_PLAYER] = nes->ram[ADDR_ACTIVE_PLAYER];
            nes->ram[ADDR_TWO_PLAYER_PIECE_DELAY_PIECE] = chooseNextTetrimino(nes);
            return;
        }

        /*
         * Delay in progress:
         * lda twoPlayerPieceDelayPlayer
         * cmp activePlayer
         * bne @ret
         * lda twoPlayerPieceDelayCounter
         * cmp #$1C
         * bne @ret
         */
        if (nes->ram[ADDR_TWO_PLAYER_PIECE_DELAY_PLAYER] != nes->ram[ADDR_ACTIVE_PLAYER]) {
            return;
        }
        if (delay_counter != TWO_PLAYER_PIECE_DELAY_FRAMES) {
            return;
        }
    }

    /*
     * @spawnPiece:
     * lda #$00
     * sta twoPlayerPieceDelayCounter
     * sta fallTimer
     * sta tetriminoY
     * lda #$01
     * sta playState
     * lda #$05
     * sta tetriminoX
     * ldx nextPiece
     * lda spawnOrientationFromOrientation,x
     * sta currentPiece
     * jsr incrementPieceStat
     */
    nes->ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER] = 0;
    nes->ram[ADDR_FALL_TIMER] = 0;
    nes->ram[ADDR_TETRIMINO_Y] = TETRIMINO_SPAWN_Y;
    nes->ram[ADDR_PLAY_STATE] = PLAYSTATE_CONTROL_ACTIVE;
    nes->ram[ADDR_TETRIMINO_X] = TETRIMINO_SPAWN_X;

    uint8_t next_piece = nes->ram[ADDR_NEXT_PIECE];
    uint8_t current_piece = (next_piece < ORIENTATION_COUNT)
        ? spawnOrientationFromOrientation[next_piece]
        : ORIENT_T_DOWN;
    nes->ram[ADDR_CURRENT_PIECE] = current_piece;

    incrementPieceStat(nes, current_piece);

    /*
     * lda numberOfPlayers
     * cmp #$01
     * beq @onePlayerPieceSelection
     * lda twoPlayerPieceDelayPiece
     * sta nextPiece
     * jmp @resetDownHold
     * @onePlayerPieceSelection:
     * jsr chooseNextTetrimino
     * sta nextPiece
     * @resetDownHold:
     * lda #$00
     * sta autorepeatY
     * @ret: rts
     */
    if (num_players == 1) {
        nes->ram[ADDR_NEXT_PIECE] = chooseNextTetrimino(nes);
    } else {
        nes->ram[ADDR_NEXT_PIECE] = nes->ram[ADDR_TWO_PLAYER_PIECE_DELAY_PIECE];
    }

    nes->ram[ADDR_AUTOREPEAT_Y] = 0;
}

/*
 * Transitions gameModeState to 2 and calls noop_disabledVramRowIncr.
 * Source address: $9E27 (main.asm lines 3771-3776).
 */
void gameModeState_vblankThenRunState2(NES_State *nes) {
    if (!nes) return;
    nes->ram[ADDR_GAME_MODE_STATE] = 0x02;
    noop_disabledVramRowIncr(nes);
}

/*
 * Master gameplay framing counter update: resets OAM staging length,
 * advances player fall timers, increments 2P piece delay counter,
 * handles SELECT toggle for displayNextPiece, and increments gameModeState.
 * Source address: $8884 (main.asm lines 1441-1464).
 */
void gameModeState_updateCountersAndNonPlayerState(NES_State *nes) {
    if (!nes) return;

    /* Original assembly switches CHR banks; in portable C, reset OAM staging length */
    nes->ram[ADDR_OAM_STAGING_LENGTH] = 0;

    nes->ram[ADDR_P1_FALL_TIMER]++;
    nes->ram[ADDR_P2_FALL_TIMER]++;

    if (nes->ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER] != 0) {
        nes->ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER]++;
    }

    if (nes->ram[ADDR_NEWLY_PRESSED_P1] & BUTTON_SELECT) {
        nes->ram[ADDR_DISPLAY_NEXT_PIECE] ^= 0x01;
    }

    nes->ram[ADDR_GAME_MODE_STATE]++;
}

/*
 * Dispatches the current playState (0 - 11) for Player 1.
 * Source address: $81B2 (main.asm lines 474-488).
 */
void branchOnPlayStatePlayer1(NES_State *nes) {
    if (!nes) return;
    uint8_t state = nes->ram[ADDR_PLAY_STATE];

    switch (state) {
        case PLAYSTATE_UNASSIGN_ORIENTATION:
            playState_unassignOrientationId(nes);
            break;
        case PLAYSTATE_CONTROL_ACTIVE:
            playState_playerControlsActiveTetrimino(nes);
            break;
        case PLAYSTATE_LOCK_TETRIMINO:
            playState_lockTetrimino(nes);
            break;
        case PLAYSTATE_CHECK_ROWS:
            playState_checkForCompletedRows(nes);
            break;
        case PLAYSTATE_LINE_CLEAR_ANIM:
            playState_noop(nes);
            break;
        case PLAYSTATE_UPDATE_LINES_AND_STATISTICS:
            playState_updateLinesAndStatistics(nes);
            break;
        case PLAYSTATE_B_TYPE_GOAL_CHECK:
            playState_bTypeGoalCheck(nes);
            break;
        case PLAYSTATE_RECEIVE_GARBAGE:
            playState_receiveGarbage(nes);
            break;
        case PLAYSTATE_SPAWN_NEXT:
            playState_spawnNextTetrimino(nes);
            break;
        case PLAYSTATE_NOOP_9:
            playState_noop(nes);
            break;
        case PLAYSTATE_CURTAIN_FALL:
            playState_updateGameOverCurtain(nes);
            break;
        case PLAYSTATE_INCREMENT_PLAY_STATE:
            playState_incrementPlayState(nes);
            break;
        default:
            /* Out-of-range safety */
            break;
    }
}

/*
 * Dispatches the current playState (0 - 11) for Player 2.
 * Source address: $81D9 (main.asm lines 495-509).
 */
void branchOnPlayStatePlayer2(NES_State *nes) {
    if (!nes) return;
    uint8_t state = nes->ram[ADDR_PLAY_STATE];

    switch (state) {
        case PLAYSTATE_UNASSIGN_ORIENTATION:
            playState_unassignOrientationId(nes);
            break;
        case PLAYSTATE_CONTROL_ACTIVE:
            playState_player2ControlsActiveTetrimino(nes);
            break;
        case PLAYSTATE_LOCK_TETRIMINO:
            playState_lockTetrimino(nes);
            break;
        case PLAYSTATE_CHECK_ROWS:
            playState_checkForCompletedRows(nes);
            break;
        case PLAYSTATE_LINE_CLEAR_ANIM:
            playState_noop(nes);
            break;
        case PLAYSTATE_UPDATE_LINES_AND_STATISTICS:
            playState_updateLinesAndStatistics(nes);
            break;
        case PLAYSTATE_B_TYPE_GOAL_CHECK:
            playState_bTypeGoalCheck(nes);
            break;
        case PLAYSTATE_RECEIVE_GARBAGE:
            playState_receiveGarbage(nes);
            break;
        case PLAYSTATE_SPAWN_NEXT:
            playState_spawnNextTetrimino(nes);
            break;
        case PLAYSTATE_NOOP_9:
            playState_noop(nes);
            break;
        case PLAYSTATE_CURTAIN_FALL:
            playState_updateGameOverCurtain(nes);
            break;
        case PLAYSTATE_INCREMENT_PLAY_STATE:
            playState_incrementPlayState(nes);
            break;
        default:
            /* Out-of-range safety */
            break;
    }
}

/*
 * Unified dispatcher branching on activePlayer (Player 1 or Player 2).
 */
void branchOnPlayState(NES_State *nes) {
    if (!nes) return;
    if (nes->ram[ADDR_ACTIVE_PLAYER] == 2) {
        branchOnPlayStatePlayer2(nes);
    } else {
        branchOnPlayStatePlayer1(nes);
    }
}

/*
 * Authoritative 44-byte nametable patch for Type B playing background.
 * Source address: $86B0 (main.asm lines 1183-1189).
 */
const uint8_t game_typeb_nametable_patch[44] = {
    0x22, 0xF7, 0x38, 0x39, 0x39, 0x39, 0x39, 0x39,
    0x39, 0x3A, 0xFE, 0x23, 0x17, 0x3B, 0x11, 0x0E,
    0x12, 0x10, 0x11, 0x1D, 0x3C, 0xFE, 0x23, 0x37,
    0x3B, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3C,
    0xFE, 0x23, 0x57, 0x3D, 0x3E, 0x3E, 0x3E, 0x3E,
    0x3E, 0x3E, 0x3F, 0xFD
};

/*
 * Memory clearing routine filling 256-byte pages from end_page down to start_page
 * with value val. Sets tmp1 = 0, tmp2 = start_page - 1.
 * Source address: $AC6A (main.asm lines 5635-5653).
 */
void memset_page(NES_State *nes, uint8_t val, uint8_t start_page, uint8_t end_page) {
    if (!nes) return;

    for (int page = (int)end_page; page >= (int)start_page; page--) {
        uint16_t addr = (uint16_t)(page << 8);
        for (int i = 0; i < 256; i++) {
            nes_write8(nes, (uint16_t)(addr + i), val);
        }
    }
    nes->ram[ADDR_TMP1] = 0x00;
    nes->ram[ADDR_TMP2] = (uint8_t)(start_page - 1);
}

/*
 * Evaluates game over state: in 1P mode triggers high-score check and name entry,
 * clears playfields to tileEmpty, and transitions gameMode to MODE_LEVEL_MENU (3).
 * In 2P mode transitions gameModeState to 9. If not game over, increments gameModeState.
 * Source address: $9CBF (main.asm lines 3545-3595).
 */
void gameModeState_handleGameOver(NES_State *nes) {
    if (!nes) return;

    nes->ram[ADDR_GENERAL_COUNTER2] = 0x05;
    if (nes->ram[ADDR_P1_PLAY_STATE] == 0) {
        goto gameOver;
    }
    if (nes->ram[ADDR_NUMBER_OF_PLAYERS] == 1) {
        goto ret;
    }

    nes->ram[ADDR_GENERAL_COUNTER2] = 0x04;
    if (nes->ram[ADDR_P2_PLAY_STATE] != 0) {
        goto ret;
    }

gameOver:
    if (nes->ram[ADDR_NUMBER_OF_PLAYERS] == 1) {
        nes->ram[ADDR_RENDER_MODE] = 3;
        if (nes->ram[ADDR_NUMBER_OF_PLAYERS] == 1) {
            handleHighScoreIfNecessary(nes);
        }
        nes->ram[ADDR_P1_PLAY_STATE] = 1;
        nes->ram[ADDR_P2_PLAY_STATE] = 1;
        memset_page(nes, TILE_EMPTY, 0x04, 0x05);
        nes->ram[ADDR_P1_VRAM_ROW] = 0;
        nes->ram[ADDR_P2_VRAM_ROW] = 0;
        nes->ram[ADDR_P1_PLAY_STATE] = 1;
        nes->ram[ADDR_P2_PLAY_STATE] = 1;
        nes->ram[ADDR_OAM_STAGING_LENGTH] = 0;
        nes->ram[ADDR_GAME_MODE] = MODE_LEVEL_MENU;
        return;
    } else {
        nes->ram[ADDR_GAME_MODE_STATE] = 9;
        return;
    }

ret:
    nes->ram[ADDR_GAME_MODE_STATE]++;
}

/*
 * Checks for A + B + SELECT + START soft reset combo on Player 1.
 * If active, resets gameMode to MODE_LEGAL_SCREEN (0). Otherwise increments gameModeState.
 * Source address: $9E16 (main.asm lines 3756-3770).
 */
void gameModeState_checkForResetKeyCombo(NES_State *nes) {
    if (!nes) return;

    uint8_t reset_combo = (BUTTON_A | BUTTON_B | BUTTON_SELECT | BUTTON_START);
    if (nes->ram[ADDR_HELD_BUTTONS_P1] == reset_combo) {
        nes->ram[ADDR_GAME_MODE] = MODE_LEGAL_SCREEN;
        return;
    }
    nes->ram[ADDR_GAME_MODE_STATE]++;
}

/*
 * Handles START button during gameplay and demo mode:
 * In Mode 5 (Demo): exits demo to Title Screen (gameMode = 1).
 * In Game (renderMode = 3): handles pause toggle, PAUSE sprite staging, and resume.
 * Source address: $A37F (main.asm lines 4475-4544).
 */
void gameModeState_startButtonHandling(NES_State *nes) {
    if (!nes) return;

    /* Mode 5 (Demo mode): START exits demo to Title Screen */
    if (nes->ram[ADDR_GAME_MODE] == MODE_DEMO) {
        if (nes->ram[ADDR_NEWLY_PRESSED_P1] == BUTTON_START) {
            nes->ram[ADDR_GAME_MODE] = MODE_TITLE_SCREEN;
            nes->ram[ADDR_GAME_MODE_STATE]++;
            return;
        }
    }

    /* In gameplay (renderMode == 3): START triggers pause toggle */
    if (nes->ram[ADDR_RENDER_MODE] == 3) {
        if (nes->ram[ADDR_NEWLY_PRESSED_P1] & BUTTON_START) {
            /* Do nothing if curtain is being lowered */
            if (nes->ram[ADDR_P1_PLAY_STATE] == PLAYSTATE_CURTAIN_FALL) {
                nes->ram[ADDR_GAME_MODE_STATE]++;
                return;
            }

            /* Pause sequence */
            nes->ram[ADDR_MUSIC_STAGING_NOISE_HI] = 0x05;
            nes->ram[ADDR_RENDER_MODE] = 0;
            nes_write8(nes, REG_PPUMASK, 0x16);
            memset_page(nes, 0xFF, 0x02, 0x02);

            /* Resume */
            nes_write8(nes, REG_PPUMASK, 0x1E);
            nes->ram[ADDR_MUSIC_STAGING_NOISE_HI] = 0;
            nes->ram[ADDR_P1_VRAM_ROW] = 0;
            nes->ram[ADDR_RENDER_MODE] = 3;
        }
    }

    nes->ram[ADDR_GAME_MODE_STATE]++;
}

/*
 * Step helper for gameModeState_startButtonHandling.
 */
void gameModeState_startButtonHandling_step(NES_State *nes) {
    gameModeState_startButtonHandling(nes);
}

/*
 * Completes background initialization, setting playStates to 1, copying startLevel
 * to levelNumber, and incrementing gameModeState.
 * Source address: $8693 (main.asm lines 1168-1182).
 */
void gameModeState_initGameBackground_finish(NES_State *nes) {
    if (!nes) return;

    nes->ram[ADDR_P1_PLAY_STATE] = 1;
    nes->ram[ADDR_P2_PLAY_STATE] = 1;
    nes->ram[ADDR_P1_LEVEL_NUMBER] = nes->ram[ADDR_P1_START_LEVEL];
    nes->ram[ADDR_P2_LEVEL_NUMBER] = nes->ram[ADDR_P2_START_LEVEL];
    nes->ram[ADDR_GAME_MODE_STATE]++;
}

/*
 * Initializes the game background, copying palettes, nametables, and writing
 * the top high score (from RAM $0730 Type A or $073C Type B) to PPU nametable.
 * Source address: $85F0 (main.asm lines 1092-1167).
 */
void gameModeState_initGameBackground(NES_State *nes) {
    if (!nes) return;

    nes_write8(nes, REG_PPUADDR, 0x20);
    nes_write8(nes, REG_PPUADDR, 0x83);

    if (nes->ram[ADDR_GAME_TYPE] == GAMETYPE_A) {
        nes_write8(nes, REG_PPUDATA, 0x0A);
        nes_write8(nes, REG_PPUADDR, 0x20);
        nes_write8(nes, REG_PPUADDR, 0xB8);
        twoDigsToPPU(nes, nes->ram[ADDR_HIGH_SCORE_A]);
        twoDigsToPPU(nes, nes->ram[ADDR_HIGH_SCORE_A + 1]);
        twoDigsToPPU(nes, nes->ram[ADDR_HIGH_SCORE_A + 2]);
        gameModeState_initGameBackground_finish(nes);
        return;
    } else {
        nes_write8(nes, REG_PPUDATA, 0x0B);
        nes_write8(nes, REG_PPUADDR, 0x20);
        nes_write8(nes, REG_PPUADDR, 0xB8);
        twoDigsToPPU(nes, nes->ram[ADDR_HIGH_SCORE_B]);
        twoDigsToPPU(nes, nes->ram[ADDR_HIGH_SCORE_B + 1]);
        twoDigsToPPU(nes, nes->ram[ADDR_HIGH_SCORE_B + 2]);

        /* Patch Type B nametable from game_typeb_nametable_patch */
        size_t idx = 0;
        while (idx < sizeof(game_typeb_nametable_patch)) {
            uint8_t hi = game_typeb_nametable_patch[idx++];
            uint8_t lo = game_typeb_nametable_patch[idx++];
            nes_write8(nes, REG_PPUADDR, hi);
            nes_write8(nes, REG_PPUADDR, lo);
            while (idx < sizeof(game_typeb_nametable_patch)) {
                uint8_t val = game_typeb_nametable_patch[idx++];
                if (val == 0xFE) {
                    break;
                }
                if (val == 0xFD) {
                    goto endPatch;
                }
                nes_write8(nes, REG_PPUDATA, val);
            }
        }

    endPatch:
        nes_write8(nes, REG_PPUADDR, 0x23);
        nes_write8(nes, REG_PPUADDR, 0x3B);
        nes_write8(nes, REG_PPUDATA, (uint8_t)(nes->ram[ADDR_START_HEIGHT] & 0x0F));
        gameModeState_initGameBackground_finish(nes);
    }
}

/*
 * Initializes game state for playing and demo modes: clears playfield, stats,
 * scores, lines, sets demo pointer, chooses starting tetriminos, and sets music.
 * Source address: $86DC (main.asm lines 1190-1268).
 */
void gameModeState_initGameState(NES_State *nes) {
    if (!nes) return;

    memset_page(nes, TILE_EMPTY, 0x04, 0x04);
    memset(&nes->ram[ADDR_STATS_BY_TYPE], 0, 15);

    nes->ram[ADDR_P1_TETRIMINO_X] = 0x05;
    nes->ram[ADDR_P2_TETRIMINO_X] = 0x05;
    nes->ram[ADDR_P1_TETRIMINO_Y] = 0x00;
    nes->ram[ADDR_P2_TETRIMINO_Y] = 0x00;
    nes->ram[ADDR_P1_VRAM_ROW] = 0x00;
    nes->ram[ADDR_P2_VRAM_ROW] = 0x00;
    nes->ram[ADDR_P1_FALL_TIMER] = 0x00;
    nes->ram[ADDR_P2_FALL_TIMER] = 0x00;
    nes->ram[ADDR_PENDING_GARBAGE] = 0x00;
    nes->ram[ADDR_PENDING_GARBAGE_INACTIVE_PLAYER] = 0x00;

    nes->ram[ADDR_P1_SCORE] = 0x00;
    nes->ram[ADDR_P1_SCORE + 1] = 0x00;
    nes->ram[ADDR_P1_SCORE + 2] = 0x00;
    nes->ram[ADDR_P2_SCORE] = 0x00;
    nes->ram[ADDR_P2_SCORE + 1] = 0x00;
    nes->ram[ADDR_P2_SCORE + 2] = 0x00;

    nes->ram[ADDR_P1_LINES] = 0x00;
    nes->ram[ADDR_P1_LINES + 1] = 0x00;
    nes->ram[ADDR_P2_LINES] = 0x00;
    nes->ram[ADDR_P2_LINES + 1] = 0x00;

    nes->ram[ADDR_TWO_PLAYER_PIECE_DELAY_COUNTER] = 0x00;
    memset(&nes->ram[ADDR_LINE_CLEAR_STATS_BY_TYPE], 0, 4);
    nes->ram[ADDR_ALLEGRO] = 0x00;

    nes->ram[ADDR_DEMO_HELD_BUTTONS] = 0x00;
    nes->ram[ADDR_DEMO_REPEATS] = 0x00;
    nes->ram[ADDR_DEMO_INDEX] = 0x00;
    nes->ram[ADDR_DEMO_BUTTONS_ADDR] = 0x00;
    nes->ram[ADDR_SPAWN_ID] = 0x00;
    nes->ram[ADDR_DEMO_BUTTONS_ADDR + 1] = 0xDD; /* >demoButtonsTable */

    nes->ram[ADDR_RENDER_MODE] = 0x03;
    nes->ram[ADDR_P1_AUTOREPEAT_Y] = INITIAL_AUTOREPEAT_Y;
    nes->ram[ADDR_P2_AUTOREPEAT_Y] = INITIAL_AUTOREPEAT_Y;

    uint8_t piece1 = chooseNextTetrimino(nes);
    nes->ram[ADDR_P1_CURRENT_PIECE] = piece1;
    nes->ram[ADDR_P2_CURRENT_PIECE] = piece1;
    incrementPieceStat(nes, piece1);

    generateNextPseudorandomNumber(nes, ADDR_RNG_SEED, 2);
    uint8_t piece2 = chooseNextTetrimino(nes);
    nes->ram[ADDR_NEXT_PIECE] = piece2;
    nes->ram[ADDR_TWO_PLAYER_PIECE_DELAY_PIECE] = piece2;

    if (nes->ram[ADDR_GAME_TYPE] != GAMETYPE_A) {
        nes->ram[ADDR_P1_LINES] = 0x25;
        nes->ram[ADDR_P2_LINES] = 0x25;
    }

    nes->ram[ADDR_OUT_OF_DATE_RENDER_FLAGS] = (RENDER_STATS | RENDER_SCORE | RENDER_LEVEL | RENDER_LINES);
    initPlayfieldIfTypeB(nes);

    uint8_t mtype = nes->ram[ADDR_MUSIC_TYPE] & 0x03;
    setMusicTrack(nes, musicSelectionTable[mtype]);

    nes->ram[ADDR_GAME_MODE_STATE]++;
}

/*
 * Master state machine dispatcher for Mode 4 (Playing) and Mode 5 (Ending / Demo),
 * jumping through switch_s_plus_2a over states 0..8 of gameModeState.
 * Source address: $819B (main.asm lines 462-473).
 */
void gameMode_playAndEndingHighScore(NES_State *nes) {
    if (!nes) return;

    uint8_t state = nes->ram[ADDR_GAME_MODE_STATE];
    switch (state) {
        case 0:
            gameModeState_initGameBackground(nes);
            break;
        case 1:
            gameModeState_initGameState(nes);
            break;
        case 2:
            gameModeState_updateCountersAndNonPlayerState(nes);
            break;
        case 3:
            gameModeState_handleGameOver(nes);
            break;
        case 4:
            /* State 4: gameModeState_updatePlayer1 (deferred to Batch 11 sprite staging) */
            nes->ram[ADDR_GAME_MODE_STATE]++;
            break;
        case 5:
            /* State 5: gameModeState_updatePlayer2 (deferred to Batch 11 sprite staging) */
            nes->ram[ADDR_GAME_MODE_STATE]++;
            break;
        case 6:
            gameModeState_checkForResetKeyCombo(nes);
            break;
        case 7:
            gameModeState_startButtonHandling(nes);
            break;
        case 8:
            gameModeState_vblankThenRunState2(nes);
            break;
        default:
            break;
    }
}

/*
 * JSR wrapper jump target for Mode 4 and Mode 5 from branchOnGameMode.
 * Source address: $815D (main.asm lines 428-431).
 */
void gameMode_playAndEndingHighScore_jmp(NES_State *nes) {
    gameMode_playAndEndingHighScore(nes);
}
