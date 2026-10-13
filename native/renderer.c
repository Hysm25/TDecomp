#include "renderer.h"
#include "game/render.h"
#include "game/tetrimino.h"
#include "constants/gameplay.h"
#include <raylib.h>
#include <stdio.h>
#include <math.h>

#define WINDOW_WIDTH    800
#define WINDOW_HEIGHT   720
#define MATRIX_X        330
#define MATRIX_Y        70
#define CELL_SIZE       28
#define MATRIX_W        (PLAYFIELD_WIDTH * CELL_SIZE)   /* 280 */
#define MATRIX_H        (PLAYFIELD_HEIGHT * CELL_SIZE)  /* 560 */

/* Authoritative 64-color NES 2C02 RGB Palette */
static const Color NES_PALETTE[64] = {
    {0x57, 0x57, 0x57, 255}, {0x00, 0x16, 0x68, 255}, {0x07, 0x06, 0x7B, 255}, {0x2A, 0x00, 0x75, 255},
    {0x48, 0x00, 0x59, 255}, {0x4E, 0x00, 0x21, 255}, {0x4C, 0x00, 0x00, 255}, {0x38, 0x11, 0x00, 255},
    {0x20, 0x29, 0x00, 255}, {0x00, 0x32, 0x00, 255}, {0x00, 0x38, 0x00, 255}, {0x00, 0x34, 0x06, 255},
    {0x00, 0x2D, 0x41, 255}, {0x00, 0x00, 0x00, 255}, {0x00, 0x00, 0x00, 255}, {0x00, 0x00, 0x00, 255},

    {0xA5, 0xA5, 0xA5, 255}, {0x18, 0x4C, 0xBB, 255}, {0x38, 0x37, 0xD6, 255}, {0x68, 0x1F, 0xCE, 255},
    {0x8E, 0x1C, 0xA6, 255}, {0x99, 0x1D, 0x5C, 255}, {0x96, 0x2A, 0x1E, 255}, {0x7A, 0x46, 0x00, 255},
    {0x59, 0x64, 0x00, 255}, {0x2A, 0x73, 0x00, 255}, {0x0E, 0x7A, 0x00, 255}, {0x00, 0x75, 0x36, 255},
    {0x03, 0x69, 0x85, 255}, {0x00, 0x00, 0x00, 255}, {0x00, 0x00, 0x00, 255}, {0x00, 0x00, 0x00, 255},

    {0xFF, 0xFF, 0xFF, 255}, {0x6D, 0xA3, 0xFF, 255}, {0x8E, 0x8D, 0xFF, 255}, {0xC0, 0x75, 0xFF, 255},
    {0xE7, 0x71, 0xFF, 255}, {0xF3, 0x72, 0xB3, 255}, {0xF0, 0x80, 0x73, 255}, {0xD3, 0x9D, 0x29, 255},
    {0xB0, 0xBC, 0x11, 255}, {0x80, 0xCB, 0x16, 255}, {0x62, 0xD3, 0x3B, 255}, {0x4D, 0xCE, 0x8C, 255},
    {0x57, 0xC1, 0xDE, 255}, {0x41, 0x41, 0x41, 255}, {0x00, 0x00, 0x00, 255}, {0x00, 0x00, 0x00, 255},

    {0xFF, 0xFF, 0xFF, 255}, {0xC4, 0xDA, 0xFF, 255}, {0xD2, 0xD1, 0xFF, 255}, {0xE6, 0xC7, 0xFF, 255},
    {0xF8, 0xC9, 0xFF, 255}, {0xFA, 0xC6, 0xE1, 255}, {0xF9, 0xCC, 0xC7, 255}, {0xED, 0xD8, 0xA9, 255},
    {0xE2, 0xE7, 0xA2, 255}, {0xCC, 0xEA, 0xA1, 255}, {0xC0, 0xED, 0xB0, 255}, {0xB7, 0xEB, 0xD1, 255},
    {0xBF, 0xE9, 0xF5, 255}, {0xB3, 0xB3, 0xB3, 255}, {0x00, 0x00, 0x00, 255}, {0x00, 0x00, 0x00, 255}
};

static Color get_nes_color(uint8_t index) {
    return NES_PALETTE[index & 0x3F];
}

