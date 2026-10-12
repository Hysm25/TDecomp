#ifndef TDECOMP_GAME_INPUT_H
#define TDECOMP_GAME_INPUT_H

#include "common.h"
#include "nes.h"
#include "constants/gameplay.h"
#include "constants/memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Sets joypad latch for player (0 for P1, 1 for P2) using standard BUTTON_* bitmask.
 * Encodes into serial shift order matching the NES 4021 shift register.
 */
void nes_set_controller(NES_State *nes, uint8_t player, uint8_t button_mask);

/*
 * Strobes and reads JOY1 ($4016) and JOY2 ($4017) serially for 8 bits.
 * Shifts bits into newlyPressedButtons_player1/2 and tmp1/2.
 * Source address: $AB69 (main.asm lines 5493-5513).
 */
void pollController_actualRead(NES_State *nes);

/*
 * Combines Famicom expansion port input (tmp1/2) with controller inputs.
 * Source address: $AB8B (main.asm lines 5515-5522).
 */
void addExpansionPortInputAsControllerInput(NES_State *nes);

/*
 * Distinguishes newly pressed buttons from continuously held buttons:
 * newlyPressed = (current ^ held) & current;
 * held = current;
 * Source address: $ABBD (main.asm lines 5541-5552).
 */
void diffOldAndNewButtons(NES_State *nes);

/*
 * Performs double-reading of controllers to debounce against DPCM DMA glitch,
 * then calls diffOldAndNewButtons.
 * Source address: $AB9D (main.asm lines 5526-5540).
 */
void pollController(NES_State *nes);

/*
 * Top-level controller polling: in standard game modes, calls pollController;
 * in demo mode ($05), handles demo playback or Start button cancel.
 * Source address: $9D51 (main.asm lines 3630-3686).
 */
void pollControllerButtons(NES_State *nes);

/*
 * Increments the 16-bit address at demoButtonsAddr ($00D1) by 1.
 * Source address: $9DE8 (main.asm lines 3725-3733).
 */
void demoButtonsTable_indexIncr(NES_State *nes);

/*
 * Copies player 1 or player 2 inputs to active player variables
 * (newlyPressedButtons, heldButtons) based on activePlayer.
 */
void updateActivePlayerInput(NES_State *nes);

/*
 * Processes Delayed Auto Shift (DAS) for horizontal movement.
 * Returns true if a shift should be triggered on this frame, false otherwise.
 * Timing matches NES NTSC (16 frames initial delay, 6 frames repeat interval).
 */
bool processDAS(uint8_t *autorepeatX, uint8_t newlyPressed, uint8_t held);

/*
 * Charges DAS counter to maximum (16 frames) when a tetrimino hits a wall.
 */
void dasWallCharge(uint8_t *autorepeatX);

#ifdef __cplusplus
}
#endif

#endif /* TDECOMP_GAME_INPUT_H */
