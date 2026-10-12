#include "game/menu.h"
#include "game/highscore.h"
#include "constants/memory.h"
#include "constants/gameplay.h"
#include "math/bcd.h"
#include "math/rng.h"
#include <string.h>

#define OAM_STAGING_SIZE 256

/*
 * Authoritative Menu Navigation Tables (PRG ROM addresses $85B2..$85D1)
 */
const uint8_t levelToSpriteYOffset[10] = {
    0x53, 0x53, 0x53, 0x53, 0x53, 0x63, 0x63, 0x63, 0x63, 0x63
};

const uint8_t levelToSpriteXOffset[10] = {
    0x34, 0x44, 0x54, 0x64, 0x74, 0x34, 0x44, 0x54, 0x64, 0x74
};

const uint8_t heightToPpuHighAddr[6] = {
    0x53, 0x53, 0x53, 0x63, 0x63, 0x63
};

const uint8_t heightToPpuLowAddr[6] = {
    0x9C, 0xAC, 0xBC, 0x9C, 0xAC, 0xBC
};

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
 * Frame-based step helper for Game Type Menu (Mode 2).
 * Source address: $830B (main.asm lines 683-797).
 */
void gameMode_gameTypeMenu_step(NES_State *nes) {
    if (!nes) return;

    nes->ram[ADDR_RENDER_MODE] = 1;
    memset(&nes->ram[ADDR_OAM_STAGING], 0xFF, OAM_STAGING_SIZE);

    uint8_t pressed = nes->ram[ADDR_NEWLY_PRESSED_P1];

    /* Horizontal navigation: Type A vs Type B */
    if (pressed == BUTTON_RIGHT) {
        nes->ram[ADDR_GAME_TYPE] = GAMETYPE_B;
        nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = SFX_MENU_MOVE;
    } else if (pressed == BUTTON_LEFT) {
        nes->ram[ADDR_GAME_TYPE] = GAMETYPE_A;
        nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = SFX_MENU_MOVE;
    }

    /* Vertical navigation: Music Type 0..3 */
    if (pressed == BUTTON_DOWN) {
        nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = SFX_MENU_MOVE;
        uint8_t music = nes->ram[ADDR_MUSIC_TYPE];
        if (music < 3) {
            music++;
            nes->ram[ADDR_MUSIC_TYPE] = music;
            setMusicTrack(nes, musicSelectionTable[music]);
        }
    } else if (pressed == BUTTON_UP) {
        nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = SFX_MENU_MOVE;
        uint8_t music = nes->ram[ADDR_MUSIC_TYPE];
        if (music > 0) {
            music--;
            nes->ram[ADDR_MUSIC_TYPE] = music;
            setMusicTrack(nes, musicSelectionTable[music]);
        }
    }

    /* Screen transitions */
    if (pressed == BUTTON_START) {
        nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = SFX_MENU_SELECT;
        nes->ram[ADDR_GAME_MODE]++; /* Advances to MODE_LEVEL_MENU (3) */
        return;
    }

    if (pressed == BUTTON_B) {
        nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = SFX_MENU_SELECT;
        nes->ram[ADDR_FRAME_COUNTER + 1] = 0;
        nes->ram[ADDR_GAME_MODE]--; /* Retreats to MODE_TITLE_SCREEN (1) */
        return;
    }

    /* Stage cursor sprite offsets */
    uint8_t gt = nes->ram[ADDR_GAME_TYPE] & 1;
    nes->ram[ADDR_SPRITE_X_OFFSET] = (uint8_t)(0x3F + gt * 0x30);
    nes->ram[ADDR_SPRITE_Y_OFFSET] = 0x3F;
    nes->ram[ADDR_SPRITE_INDEX_OAM] = ((nes->ram[ADDR_FRAME_COUNTER] & MENU_CURSOR_MASK) != 0) ? 0x01 : 0x02;

    uint8_t mt = nes->ram[ADDR_MUSIC_TYPE];
    if (mt > 3) mt = 3;
    nes->ram[ADDR_SPRITE_Y_OFFSET] = (uint8_t)(0x8F + mt * 0x10);
    nes->ram[ADDR_SPRITE_INDEX_OAM] = ((nes->ram[ADDR_FRAME_COUNTER] & MENU_CURSOR_MASK) != 0) ? 0x53 : 0x02;
    nes->ram[ADDR_SPRITE_X_OFFSET] = 0x67;
}

/*
 * Game Type & Music Selection Menu (Mode 2):
 * Source address: $82D1 (main.asm lines 657-797).
 */
