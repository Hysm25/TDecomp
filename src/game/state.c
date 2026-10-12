#include "game/state.h"
#include "game/tetrimino.h"
#include "game/playfield.h"
#include "game/input.h"
#include "math/bcd.h"
#include "math/rng.h"
#include <string.h>

/* Type B Success Graphic placed into playfield upon achieving line goal ($A42E) */
static const uint8_t typebSuccessGraphic[] = {
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
