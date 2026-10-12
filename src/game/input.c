#include "game/input.h"
#include <string.h>

/*
 * Bit-reversal helper for serial shift register communication.
 * NES controller shifts out A, B, Select, Start, Up, Down, Left, Right serially.
 */
static inline uint8_t button_mask_to_serial(uint8_t mask) {
    uint8_t rev = 0;
    for (int i = 0; i < 8; i++) {
        rev = (uint8_t)((rev << 1) | (mask & 1));
        mask >>= 1;
    }
    return rev;
}

void nes_set_controller(NES_State *nes, uint8_t player, uint8_t button_mask) {
    if (!nes) return;
    uint8_t serial = button_mask_to_serial(button_mask);
    if (player == 0) {
        nes->joy1_latch = serial;
        nes->joy1_shift = serial;
    } else if (player == 1) {
        nes->joy2_latch = serial;
        nes->joy2_shift = serial;
    }
}

/*
 * Strobes and reads JOY1 ($4016) and JOY2 ($4017) serially for 8 bits.
 * Shifts bits into newlyPressedButtons_player1/2 and tmp1/2.
 * Source address: $AB69 (main.asm lines 5493-5513).
 */
void pollController_actualRead(NES_State *nes) {
    if (!nes) return;

    /* Strobe JOY1 ($4016) */
    uint8_t joy1_loc = nes->ram[ADDR_JOY1_LOCATION];
    nes_write8(nes, REG_JOY1, (uint8_t)(joy1_loc + 1));
    nes_write8(nes, REG_JOY1, joy1_loc);

    uint8_t p1 = 0;
    uint8_t p2 = 0;
    uint8_t t1 = 0;
    uint8_t t2 = 0;

    for (int i = 0; i < 8; i++) {
        uint8_t j1 = nes_read8(nes, REG_JOY1);
        p1 = (uint8_t)((p1 << 1) | (j1 & 1));
        t1 = (uint8_t)((t1 << 1) | ((j1 >> 1) & 1));

        uint8_t j2 = nes_read8(nes, REG_JOY2_APUFC);
        p2 = (uint8_t)((p2 << 1) | (j2 & 1));
        t2 = (uint8_t)((t2 << 1) | ((j2 >> 1) & 1));
    }

    nes->ram[ADDR_NEWLY_PRESSED_P1] = p1;
    nes->ram[ADDR_NEWLY_PRESSED_P2] = p2;
    nes->ram[ADDR_TMP1] = t1;
    nes->ram[ADDR_TMP2] = t2;
}

/*
 * Combines Famicom expansion port input (tmp1/2) with controller inputs.
 * Source address: $AB8B (main.asm lines 5515-5522).
 */
void addExpansionPortInputAsControllerInput(NES_State *nes) {
    if (!nes) return;
    nes->ram[ADDR_NEWLY_PRESSED_P1] |= nes->ram[ADDR_TMP1];
    nes->ram[ADDR_NEWLY_PRESSED_P2] |= nes->ram[ADDR_TMP2];
}

/*
 * Distinguishes newly pressed buttons from continuously held buttons:
 * newlyPressed = (current ^ held) & current;
 * held = current;
 * Source address: $ABBD (main.asm lines 5541-5552).
 */
void diffOldAndNewButtons(NES_State *nes) {
    if (!nes) return;

    /* Player 2 (X=1) */
    uint8_t curr2 = nes->ram[ADDR_NEWLY_PRESSED_P2];
    uint8_t held2 = nes->ram[ADDR_HELD_BUTTONS_P2];
    nes->ram[ADDR_NEWLY_PRESSED_P2] = (uint8_t)((curr2 ^ held2) & curr2);
    nes->ram[ADDR_HELD_BUTTONS_P2] = curr2;

    /* Player 1 (X=0) */
    uint8_t curr1 = nes->ram[ADDR_NEWLY_PRESSED_P1];
    uint8_t held1 = nes->ram[ADDR_HELD_BUTTONS_P1];
    nes->ram[ADDR_NEWLY_PRESSED_P1] = (uint8_t)((curr1 ^ held1) & curr1);
    nes->ram[ADDR_HELD_BUTTONS_P1] = curr1;
}

