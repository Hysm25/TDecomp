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

/*
 * Memory clearing routine filling 256-byte pages from end_page down to start_page
 * with value val. Sets tmp1 = 0, tmp2 = start_page - 1.
 * Source address: $AC6A (main.asm lines 5635-5653).
 */
void memset_page(NES_State *nes, uint8_t val, uint8_t start_page, uint8_t end_page);

/*
 * Evaluates game over state: in 1P mode triggers high-score check and name entry,
 * clears playfields to tileEmpty, and transitions gameMode to MODE_LEVEL_MENU (3).
 * In 2P mode transitions gameModeState to 9. If not game over, increments gameModeState.
 * Source address: $9CBF (main.asm lines 3545-3595).
 */
void gameModeState_handleGameOver(NES_State *nes);

/*
 * Checks for A + B + SELECT + START soft reset combo on Player 1.
 * If active, resets gameMode to MODE_LEGAL_SCREEN (0). Otherwise increments gameModeState.
 * Source address: $9E16 (main.asm lines 3756-3770).
 */
void gameModeState_checkForResetKeyCombo(NES_State *nes);

/*
 * Handles START button during gameplay and demo mode:
 * In Mode 5 (Demo): exits demo to Title Screen (gameMode = 1).
 * In Game (renderMode = 3): handles pause toggle, PAUSE sprite staging, and resume.
 * Source address: $A37F (main.asm lines 4475-4544).
 */
void gameModeState_startButtonHandling(NES_State *nes);
void gameModeState_startButtonHandling_step(NES_State *nes);

/*
 * Authoritative 44-byte nametable patch for Type B playing background.
 * Source address: $86B0 (main.asm lines 1183-1189).
 */
extern const uint8_t game_typeb_nametable_patch[44];

/*
 * Initializes the game background, copying palettes, nametables, and writing
 * the top high score (from RAM $0730 Type A or $073C Type B) to PPU nametable.
 * Source address: $85F0 (main.asm lines 1092-1167).
 */
void gameModeState_initGameBackground(NES_State *nes);

/*
 * Completes background initialization, setting playStates to 1, copying startLevel
 * to levelNumber, and incrementing gameModeState.
 * Source address: $8693 (main.asm lines 1168-1182).
 */
void gameModeState_initGameBackground_finish(NES_State *nes);

/*
 * Initializes game state for playing and demo modes: clears playfield, stats,
 * scores, lines, sets demo pointer, chooses starting tetriminos, and sets music.
 * Source address: $86DC (main.asm lines 1190-1268).
 */
void gameModeState_initGameState(NES_State *nes);

/*
 * Master state machine dispatcher for Mode 4 (Playing) and Mode 5 (Ending / Demo),
 * jumping through switch_s_plus_2a over states 0..8 of gameModeState.
 * Source address: $819B (main.asm lines 462-473).
 */
void gameMode_playAndEndingHighScore(NES_State *nes);

/*
 * JSR wrapper jump target for Mode 4 and Mode 5 from branchOnGameMode.
 * Source address: $815D (main.asm lines 428-431).
 */
void gameMode_playAndEndingHighScore_jmp(NES_State *nes);

/*
 * Mode 4 / 5 State 4: updates Player 1 gameplay state.
 * Activates P1, branches on playState, stages current piece sprites,
 * saves P1 state, stages next piece sprite, and increments gameModeState.
 * Source address: $8174 (main.asm lines 442-450).
 */
void gameModeState_updatePlayer1(NES_State *nes);

/*
 * Mode 4 / 5 State 5: updates Player 2 gameplay state (2-player mode only).
 * If numberOfPlayers == 2, activates P2, branches on playState, stages sprites,
 * and saves P2 state. Increments gameModeState.
 * Source address: $8186 (main.asm lines 451-461).
 */
void gameModeState_updatePlayer2(NES_State *nes);

/*
 * Return stub for playState_bTypeGoalCheck: increments playState and returns.
 * Source address: $A42B (main.asm lines 4585-4588).
 */
void playState_bTypeGoalCheck_ret(NES_State *nes);

#ifdef __cplusplus
}
#endif

#endif /* TDECOMP_GAME_STATE_H */