void gameMode_gameTypeMenu(NES_State *nes) {
    if (!nes) return;

    nes->ram[ADDR_RENDER_MODE] = 1;
    uint8_t music = nes->ram[ADDR_MUSIC_TYPE];
    if (music > 3) music = 3;
    setMusicTrack(nes, musicSelectionTable[music]);

    gameMode_gameTypeMenu_step(nes);
}

/*
 * Handle Level and Height Navigation:
 * Processes D-pad inputs for level and height adjustment, Button A toggling in Type B,
 * and stages cursor sprite coordinates in zero-page variables.
 * Source address: $84AE (main.asm lines 920-1064).
 */
void gameMode_levelMenu_handleLevelHeightNavigation(NES_State *nes) {
    if (!nes) return;

    uint8_t pressed = nes->ram[ADDR_NEWLY_PRESSED_BUTTONS];
    uint8_t selecting = nes->ram[ADDR_SELECTING_LEVEL_OR_HEIGHT];

    /* RIGHT */
    if (pressed == BUTTON_RIGHT) {
        nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = SFX_MENU_MOVE;
        if (selecting == 0) {
            if (nes->ram[ADDR_START_LEVEL] < LEVEL_MAX) {
                nes->ram[ADDR_START_LEVEL]++;
            }
        } else {
            if (nes->ram[ADDR_START_HEIGHT] < HEIGHT_MAX) {
                nes->ram[ADDR_START_HEIGHT]++;
            }
        }
    }

    /* LEFT */
    if (pressed == BUTTON_LEFT) {
        nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = SFX_MENU_MOVE;
        if (selecting == 0) {
            if (nes->ram[ADDR_START_LEVEL] > 0) {
                nes->ram[ADDR_START_LEVEL]--;
            }
        } else {
            if (nes->ram[ADDR_START_HEIGHT] > 0) {
                nes->ram[ADDR_START_HEIGHT]--;
            }
        }
    }

    /* DOWN */
    if (pressed == BUTTON_DOWN) {
        nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = SFX_MENU_MOVE;
        if (selecting == 0) {
            if (nes->ram[ADDR_START_LEVEL] < 5) {
                nes->ram[ADDR_START_LEVEL] += 5;
            }
        } else {
            if (nes->ram[ADDR_START_HEIGHT] < 3) {
                nes->ram[ADDR_START_HEIGHT] += 3;
            }
        }
    }

    /* UP */
    if (pressed == BUTTON_UP) {
        nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = SFX_MENU_MOVE;
        if (selecting == 0) {
            if (nes->ram[ADDR_START_LEVEL] >= 5) {
                nes->ram[ADDR_START_LEVEL] -= 5;
            }
        } else {
            if (nes->ram[ADDR_START_HEIGHT] >= 3) {
                nes->ram[ADDR_START_HEIGHT] -= 3;
            }
        }
    }

    /* BUTTON_A (Only toggles between Level and Height in Type B) */
    if (nes->ram[ADDR_GAME_TYPE] != GAMETYPE_A) {
        if (pressed == BUTTON_A) {
            nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = SFX_MENU_MOVE;
            nes->ram[ADDR_SELECTING_LEVEL_OR_HEIGHT] ^= 0x01;
            selecting = nes->ram[ADDR_SELECTING_LEVEL_OR_HEIGHT];
        }
    }

    /* Stage cursor sprite coordinates in zero page */
    bool show_level = (selecting != 0) || ((nes->ram[ADDR_FRAME_COUNTER] & MENU_CURSOR_MASK) != 0);
    if (show_level) {
        uint8_t lvl = nes->ram[ADDR_START_LEVEL];
        if (lvl > LEVEL_MAX) lvl = LEVEL_MAX;
        uint8_t spr_y = levelToSpriteYOffset[lvl];
        uint8_t spr_x = levelToSpriteXOffset[lvl];
        if (nes->ram[ADDR_ACTIVE_PLAYER] == 1) {
            spr_y += 0x50;
        }
        nes->ram[ADDR_SPRITE_Y_OFFSET] = spr_y;
        nes->ram[ADDR_SPRITE_X_OFFSET] = spr_x;
        nes->ram[ADDR_SPRITE_INDEX_OAM] = 0x00;
    }

    if (nes->ram[ADDR_GAME_TYPE] != GAMETYPE_A) {
        bool show_height = (selecting == 0) || ((nes->ram[ADDR_FRAME_COUNTER] & MENU_CURSOR_MASK) != 0);
        if (show_height) {
            uint8_t hgt = nes->ram[ADDR_START_HEIGHT];
            if (hgt > HEIGHT_MAX) hgt = HEIGHT_MAX;
            uint8_t h_y = heightToPpuHighAddr[hgt];
            uint8_t h_x = heightToPpuLowAddr[hgt];
            if (nes->ram[ADDR_ACTIVE_PLAYER] == 1) {
                h_y += 0x50;
            }
            nes->ram[ADDR_SPRITE_Y_OFFSET] = h_y;
            nes->ram[ADDR_SPRITE_X_OFFSET] = h_x;
            nes->ram[ADDR_SPRITE_INDEX_OAM] = 0x00;
        }
    }
}

