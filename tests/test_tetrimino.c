#include <stdio.h>
#include <assert.h>
#include "nes.h"
#include "game/tetrimino.h"

int test_tetrimino_subsystem(void) {
    NES_State nes;
    nes_init(&nes);

    /* Initialize empty playfield */
    nes_write16(&nes, ADDR_PLAYFIELD_ADDR, ADDR_PLAYFIELD);
    for (int i = 0; i < PLAYFIELD_SIZE; i++) {
        nes.ram[ADDR_PLAYFIELD + i] = TILE_EMPTY;
    }

    /* Test 1: isPositionValid center of board */
    nes.ram[ADDR_TETRIMINO_X] = 5;
    nes.ram[ADDR_TETRIMINO_Y] = 5;
    nes.ram[ADDR_CURRENT_PIECE] = ORIENT_T_DOWN;
    assert(isPositionValid(&nes) == 0);

    /* Test 2: Wall collision - shift left out of bounds */
    nes.ram[ADDR_TETRIMINO_X] = 0;
    /* tDown has mino at x_offset = -1: 0 + (-1) = -1 < 0 -> Invalid! */
    assert(isPositionValid(&nes) != 0);

    /* Move to X = 1: 1 + (-1) = 0 -> Valid! */
    nes.ram[ADDR_TETRIMINO_X] = 1;
    assert(isPositionValid(&nes) == 0);

    /* Test 3: Wall collision - shift right out of bounds */
    /* tDown has mino at x_offset = +1: X = 9 -> 9 + 1 = 10 >= 10 -> Invalid! */
    nes.ram[ADDR_TETRIMINO_X] = 9;
    assert(isPositionValid(&nes) != 0);

    /* Test 4: Floor collision - Y reaching bottom (row 20) */
    nes.ram[ADDR_TETRIMINO_X] = 5;
    nes.ram[ADDR_TETRIMINO_Y] = 19;
    /* tDown mino 3 has y_offset = 1: 19 + 1 = 20 -> row 20 >= 20 -> Invalid! */
    assert(isPositionValid(&nes) != 0);

    /* Test 5: Placed mino collision in playfield */
    nes.ram[ADDR_TETRIMINO_X] = 5;
    nes.ram[ADDR_TETRIMINO_Y] = 10;
    assert(isPositionValid(&nes) == 0);

    /* Place block at (row 10, col 5): cell offset = 10 * 10 + 5 = 105 */
    nes.ram[ADDR_PLAYFIELD + 105] = TILE_1;
    assert(isPositionValid(&nes) != 0);

    /* Clear block */
    nes.ram[ADDR_PLAYFIELD + 105] = TILE_EMPTY;
    assert(isPositionValid(&nes) == 0);

    /* Test 6: rotate_tetrimino Button A (Clockwise) */
    nes.ram[ADDR_CURRENT_PIECE] = ORIENT_T_UP;
    nes.ram[ADDR_NEWLY_PRESSED_BUTTONS] = BUTTON_A;
    rotate_tetrimino(&nes);
    /* tUp clockwise -> tRight */
    assert(nes.ram[ADDR_CURRENT_PIECE] == ORIENT_T_RIGHT);
    assert(nes.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == 0x05);

    /* Rotate against left wall: should revert */
    nes.ram[ADDR_TETRIMINO_X] = 0;
    nes.ram[ADDR_CURRENT_PIECE] = ORIENT_T_RIGHT; /* tRight minos at X=0, X=1 */
    /* Rotate with Button A to tDown (minos at X=-1, X=0, X=1) -> collision with left wall! */
    nes.ram[ADDR_NEWLY_PRESSED_BUTTONS] = BUTTON_A;
    rotate_tetrimino(&nes);
    assert(nes.ram[ADDR_CURRENT_PIECE] == ORIENT_T_RIGHT); /* Reverted! */

    /* Test 7: shift_tetrimino horizontal movement */
    nes.ram[ADDR_TETRIMINO_X] = 5;
    nes.ram[ADDR_HELD_BUTTONS] = BUTTON_RIGHT;
    nes.ram[ADDR_NEWLY_PRESSED_BUTTONS] = BUTTON_RIGHT;
    shift_tetrimino(&nes);
    assert(nes.ram[ADDR_TETRIMINO_X] == 6);
    assert(nes.ram[ADDR_SOUND_EFFECT_SLOT1_INIT] == 0x03);

    /* Test 8: drop_tetrimino gravity & touchdown */
    nes.ram[ADDR_TETRIMINO_X] = 5;
    nes.ram[ADDR_TETRIMINO_Y] = 10;
    nes.ram[ADDR_LEVEL_NUMBER] = 0; /* 48 frames per drop */
    nes.ram[ADDR_FALL_TIMER] = 47;
    nes.ram[ADDR_HELD_BUTTONS] = 0;
    nes.ram[ADDR_NEWLY_PRESSED_BUTTONS] = 0;
    nes.ram[ADDR_AUTOREPEAT_Y] = 0;
    nes.ram[ADDR_PLAY_STATE] = PLAYSTATE_CONTROL_ACTIVE;

    /* Frame 47 < 48: piece does not drop */
    drop_tetrimino(&nes);
    assert(nes.ram[ADDR_TETRIMINO_Y] == 10);

    /* Advance timer to 48: piece drops to Y = 11 */
    nes.ram[ADDR_FALL_TIMER] = 48;
    drop_tetrimino(&nes);
    assert(nes.ram[ADDR_TETRIMINO_Y] == 11);
    assert(nes.ram[ADDR_PLAY_STATE] == PLAYSTATE_CONTROL_ACTIVE);

    /* Touchdown: placed block below piece at (row 12, col 5) */
    nes.ram[ADDR_PLAYFIELD + (12 * 10 + 5)] = TILE_2;
    nes.ram[ADDR_FALL_TIMER] = 48;
    drop_tetrimino(&nes);
    /* Cannot move to Y=12; locks at Y=11 and transitions to PLAYSTATE_CHECK_ROWS (2) */
    assert(nes.ram[ADDR_TETRIMINO_Y] == 11);
    assert(nes.ram[ADDR_PLAY_STATE] == PLAYSTATE_CHECK_ROWS);

    /* Test 9: getDropSpeed killscreen */
    assert(getDropSpeed(0) == 48);
    assert(getDropSpeed(18) == 3);
    assert(getDropSpeed(19) == 2);
    assert(getDropSpeed(28) == 2);
    assert(getDropSpeed(29) == 1);
    assert(getDropSpeed(35) == 1);

    printf("  [PASS] test_tetrimino_subsystem\n");
    return 0;
}
