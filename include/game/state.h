#ifndef TDECOMP_GAME_STATE_H
#define TDECOMP_GAME_STATE_H

#include "common.h"
#include "nes.h"
#include "constants/gameplay.h"
#include "constants/memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Copies Player 1 zero-page state ($0060 - $007F) into the active player
 * work area ($0040 - $005F), sets playfieldAddr to $0400, and mirrors inputs.
 * Source address: $8776 (main.asm lines 1269-1285).
 */
void makePlayer1Active(NES_State *nes);

/*
 * Copies Player 2 zero-page state ($0080 - $009F) into the active player
 * work area ($0040 - $005F), sets playfieldAddr to $0500, and mirrors inputs.
 * Source address: $8792 (main.asm lines 1288-1304).
 */
void makePlayer2Active(NES_State *nes);

/*
 * Saves active player state ($0040 - $005F) back to Player 1 mirror ($0060 - $007F).
 * In 2-player mode, swaps pendingGarbage and pendingGarbageInactivePlayer.
 * Source address: $87AE (main.asm lines 1307-1322).
 */
void savePlayer1State(NES_State *nes);

/*
 * Saves active player state ($0040 - $005F) back to Player 2 mirror ($0080 - $009F).
 * Swaps pendingGarbage and pendingGarbageInactivePlayer.
 * Source address: $87C8 (main.asm lines 1325-1337).
 */
void savePlayer2State(NES_State *nes);

/*
 * Sets currentPiece to TETRIMINO_HIDDEN (0x13).
 * Source address: $9E2F (main.asm lines 3777-3780).
 */
void playState_unassignOrientationId(NES_State *nes);

/*
 * Increments playState by 1.
 * Source address: $9E37 (main.asm lines 3785-3786).
 */
void playState_incrementPlayState(NES_State *nes);

/*
 * No-operation state handler.
 * Source address: $9E39 (main.asm lines 3787-3788).
 */
void playState_noop(NES_State *nes);

/*
 * Handles player 1 active piece control (shift, rotate, drop).
 * Source address: $81CF (main.asm lines 489-493).
 */
void playState_playerControlsActiveTetrimino(NES_State *nes);

/*
 * Handles player 2 active piece control (shift, rotate, drop).
 * Source address: $81F6 (main.asm lines 510-514).
 */
void playState_player2ControlsActiveTetrimino(NES_State *nes);

/*
 * Checks Type B line goal; increments playState or triggers goal completion.
 * Source address: $A3F2 (main.asm lines 4545-4587).
 */
void playState_bTypeGoalCheck(NES_State *nes);

/*
 * Spawns the next tetromino, resets fall/autorepeat counters, and selects next piece.
 * Handles 2-player piece spawn delay (28 frames) and updates piece statistics.
 * Source address: $988E (main.asm lines 2896-2947).
 */
void playState_spawnNextTetrimino(NES_State *nes);

/*
 * Unused / disabled VRAM row increment routine (single RTS).
 * Source address: $9874 (main.asm lines 2878-2895).
 */
void noop_disabledVramRowIncr(NES_State *nes);

/*
 * Transitions gameModeState to 2 and calls noop_disabledVramRowIncr.
 * Source address: $9E27 (main.asm lines 3771-3776).
 */
void gameModeState_vblankThenRunState2(NES_State *nes);

/*
 * Master gameplay framing counter update: resets OAM staging length,
 * advances player fall timers, increments 2P piece delay counter,
 * handles SELECT toggle for displayNextPiece, and increments gameModeState.
 * Source address: $8884 (main.asm lines 1441-1464).
 */
void gameModeState_updateCountersAndNonPlayerState(NES_State *nes);

/*
 * Dispatches the current playState (0 - 11) for Player 1.
 * Source address: $81B2 (main.asm lines 474-488).
 */
void branchOnPlayStatePlayer1(NES_State *nes);

/*
 * Dispatches the current playState (0 - 11) for Player 2.
 * Source address: $81D9 (main.asm lines 495-509).
 */
void branchOnPlayStatePlayer2(NES_State *nes);

/*
 * Unified dispatcher branching on activePlayer (Player 1 or Player 2).
 */
void branchOnPlayState(NES_State *nes);

#ifdef __cplusplus
}
#endif

#endif /* TDECOMP_GAME_STATE_H */
