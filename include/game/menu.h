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

/*
 * Authoritative Menu Navigation Tables (PRG ROM addresses $85B2..$85D1)
 */
extern const uint8_t levelToSpriteYOffset[10];
extern const uint8_t levelToSpriteXOffset[10];
extern const uint8_t heightToPpuHighAddr[6];
extern const uint8_t heightToPpuLowAddr[6];

/*
 * Game Type & Music Selection Menu (Mode 2):
 * Allows player 1 to toggle Game Type (Type A vs Type B) and Music Type (0..3).
 * START advances to MODE_LEVEL_MENU (3).
 * BUTTON_B retreats to MODE_TITLE_SCREEN (1).
 * Source address: $82D1 (main.asm lines 657-797).
 */
void gameMode_gameTypeMenu(NES_State *nes);

/*
 * Frame-based step helper for Game Type Menu (Mode 2).
 */
void gameMode_gameTypeMenu_step(NES_State *nes);

/*
 * Level & Height Selection Menu (Mode 3):
 * Allows player 1 to select starting level (0..9) and starting height (0..5 in Type B).
 * START advances to MODE_PLAY (4), supporting the A+START level+10 cheat code.
 * BUTTON_B retreats to MODE_GAME_TYPE_MENU (2).
 * Source address: $83D7 (main.asm lines 799-918).
 */
void gameMode_levelMenu(NES_State *nes);

/*
 * Process Player 1 Navigation in Level Menu:
 * Handles active player state mapping, calling navigation handler,
 * checking START / B button edge transitions, and garbage hole generation.
 * Source address: $8436 (main.asm lines 845-918).
 */
void gameMode_levelMenu_processPlayer1Navigation(NES_State *nes);

/*
 * Handle Level and Height Navigation:
 * Processes D-pad inputs for level and height adjustment, Button A toggling in Type B,
 * and stages cursor sprite coordinates in zero-page variables.
 * Source address: $84AE (main.asm lines 920-1064).
 */
void gameMode_levelMenu_handleLevelHeightNavigation(NES_State *nes);

/*
 * Frame-based step helper for Level Menu (Mode 3).
 */
void gameMode_levelMenu_step(NES_State *nes);

#ifdef __cplusplus
}
#endif

#endif /* TDECOMP_GAME_MENU_H */