/*
 * Performs double-reading of controllers to debounce against DPCM DMA glitch,
 * then calls diffOldAndNewButtons.
 * Source address: $AB9D (main.asm lines 5526-5540).
 */
void pollController(NES_State *nes) {
    if (!nes) return;

    pollController_actualRead(nes);
    addExpansionPortInputAsControllerInput(nes);

    nes->ram[ADDR_GENERAL_COUNTER2] = nes->ram[ADDR_NEWLY_PRESSED_P1];
    nes->ram[ADDR_GENERAL_COUNTER3] = nes->ram[ADDR_NEWLY_PRESSED_P2];

    pollController_actualRead(nes);
    addExpansionPortInputAsControllerInput(nes);

    nes->ram[ADDR_NEWLY_PRESSED_P1] &= nes->ram[ADDR_GENERAL_COUNTER2];
    nes->ram[ADDR_NEWLY_PRESSED_P2] &= nes->ram[ADDR_GENERAL_COUNTER3];

    diffOldAndNewButtons(nes);
}

/*
 * Increments the 16-bit address at demoButtonsAddr ($00D1) by 1.
 * Source address: $9DE8 (main.asm lines 3725-3733).
 */
void demoButtonsTable_indexIncr(NES_State *nes) {
    if (!nes) return;
    uint16_t addr = (uint16_t)(nes->ram[ADDR_DEMO_BUTTONS_ADDR] |
                              ((uint16_t)nes->ram[ADDR_DEMO_BUTTONS_ADDR + 1] << 8));
    addr++;
    nes->ram[ADDR_DEMO_BUTTONS_ADDR] = (uint8_t)(addr & 0xFF);
    nes->ram[ADDR_DEMO_BUTTONS_ADDR + 1] = (uint8_t)((addr >> 8) & 0xFF);
}

/*
 * Top-level controller polling: in standard game modes, calls pollController;
 * in demo mode ($05), handles demo playback or Start button cancel.
 * Source address: $9D51 (main.asm lines 3630-3686).
 */
void pollControllerButtons(NES_State *nes) {
    if (!nes) return;

    if (nes->ram[ADDR_GAME_MODE] != MODE_DEMO) {
        pollController(nes);
        return;
    }

    /* Demo mode ($05) */
    if (nes->ram[ADDR_DEMO_RECORD_FLAG] == 0xFF) {
        /* Recording mode (original debug feature) */
        pollController(nes);
        if (nes->ram[ADDR_GAME_MODE] != MODE_DEMO || nes->ram[ADDR_DEMO_RECORD_FLAG] != 0xFF) {
            return;
        }
        if (nes->ram[ADDR_HELD_BUTTONS_P1] == nes->ram[ADDR_DEMO_HELD_BUTTONS]) {
            nes->ram[ADDR_DEMO_REPEATS]++;
            return;
        }
        uint16_t addr = (uint16_t)(nes->ram[ADDR_DEMO_BUTTONS_ADDR] |
                                  ((uint16_t)nes->ram[ADDR_DEMO_BUTTONS_ADDR + 1] << 8));
        nes_write8(nes, addr, nes->ram[ADDR_DEMO_HELD_BUTTONS]);
        demoButtonsTable_indexIncr(nes);
        addr = (uint16_t)(nes->ram[ADDR_DEMO_BUTTONS_ADDR] |
                         ((uint16_t)nes->ram[ADDR_DEMO_BUTTONS_ADDR + 1] << 8));
        nes_write8(nes, addr, nes->ram[ADDR_DEMO_REPEATS]);
        demoButtonsTable_indexIncr(nes);
        if (nes->ram[ADDR_DEMO_BUTTONS_ADDR + 1] == DEMO_TETRIMINO_TABLE_ADDR_HI) {
            return;
        }
        nes->ram[ADDR_DEMO_HELD_BUTTONS] = nes->ram[ADDR_HELD_BUTTONS_P1];
        nes->ram[ADDR_DEMO_REPEATS] = 0;
        return;
    }

    /* Demo playback mode */
    pollController(nes);
    if (nes->ram[ADDR_NEWLY_PRESSED_P1] == BUTTON_START) {
        /* Start pressed: cancel demo and return to title */
        nes->ram[ADDR_DEMO_BUTTONS_ADDR + 1] = DEMO_BUTTONS_TABLE_ADDR_HI;
        nes->ram[ADDR_FRAME_COUNTER + 1] = 0x00;
        nes->ram[ADDR_GAME_MODE] = MODE_TITLE_SCREEN;
        return;
    }

    if (nes->ram[ADDR_DEMO_REPEATS] != 0) {
        nes->ram[ADDR_DEMO_REPEATS]--;
        nes->ram[ADDR_NEWLY_PRESSED_P1] = 0x00;
        nes->ram[ADDR_HELD_BUTTONS_P1] = nes->ram[ADDR_DEMO_HELD_BUTTONS];
        return;
    }

    /* Finished current move, fetch next move from demo table */
    uint16_t addr = (uint16_t)(nes->ram[ADDR_DEMO_BUTTONS_ADDR] |
                              ((uint16_t)nes->ram[ADDR_DEMO_BUTTONS_ADDR + 1] << 8));
    uint8_t btn = nes_read8(nes, addr);
    nes->ram[ADDR_GENERAL_COUNTER] = btn;
    demoButtonsTable_indexIncr(nes);

    uint8_t demo_held = nes->ram[ADDR_DEMO_HELD_BUTTONS];
    nes->ram[ADDR_NEWLY_PRESSED_P1] = (uint8_t)((demo_held ^ btn) & btn);
    nes->ram[ADDR_DEMO_HELD_BUTTONS] = btn;

    addr = (uint16_t)(nes->ram[ADDR_DEMO_BUTTONS_ADDR] |
                     ((uint16_t)nes->ram[ADDR_DEMO_BUTTONS_ADDR + 1] << 8));
    uint8_t reps = nes_read8(nes, addr);
    nes->ram[ADDR_DEMO_REPEATS] = reps;
    demoButtonsTable_indexIncr(nes);

    if (nes->ram[ADDR_DEMO_BUTTONS_ADDR + 1] == DEMO_TETRIMINO_TABLE_ADDR_HI) {
        return;
    }

    nes->ram[ADDR_HELD_BUTTONS_P1] = nes->ram[ADDR_DEMO_HELD_BUTTONS];
}