/*
 * Process Player 1 Navigation in Level Menu:
 * Handles active player state mapping, calling navigation handler,
 * checking START / B button edge transitions, and garbage hole generation.
 * Source address: $8436 (main.asm lines 845-918).
 */
void gameMode_levelMenu_processPlayer1Navigation(NES_State *nes) {
    if (!nes) return;

    nes->ram[ADDR_ACTIVE_PLAYER] = 0;
    nes->ram[ADDR_START_LEVEL] = nes->ram[ADDR_P1_START_LEVEL];
    nes->ram[ADDR_START_HEIGHT] = nes->ram[ADDR_P1_START_HEIGHT];
    nes->ram[ADDR_SELECTING_LEVEL_OR_HEIGHT] = nes->ram[ADDR_ORIGINAL_Y];
    nes->ram[ADDR_NEWLY_PRESSED_BUTTONS] = nes->ram[ADDR_NEWLY_PRESSED_P1];

    gameMode_levelMenu_handleLevelHeightNavigation(nes);

    nes->ram[ADDR_P1_START_LEVEL] = nes->ram[ADDR_START_LEVEL];
    nes->ram[ADDR_P1_START_HEIGHT] = nes->ram[ADDR_START_HEIGHT];
    nes->ram[ADDR_ORIGINAL_Y] = nes->ram[ADDR_SELECTING_LEVEL_OR_HEIGHT];

    /* START Button handling (with A+START Level+10 cheat code) */
    if (nes->ram[ADDR_NEWLY_PRESSED_P1] == BUTTON_START) {
        if (nes->ram[ADDR_HELD_BUTTONS_P1] == (BUTTON_A | BUTTON_START)) {
            nes->ram[ADDR_P1_START_LEVEL] += 10;
        }
        nes->ram[ADDR_GAME_MODE_STATE] = 0;
        nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = SFX_MENU_SELECT;
        nes->ram[ADDR_GAME_MODE]++; /* Advances to MODE_PLAY (4) */
        return;
    }

    /* BUTTON_B retreats to Game Type Menu */
    if (nes->ram[ADDR_NEWLY_PRESSED_P1] == BUTTON_B) {
        nes->ram[ADDR_SOUND_EFFECT_SLOT1_INIT] = SFX_MENU_SELECT;
        nes->ram[ADDR_GAME_MODE]--; /* Retreats to MODE_GAME_TYPE_MENU (2) */
        return;
    }

    /* Generate random garbage hole column (0..9) for Player 1 & 2 */
    do {
        generateNextPseudorandomNumber(nes, ADDR_RNG_SEED, 2);
    } while ((nes->ram[ADDR_RNG_SEED] & 0x0F) >= 10);
    nes->ram[ADDR_P1_GARBAGE_HOLE] = nes->ram[ADDR_RNG_SEED] & 0x0F;

    do {
        generateNextPseudorandomNumber(nes, ADDR_RNG_SEED, 2);
    } while ((nes->ram[ADDR_RNG_SEED] & 0x0F) >= 10);
    nes->ram[ADDR_P2_GARBAGE_HOLE] = nes->ram[ADDR_RNG_SEED] & 0x0F;
}

/*
 * Frame-based step helper for Level Menu (Mode 3).
 */
void gameMode_levelMenu_step(NES_State *nes) {
    if (!nes) return;
    gameMode_levelMenu_processPlayer1Navigation(nes);
}

/*
 * Level & Height Selection Menu (Mode 3):
 * Source address: $83D7 (main.asm lines 799-918).
 */
void gameMode_levelMenu(NES_State *nes) {
    if (!nes) return;

    nes->ram[ADDR_RENDER_MODE] = 1;
    showHighScores(nes);
    nes->ram[ADDR_ORIGINAL_Y] = 0;
    nes->ram[ADDR_DROP_SPEED] = 0;

    /* Normalize startLevel to range 0..9 (main.asm lines 836-843) */
    while (nes->ram[ADDR_P1_START_LEVEL] >= 10) {
        nes->ram[ADDR_P1_START_LEVEL] -= 10;
    }

    gameMode_levelMenu_processPlayer1Navigation(nes);
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
            gameMode_gameTypeMenu(nes);
            break;
        case MODE_LEVEL_MENU:
            gameMode_levelMenu(nes);
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