void renderer_init(void) {
    /* No dynamic font allocation required; Raylib default font handles HUD cleanly */
}

void renderer_shutdown(void) {
}

/* Draws an authentic NES-style beveled tetromino block cell */
static void draw_block(int px, int py, int size, uint8_t tile_id, uint8_t level) {
    uint8_t lvl_mod = (uint8_t)(level % 10);
    Color primary   = get_nes_color(colorTable[lvl_mod * 4 + 2]);
    Color secondary = get_nes_color(colorTable[lvl_mod * 4 + 3]);
    Color white     = (Color){255, 255, 255, 255};
    Color dark      = (Color){15, 15, 20, 255};

    if (tile_id == TILE_CURTAIN) {
        /* Curtain tile during game over animation */
        DrawRectangle(px, py, size, size, (Color){180, 180, 180, 255});
        DrawRectangle(px + 2, py + 2, size - 4, size - 4, (Color){100, 100, 100, 255});
        DrawRectangle(px + 4, py + 4, size - 8, size - 8, (Color){220, 220, 220, 255});
        return;
    }

    if (tile_id == TILE_1) {
        /* Tile 1: Solid primary color with white shine corner */
        DrawRectangle(px, py, size, size, primary);
        /* Darker shadow edge */
        DrawRectangle(px + size - 2, py, 2, size, dark);
        DrawRectangle(px, py + size - 2, size, 2, dark);
        /* Bevel corner */
        DrawRectangle(px + 2, py + 2, size - 4, size - 4, primary);
        /* White shine block */
        DrawRectangle(px + 3, py + 3, 5, 5, white);
    } else if (tile_id == TILE_2) {
        /* Tile 2: Secondary color with checkered / inner primary center */
        DrawRectangle(px, py, size, size, secondary);
        DrawRectangle(px + size - 2, py, 2, size, dark);
        DrawRectangle(px, py + size - 2, size, 2, dark);
        /* Inner primary box */
        DrawRectangle(px + 4, py + 4, size - 8, size - 8, primary);
        DrawRectangle(px + 6, py + 6, size - 12, size - 12, white);
    } else {
        /* Tile 3: White base with primary accent */
        DrawRectangle(px, py, size, size, white);
        DrawRectangle(px + size - 2, py, 2, size, dark);
        DrawRectangle(px, py + size - 2, size, 2, dark);
        DrawRectangle(px + 3, py + 3, size - 6, size - 6, primary);
        DrawRectangle(px + 6, py + 6, size - 12, size - 12, white);
    }
}

/* Draws a retro panel box with bevel border */
static void draw_retro_panel(int x, int y, int w, int h, const char *title) {
    DrawRectangle(x, y, w, h, (Color){18, 20, 28, 255});
    DrawRectangleLinesEx((Rectangle){(float)x, (float)y, (float)w, (float)h}, 2.0f, (Color){50, 60, 85, 255});
    DrawRectangleLinesEx((Rectangle){(float)(x + 3), (float)(y + 3), (float)(w - 6), (float)(h - 6)}, 1.0f, (Color){35, 40, 58, 255});

    if (title && title[0] != '\0') {
        int tw = MeasureText(title, 14);
        int tx = x + (w - tw) / 2;
        DrawText(title, tx, y + 8, 14, (Color){180, 195, 220, 255});
    }
}