/*
 * Copies player 1 or player 2 inputs to active player variables
 * (newlyPressedButtons, heldButtons) based on activePlayer.
 */
void updateActivePlayerInput(NES_State *nes) {
    if (!nes) return;
    if (nes->ram[ADDR_ACTIVE_PLAYER] == 2) {
        nes->ram[ADDR_NEWLY_PRESSED_BUTTONS] = nes->ram[ADDR_NEWLY_PRESSED_P2];
        nes->ram[ADDR_HELD_BUTTONS] = nes->ram[ADDR_HELD_BUTTONS_P2];
    } else {
        nes->ram[ADDR_NEWLY_PRESSED_BUTTONS] = nes->ram[ADDR_NEWLY_PRESSED_P1];
        nes->ram[ADDR_HELD_BUTTONS] = nes->ram[ADDR_HELD_BUTTONS_P1];
    }
}

/*
 * Processes Delayed Auto Shift (DAS) for horizontal movement.
 * Returns true if a shift should be triggered on this frame, false otherwise.
 * Timing matches NES NTSC (16 frames initial delay, 6 frames repeat interval).
 */
bool processDAS(uint8_t *autorepeatX, uint8_t newlyPressed, uint8_t held) {
    if (!autorepeatX) return false;

    if (held & BUTTON_DOWN) {
        return false;
    }

    if (newlyPressed & (BUTTON_LEFT | BUTTON_RIGHT)) {
        *autorepeatX = 0;
        return true;
    }

    if (!(held & (BUTTON_LEFT | BUTTON_RIGHT))) {
        return false;
    }

    (*autorepeatX)++;
    if (*autorepeatX < DAS_RESET) {
        return false;
    }

    *autorepeatX = DAS_DELAY;
    return true;
}

/*
 * Charges DAS counter to maximum (16 frames) when a tetrimino hits a wall.
 */
void dasWallCharge(uint8_t *autorepeatX) {
    if (autorepeatX) {
        *autorepeatX = DAS_RESET;
    }
}
