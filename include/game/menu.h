#ifndef TDECOMP_GAME_MENU_H
#define TDECOMP_GAME_MENU_H

#include "common.h"
#include "nes.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Legal Screen (Mode 0): Displays copyright and licensing information.
 * Waits for sleep counter countdown or player 1 START button press,
 * then advances gameMode to MODE_TITLE_SCREEN (1).
 * Source address: $8200 (main.asm lines 516-558).
 */
void gameMode_legalScreen(NES_State *nes);

/*
 * Title Screen (Mode 1): Displays title logo and prompt.
 * Waits for player 1 START button press (advancing to MODE_GAME_TYPE_MENU, 2)
 * or timeout (frameCounter+1 == $05, advancing to MODE_START_DEMO, 6).
 * Source address: $824F (main.asm lines 559-635).
 */
void gameMode_titleScreen(NES_State *nes);

/*
 * Start Demo (Mode 6): Resets game configuration for demo playback and
 * enters MODE_DEMO (5).
 * Source address: $9DF6 (main.asm lines 3735-3745).
 */
void gameMode_startDemo(NES_State *nes);

/*
 * Master Game Mode Dispatcher: Dispatches execution across gameModes 0 to 6.
 * Source address: $8161 (main.asm lines 432-441).
 */
void branchOnGameMode(NES_State *nes);

/*
 * Frame-based step helper for legal screen (Mode 0).
 * Useful for modern portable game loops (Linux, Windows, Android).
 */
void gameMode_legalScreen_step(NES_State *nes);

/*
 * Frame-based step helper for title screen (Mode 1).
 * Useful for modern portable game loops (Linux, Windows, Android).
 */
void gameMode_titleScreen_step(NES_State *nes);

#ifdef __cplusplus
}
#endif

#endif /* TDECOMP_GAME_MENU_H */