static void draw_hud(const GameAdapter *adapter) {
    char buf[64];

    /* Title header */
    DrawText("TDecomp", 40, 20, 28, (Color){255, 80, 80, 255});
    DrawText("NES TETRIS", 180, 26, 20, (Color){255, 220, 100, 255});

    /* TOP SCORE */
    draw_retro_panel(40, 70, 230, 65, "TOP SCORE");
    snprintf(buf, sizeof(buf), "%06u", (unsigned int)adapter->high_score);
    int tw = MeasureText(buf, 24);
    DrawText(buf, 40 + (230 - tw) / 2, 70 + 30, 24, (Color){255, 255, 255, 255});

    /* CURRENT SCORE */
    draw_retro_panel(40, 145, 230, 65, "SCORE");
    snprintf(buf, sizeof(buf), "%06u", (unsigned int)adapter->score);
    tw = MeasureText(buf, 24);
    DrawText(buf, 40 + (230 - tw) / 2, 145 + 30, 24, (Color){255, 255, 255, 255});

    /* LINES */
    draw_retro_panel(40, 220, 230, 65, "LINES");
    snprintf(buf, sizeof(buf), "%03u", (unsigned int)adapter->lines);
    tw = MeasureText(buf, 24);
    DrawText(buf, 40 + (230 - tw) / 2, 220 + 30, 24, (Color){100, 230, 100, 255});

    /* LEVEL */
    draw_retro_panel(40, 295, 230, 65, "LEVEL");
    snprintf(buf, sizeof(buf), "%02u", (unsigned int)adapter->level);
    tw = MeasureText(buf, 24);
    DrawText(buf, 40 + (230 - tw) / 2, 295 + 30, 24, (Color){255, 200, 60, 255});

    /* STATISTICS */
    draw_retro_panel(40, 370, 230, 260, "STATISTICS");
    static const char *piece_names[7] = {"T", "J", "Z", "O", "S", "L", "I"};
    static const uint8_t sample_orientations[7] = {
        ORIENT_T_UP, ORIENT_J_LEFT, ORIENT_Z_VERT, ORIENT_O_FIXED, ORIENT_S_VERT, ORIENT_L_RIGHT, ORIENT_I_HORIZ
    };

    for (int p = 0; p < 7; p++) {
        int py = 405 + p * 30;
        DrawText(piece_names[p], 55, py, 16, (Color){255, 90, 90, 255});

        /* Miniature piece preview */
        uint8_t o = sample_orientations[p];
        for (int m = 0; m < 4; m++) {
            int8_t my = (int8_t)orientationTable[o * 12 + m * 3 + 0];
            int8_t mx = (int8_t)orientationTable[o * 12 + m * 3 + 2];
            int bx = 110 + mx * 8;
            int by = py + 7 + my * 8;
            DrawRectangle(bx, by, 7, 7, (Color){200, 180, 240, 255});
        }

        snprintf(buf, sizeof(buf), "%03u", (unsigned int)adapter->stats[p]);
        DrawText(buf, 190, py, 16, (Color){255, 255, 255, 255});
    }

    /* NEXT BOX (Right panel) */
    draw_retro_panel(640, 70, 130, 110, "NEXT");
    if (adapter->show_next_piece && adapter->next_orientation < ORIENTATION_COUNT) {
        uint8_t no = adapter->next_orientation;
        for (int m = 0; m < 4; m++) {
            int8_t my = (int8_t)orientationTable[no * 12 + m * 3 + 0];
            uint8_t mt = orientationTable[no * 12 + m * 3 + 1];
            int8_t mx = (int8_t)orientationTable[no * 12 + m * 3 + 2];
            int bx = 640 + 65 + mx * 18 - 9;
            int by = 70 + 60 + my * 18 - 9;
            draw_block(bx, by, 17, mt, adapter->level);
        }
    }

    /* CONTROLS GUIDE */
    draw_retro_panel(630, 200, 150, 430, "CONTROLS");
    int cy = 230;
    DrawText("Arrows / WASD", 640, cy, 11, (Color){255, 200, 80, 255});
    DrawText("Move & Drop", 640, cy + 13, 11, (Color){200, 200, 200, 255});
    cy += 38;
    DrawText("Z / J", 640, cy, 11, (Color){255, 200, 80, 255});
    DrawText("Rotate CCW", 640, cy + 13, 11, (Color){200, 200, 200, 255});
    cy += 38;
    DrawText("X / K / Up", 640, cy, 11, (Color){255, 200, 80, 255});
    DrawText("Rotate CW", 640, cy + 13, 11, (Color){200, 200, 200, 255});
    cy += 38;
    DrawText("Enter / Space", 640, cy, 11, (Color){255, 200, 80, 255});
    DrawText("Pause / Select", 640, cy + 13, 11, (Color){200, 200, 200, 255});
    cy += 38;
    DrawText("Tab / Shift", 640, cy, 11, (Color){255, 200, 80, 255});
    DrawText("Toggle Next", 640, cy + 13, 11, (Color){200, 200, 200, 255});
    cy += 38;
    DrawText("R", 640, cy, 11, (Color){255, 200, 80, 255});
    DrawText("Quick Restart", 640, cy + 13, 11, (Color){200, 200, 200, 255});
    cy += 38;
    DrawText("M", 640, cy, 11, (Color){255, 200, 80, 255});
    DrawText("Mute Audio", 640, cy + 13, 11, (Color){200, 200, 200, 255});
    cy += 38;
    DrawText("Esc", 640, cy, 11, (Color){255, 200, 80, 255});
    DrawText("Quit Game", 640, cy + 13, 11, (Color){200, 200, 200, 255});
}

