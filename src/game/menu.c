#include "game/menu.h"
#include "constants/memory.h"
#include "constants/gameplay.h"
#include <string.h>

#define OAM_STAGING_SIZE 256

/*
 * Legal Screen (Mode 0): Displays copyright and licensing information.
 * Source address: $8200 (main.asm lines 516-558).
 */
void gameMode_legalScreen(NES_State *nes) {
    if (!nes) return;

    nes->ram[ADDR_RENDER_MODE] = 0;
    memset(&nes->ram[ADDR_OAM_STAGING], 0, OAM_STAGING_SIZE);

    /*
     * In original assembly (main.asm lines 543-557):
     * lda #LEGAL_SLEEP_TIME; jsr sleep_for_a_vblanks;
     * lda #LEGAL_SLEEP_TIME; sta generalCounter;
     * @waitForStartButton:
     * lda newlyPressedButtons_player1; cmp #BUTTON_START; beq @continueToNextScreen
     * jsr updateAudioWaitForNmiAndResetOamStaging; dec generalCounter; bne @waitForStartButton
     * @continueToNextScreen:
     * inc gameMode; rts
     */
    nes->ram[ADDR_SLEEP_COUNTER] = 0;
    nes->ram[ADDR_GENERAL_COUNTER] = 0;
    nes->ram[ADDR_GAME_MODE]++;
}

/*
 * Frame-based step helper for legal screen (Mode 0).
 */
void gameMode_legalScreen_step(NES_State *nes) {
    if (!nes) return;

    nes->ram[ADDR_RENDER_MODE] = 0;

    /* Player 1 START button skips the remaining legal screen delay */
    if (nes->ram[ADDR_NEWLY_PRESSED_P1] & BUTTON_START) {
        nes->ram[ADDR_GAME_MODE] = MODE_TITLE_SCREEN;
        return;
    }

    if (nes->ram[ADDR_SLEEP_COUNTER] > 0) {
        nes->ram[ADDR_SLEEP_COUNTER]--;
        return;
    }

    if (nes->ram[ADDR_GENERAL_COUNTER] > 0) {
        nes->ram[ADDR_GENERAL_COUNTER]--;
        if (nes->ram[ADDR_GENERAL_COUNTER] == 0) {
            nes->ram[ADDR_GAME_MODE] = MODE_TITLE_SCREEN;
        }
    } else {
        nes->ram[ADDR_GAME_MODE] = MODE_TITLE_SCREEN;
    }
}

/*
 * Title Screen (Mode 1): Displays title logo and prompt.
 * Source address: $824F (main.asm lines 559-635).
 */
void gameMode_titleScreen(NES_State *nes) {
    if (!nes) return;

    nes->ram[ADDR_RENDER_MODE] = 0;
    nes->ram[ADDR_DEMO_RECORD_FLAG] = 0;
    nes->ram[ADDR_DISPLAY_NEXT_PIECE] = 0;
    memset(&nes->ram[ADDR_OAM_STAGING], 0, OAM_STAGING_SIZE);

    /*
     * In original assembly (main.asm lines 609-634):
     * @waitForStartButton:
     * jsr updateAudioWaitForNmiAndResetOamStaging
     * lda newlyPressedButtons_player1; cmp #BUTTON_START; beq @startButtonPressed
     * lda frameCounter+1; cmp #$05; beq @timeout; jmp @waitForStartButton
     * @startButtonPressed:
     * lda #$02; sta soundEffectSlot1Init; inc gameMode; rts
     * @timeout:
     * lda #$02; sta soundEffectSlot1Init; lda #$06; sta gameMode; rts
     */
    if (nes->ram[ADDR_NEWLY_PRESSED_P1] & BUTTON_START) {
        nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = SFX_MENU_SELECT;
        nes->ram[ADDR_GAME_MODE]++;
    } else if (nes->ram[ADDR_FRAME_COUNTER + 1] >= TITLE_SCREEN_TIMEOUT_HI) {
        nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = SFX_MENU_SELECT;
        nes->ram[ADDR_GAME_MODE] = MODE_START_DEMO;
    } else {
        nes->ram[ADDR_FRAME_COUNTER + 1] = 0;
    }
}

/*
 * Frame-based step helper for title screen (Mode 1).
 */
void gameMode_titleScreen_step(NES_State *nes) {
    if (!nes) return;

    nes->ram[ADDR_RENDER_MODE] = 0;
    nes->ram[ADDR_DEMO_RECORD_FLAG] = 0;
    nes->ram[ADDR_DISPLAY_NEXT_PIECE] = 0;

    if (nes->ram[ADDR_NEWLY_PRESSED_P1] & BUTTON_START) {
        nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = SFX_MENU_SELECT;
        nes->ram[ADDR_GAME_MODE] = MODE_GAME_TYPE_MENU;
        return;
    }

    if (nes->ram[ADDR_FRAME_COUNTER + 1] >= TITLE_SCREEN_TIMEOUT_HI) {
        nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = SFX_MENU_SELECT;
        nes->ram[ADDR_GAME_MODE] = MODE_START_DEMO;
        return;
    }
}

/*
 * Start Demo (Mode 6): Resets game configuration for demo playback and enters Mode 5.
 * Source address: $9DF6 (main.asm lines 3735-3743).
 */
void gameMode_startDemo(NES_State *nes) {
    if (!nes) return;

    /*
     * Source address: $9DF6 (main.asm lines 3735-3743)
     * lda #$00
     * sta gameType
     * sta player1_startLevel
     * sta gameModeState
     * sta player1_playState
     * lda #$05
     * sta gameMode
     */
    nes->ram[ADDR_GAME_TYPE] = 0;
    nes->ram[ADDR_P1_START_LEVEL] = 0;
    nes->ram[ADDR_GAME_MODE_STATE] = 0;
    nes->ram[ADDR_P1_PLAY_STATE] = 0;
    nes->ram[ADDR_GAME_MODE] = MODE_DEMO; /* 5 */
}

/*
 * Master Game Mode Dispatcher: Dispatches execution across gameModes 0 to 6.
 * Source address: $8161 (main.asm lines 432-441).
 */
void branchOnGameMode(NES_State *nes) {
    if (!nes) return;

    uint8_t mode = nes->ram[ADDR_GAME_MODE];
    switch (mode) {
        case MODE_LEGAL_SCREEN:
            gameMode_legalScreen(nes);
            break;
        case MODE_TITLE_SCREEN:
            gameMode_titleScreen(nes);
            break;
        case MODE_GAME_TYPE_MENU:
            /* Decompilation deferred to subsequent batch */
            break;
        case MODE_LEVEL_MENU:
            /* Decompilation deferred to subsequent batch */
            break;
        case MODE_PLAY:
        case MODE_PLAY_ENDING_HIGH_SCORE:
            /* Gameplay state machine dispatched in Mode 4/5 */
            break;
        case MODE_START_DEMO:
            gameMode_startDemo(nes);
            break;
        default:
            /* Out of range safety */
            break;
    }
}
