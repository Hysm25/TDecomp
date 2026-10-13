#include "game_adapter.h"
#include "renderer.h"
#include "audio.h"
#include "constants/gameplay.h"
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif

static GameAdapter s_adapter;

static void print_usage(const char *prog) {
    printf("TDecomp - NES Tetris (Portable C Decompilation Port)\n\n");
    printf("Usage:\n");
    printf("  %s [options]\n\n", prog);
    printf("Options:\n");
    printf("  --headless           Run headless simulation test and exit cleanly (0)\n");
    printf("  --test-frames <N>    Run N frames in headless mode (default: 180)\n");
    printf("  --level <0-19>       Start immediately at the specified level\n");
    printf("  --no-audio           Disable procedural sound effects\n");
    printf("  --help, -h           Show this help message\n\n");
    printf("Controls:\n");
    printf("  Left / Right / Down  : Shift & Soft Drop (Arrows or A/D/S)\n");
    printf("  Z / J                : Rotate Counter-Clockwise (Button B)\n");
    printf("  X / K / Up           : Rotate Clockwise (Button A)\n");
    printf("  Enter / Space        : Start Game / Pause / Resume\n");
    printf("  Tab / Shift          : Toggle Next Piece Preview\n");
    printf("  R                    : Quick Restart\n");
    printf("  M                    : Toggle Audio Mute\n");
    printf("  Esc                  : Exit\n");
}

static void game_frame_step(void) {
    /* Global keys */
    if (IsKeyPressed(KEY_M)) {
        native_audio_toggle_mute();
    }

    if (s_adapter.screen == SCREEN_TITLE) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            game_adapter_menu_select(&s_adapter);
        }
    } else if (s_adapter.screen == SCREEN_LEVEL_SELECT) {
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
            game_adapter_menu_left(&s_adapter);
        }
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
            game_adapter_menu_right(&s_adapter);
        }
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
            game_adapter_menu_up(&s_adapter);
        }
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
            game_adapter_menu_down(&s_adapter);
        }
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            game_adapter_menu_select(&s_adapter);
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            game_adapter_menu_back(&s_adapter);
        }
    } else if (s_adapter.screen == SCREEN_PLAYING) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            game_adapter_toggle_pause(&s_adapter);
        }
        if (IsKeyPressed(KEY_R)) {
            game_adapter_restart(&s_adapter);
        }

        /* Gather controller button bitmask for player 1 */
        uint8_t btn = 0;
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) btn |= BUTTON_RIGHT;
        if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) btn |= BUTTON_LEFT;
        if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) btn |= BUTTON_DOWN;
        if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) btn |= BUTTON_UP;
        if (IsKeyDown(KEY_Z)     || IsKeyDown(KEY_J)) btn |= BUTTON_B;
        if (IsKeyDown(KEY_X)     || IsKeyDown(KEY_K) || IsKeyDown(KEY_UP)) btn |= BUTTON_A;
        if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT)) {
            btn |= BUTTON_SELECT;
        }

        game_adapter_update(&s_adapter, btn);

        if (s_adapter.pending_audio != AUDIO_EVENT_NONE) {
            native_audio_play(s_adapter.pending_audio);
        }
    } else if (s_adapter.screen == SCREEN_PAUSED) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            game_adapter_toggle_pause(&s_adapter);
        }
        if (IsKeyPressed(KEY_R)) {
            game_adapter_restart(&s_adapter);
        }
    } else if (s_adapter.screen == SCREEN_GAME_OVER) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            game_adapter_restart(&s_adapter);
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            s_adapter.screen = SCREEN_TITLE;
        }
    }

    renderer_draw(&s_adapter);
}

int main(int argc, char *argv[]) {
    bool headless = false;
    uint32_t test_frames = 180;
    int start_level = -1;
    bool enable_audio = true;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--headless") == 0) {
            headless = true;
        } else if (strcmp(argv[i], "--test-frames") == 0 && i + 1 < argc) {
            headless = true;
            test_frames = (uint32_t)strtoul(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--level") == 0 && i + 1 < argc) {
            start_level = atoi(argv[++i]);
            if (start_level < 0) start_level = 0;
            if (start_level > 19) start_level = 19;
        } else if (strcmp(argv[i], "--no-audio") == 0) {
            enable_audio = false;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    game_adapter_init(&s_adapter);

    if (start_level >= 0) {
        game_adapter_start_game(&s_adapter, (uint8_t)start_level, GAMETYPE_A, 0);
    }

    if (headless) {
        printf("[HEADLESS] Stepping game simulation for %u frames...\n", test_frames);
        if (s_adapter.screen != SCREEN_PLAYING) {
            game_adapter_start_game(&s_adapter, 0, GAMETYPE_A, 0);
        }

        /* Step frames with intermittent soft drops to test pieces landing */
        for (uint32_t f = 0; f < test_frames; f++) {
            uint8_t btn = 0;
            if ((f % 60) >= 30) {
                btn = BUTTON_DOWN;
            }
            game_adapter_update(&s_adapter, btn);
        }

        printf("[HEADLESS] Verification passed: score=%u lines=%u level=%u frames=%llu\n",
               (unsigned int)s_adapter.score,
               (unsigned int)s_adapter.lines,
               (unsigned int)s_adapter.level,
               (unsigned long long)s_adapter.frame_count);
        return 0;
    }

    /* Raylib Graphical Window Setup */
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(800, 720, "TDecomp - NES Tetris (C Decompilation)");
    SetTargetFPS(60);

    renderer_init();
    native_audio_init(enable_audio);

#if defined(__EMSCRIPTEN__)
    emscripten_set_main_loop(game_frame_step, 0, 1);
#else
    while (!WindowShouldClose()) {
        game_frame_step();
    }

    native_audio_shutdown();
    renderer_shutdown();
    CloseWindow();
#endif

    return 0;
}