static void draw_playfield(const GameAdapter *adapter) {
    /* Outer border frame */
    DrawRectangle(MATRIX_X - 6, MATRIX_Y - 6, MATRIX_W + 12, MATRIX_H + 12, (Color){40, 50, 75, 255});
    DrawRectangle(MATRIX_X - 3, MATRIX_Y - 3, MATRIX_W + 6, MATRIX_H + 6, (Color){15, 20, 32, 255});

    /* Playfield background */
    DrawRectangle(MATRIX_X, MATRIX_Y, MATRIX_W, MATRIX_H, (Color){8, 10, 16, 255});

    /* Subtle grid lines */
    for (int c = 1; c < PLAYFIELD_WIDTH; c++) {
        DrawLine(MATRIX_X + c * CELL_SIZE, MATRIX_Y, MATRIX_X + c * CELL_SIZE, MATRIX_Y + MATRIX_H, (Color){18, 22, 34, 255});
    }
    for (int r = 1; r < PLAYFIELD_HEIGHT; r++) {
        DrawLine(MATRIX_X, MATRIX_Y + r * CELL_SIZE, MATRIX_X + MATRIX_W, MATRIX_Y + r * CELL_SIZE, (Color){18, 22, 34, 255});
    }

    /* Locked mino cells on the playfield */
    for (int r = 0; r < PLAYFIELD_HEIGHT; r++) {
        for (int c = 0; c < PLAYFIELD_WIDTH; c++) {
            if (adapter->playfield[r][c].occupied) {
                int px = MATRIX_X + c * CELL_SIZE;
                int py = MATRIX_Y + r * CELL_SIZE;
                draw_block(px, py, CELL_SIZE, adapter->playfield[r][c].tile_id, adapter->level);
            }
        }
    }

    /* Active falling tetromino */
    for (int m = 0; m < 4; m++) {
        if (adapter->active_minos[m].visible) {
            int mx = adapter->active_minos[m].x;
            int my = adapter->active_minos[m].y;
            if (mx >= 0 && mx < PLAYFIELD_WIDTH && my >= 0 && my < PLAYFIELD_HEIGHT) {
                int px = MATRIX_X + mx * CELL_SIZE;
                int py = MATRIX_Y + my * CELL_SIZE;
                draw_block(px, py, CELL_SIZE, adapter->active_minos[m].tile_id, adapter->level);
            }
        }
    }

    /* Line clear flash animation */
    if (adapter->line_clear_anim_active) {
        float t = (float)GetTime();
        if ((int)(t * 20.0f) % 2 == 0) {
            /* Flash bright overlay on cleared rows */
            DrawRectangle(MATRIX_X, MATRIX_Y, MATRIX_W, MATRIX_H, (Color){255, 255, 255, 40});
        }
    }
}

static void draw_title_screen(const GameAdapter *adapter) {
    (void)adapter;
    DrawRectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, (Color){10, 12, 18, 255});

    DrawText("TDecomp", WINDOW_WIDTH / 2 - 120, 140, 56, (Color){255, 60, 60, 255});
    DrawText("NES TETRIS", WINDOW_WIDTH / 2 - 95, 210, 32, (Color){255, 220, 100, 255});
    DrawText("PORTABLE C11 DECOMPILATION", WINDOW_WIDTH / 2 - 130, 260, 16, (Color){150, 180, 220, 255});

    float t = (float)GetTime();
    if ((int)(t * 2.5f) % 2 == 0) {
        DrawText("PRESS ENTER TO START", WINDOW_WIDTH / 2 - 140, 420, 22, (Color){255, 255, 255, 255});
    }

    DrawText("Arrow Keys: Navigate   |   Z/X: Rotate   |   Esc: Exit", WINDOW_WIDTH / 2 - 180, 540, 15, (Color){160, 170, 190, 255});
    DrawText("Original research: TetrisNESDisasm, taus, nes-util", WINDOW_WIDTH / 2 - 180, 640, 13, (Color){100, 110, 130, 255});
}

static void draw_level_select_screen(const GameAdapter *adapter) {
    DrawRectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, (Color){12, 14, 22, 255});

    DrawText("SELECT STARTING LEVEL", WINDOW_WIDTH / 2 - 170, 120, 26, (Color){255, 220, 100, 255});

    /* Level grid: row 1 (0..9), row 2 (10..19) */
    char buf[16];
    for (int lvl = 0; lvl < 20; lvl++) {
        int row = lvl / 10;
        int col = lvl % 10;
        int bx = 110 + col * 58;
        int by = 220 + row * 80;
        int bw = 48;
        int bh = 48;

        bool selected = (lvl == adapter->selected_level);
        Color border_col = selected ? (Color){255, 220, 80, 255} : (Color){50, 60, 90, 255};
        Color fill_col   = selected ? (Color){60, 70, 110, 255} : (Color){20, 24, 38, 255};

        DrawRectangle(bx, by, bw, bh, fill_col);
        DrawRectangleLinesEx((Rectangle){(float)bx, (float)by, (float)bw, (float)bh}, selected ? 3.0f : 1.5f, border_col);

        snprintf(buf, sizeof(buf), "%d", lvl);
        int tw = MeasureText(buf, 20);
        DrawText(buf, bx + (bw - tw) / 2, by + 14, 20, selected ? (Color){255, 255, 255, 255} : (Color){170, 180, 200, 255});
    }

    float t = (float)GetTime();
    if ((int)(t * 2.5f) % 2 == 0) {
        DrawText("PRESS ENTER TO PLAY", WINDOW_WIDTH / 2 - 130, 460, 22, (Color){100, 240, 120, 255});
    }

    DrawText("Left / Right / Up / Down : Select Level", WINDOW_WIDTH / 2 - 145, 540, 16, (Color){180, 190, 210, 255});
    DrawText("Esc : Back to Title", WINDOW_WIDTH / 2 - 70, 580, 15, (Color){140, 150, 170, 255});
}

static void draw_overlay(const GameAdapter *adapter) {
    if (adapter->screen == SCREEN_PAUSED) {
        DrawRectangle(MATRIX_X, MATRIX_Y, MATRIX_W, MATRIX_H, (Color){0, 0, 0, 160});
        float t = (float)GetTime();
        if ((int)(t * 2.0f) % 2 == 0) {
            DrawText("PAUSE", MATRIX_X + 85, MATRIX_Y + 250, 32, (Color){255, 220, 80, 255});
        }
    } else if (adapter->screen == SCREEN_GAME_OVER) {
        DrawRectangle(MATRIX_X, MATRIX_Y + 180, MATRIX_W, 180, (Color){15, 18, 28, 230});
        DrawRectangleLinesEx((Rectangle){(float)MATRIX_X, (float)(MATRIX_Y + 180), (float)MATRIX_W, 180.0f}, 2.0f, (Color){255, 80, 80, 255});

        DrawText("GAME OVER", MATRIX_X + 50, MATRIX_Y + 210, 30, (Color){255, 70, 70, 255});

        char buf[32];
        snprintf(buf, sizeof(buf), "SCORE: %06u", (unsigned int)adapter->score);
        int tw = MeasureText(buf, 18);
        DrawText(buf, MATRIX_X + (MATRIX_W - tw) / 2, MATRIX_Y + 260, 18, (Color){255, 255, 255, 255});

        float t = (float)GetTime();
        if ((int)(t * 2.5f) % 2 == 0) {
            DrawText("PRESS ENTER TO RETRY", MATRIX_X + 25, MATRIX_Y + 310, 18, (Color){255, 220, 80, 255});
        }
    }
}

void renderer_draw(const GameAdapter *adapter) {
    if (!adapter) return;

    BeginDrawing();
    ClearBackground((Color){14, 16, 24, 255});

    if (adapter->screen == SCREEN_TITLE) {
        draw_title_screen(adapter);
    } else if (adapter->screen == SCREEN_LEVEL_SELECT) {
        draw_level_select_screen(adapter);
    } else {
        draw_hud(adapter);
        draw_playfield(adapter);
        draw_overlay(adapter);
    }

    EndDrawing();
}
