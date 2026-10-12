#ifndef TDECOMP_CONSTANTS_MEMORY_H
#define TDECOMP_CONSTANTS_MEMORY_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Zero Page Variables ($0000 - $00FF) */
#define ADDR_TMP1                           0x0000
#define ADDR_TMP2                           0x0001
#define ADDR_TMP3                           0x0002
#define ADDR_TMP_BULK_COPY_TO_PPU_RET       0x0005
#define ADDR_PATCH_TO_PPU                   0x0014
#define ADDR_RNG_SEED                       0x0017 /* 2 bytes */
#define ADDR_SPAWN_ID                       0x0019
#define ADDR_SPAWN_COUNT                    0x001A
#define ADDR_VERTICAL_BLANK_INTERVAL        0x0033
#define ADDR_UNUSED_0E                      0x0034

/* Active Player Variables ($0040 - $005F) */
#define ADDR_TETRIMINO_X                    0x0040
#define ADDR_TETRIMINO_Y                    0x0041
#define ADDR_CURRENT_PIECE                  0x0042
#define ADDR_LEVEL_NUMBER                   0x0044
#define ADDR_FALL_TIMER                     0x0045
#define ADDR_AUTOREPEAT_X                   0x0046
#define ADDR_START_LEVEL                    0x0047
#define ADDR_PLAY_STATE                     0x0048
#define ADDR_VRAM_ROW                       0x0049
#define ADDR_COMPLETED_ROW                  0x004A /* 4 bytes: 0x4A..0x4D */
#define ADDR_AUTOREPEAT_Y                   0x004E
#define ADDR_HOLD_DOWN_POINTS               0x004F
#define ADDR_LINES                          0x0050 /* 2 bytes BCD */
#define ADDR_ROW_Y                          0x0052
#define ADDR_SCORE                          0x0053 /* 3 bytes BCD: low, mid, high */
#define ADDR_COMPLETED_LINES                0x0056
#define ADDR_LINE_INDEX                     0x0057
#define ADDR_CURTAIN_ROW                    0x0058
#define ADDR_START_HEIGHT                   0x0059
#define ADDR_GARBAGE_HOLE                   0x005A

/* Player 1 Stored Variables ($0060 - $007F) */
#define ADDR_P1_BASE                        0x0060
#define ADDR_P1_TETRIMINO_X                 0x0060
#define ADDR_P1_TETRIMINO_Y                 0x0061
#define ADDR_P1_CURRENT_PIECE               0x0062
#define ADDR_P1_LEVEL_NUMBER                0x0064
#define ADDR_P1_FALL_TIMER                  0x0065
#define ADDR_P1_AUTOREPEAT_X                0x0066
#define ADDR_P1_START_LEVEL                 0x0067
#define ADDR_P1_PLAY_STATE                  0x0068
#define ADDR_P1_VRAM_ROW                    0x0069
#define ADDR_P1_COMPLETED_ROW               0x006A
#define ADDR_P1_AUTOREPEAT_Y                0x006E
#define ADDR_P1_HOLD_DOWN_POINTS            0x006F
#define ADDR_P1_LINES                       0x0070
#define ADDR_P1_ROW_Y                       0x0072
#define ADDR_P1_SCORE                       0x0073
#define ADDR_P1_COMPLETED_LINES             0x0076
#define ADDR_P1_CURTAIN_ROW                 0x0078
#define ADDR_P1_START_HEIGHT                0x0079
#define ADDR_P1_GARBAGE_HOLE                0x007A

/* Player 2 Stored Variables ($0080 - $009F) */
#define ADDR_P2_BASE                        0x0080
#define ADDR_P2_TETRIMINO_X                 0x0080
#define ADDR_P2_TETRIMINO_Y                 0x0081
#define ADDR_P2_CURRENT_PIECE               0x0082
#define ADDR_P2_LEVEL_NUMBER                0x0084
#define ADDR_P2_FALL_TIMER                  0x0085
#define ADDR_P2_AUTOREPEAT_X                0x0086
#define ADDR_P2_START_LEVEL                 0x0087
#define ADDR_P2_PLAY_STATE                  0x0088
#define ADDR_P2_VRAM_ROW                    0x0089
#define ADDR_P2_COMPLETED_ROW               0x008A
#define ADDR_P2_AUTOREPEAT_Y                0x008E
#define ADDR_P2_HOLD_DOWN_POINTS            0x008F
#define ADDR_P2_LINES                       0x0090
#define ADDR_P2_ROW_Y                       0x0092
#define ADDR_P2_SCORE                       0x0093
#define ADDR_P2_COMPLETED_LINES             0x0096
#define ADDR_P2_CURTAIN_ROW                 0x0098
#define ADDR_P2_START_HEIGHT                0x0099
#define ADDR_P2_GARBAGE_HOLE                0x009A

/* System & Control Variables ($00A0 - $00DF) */
#define ADDR_SPRITE_X_OFFSET                0x00A0
#define ADDR_SPRITE_Y_OFFSET                0x00A1
#define ADDR_SPRITE_INDEX_OAM               0x00A2
#define ADDR_OUT_OF_DATE_RENDER_FLAGS       0x00A3
#define ADDR_GAME_MODE_STATE                0x00A7
#define ADDR_GENERAL_COUNTER                0x00A8
#define ADDR_GENERAL_COUNTER2               0x00A9
#define ADDR_GENERAL_COUNTER3               0x00AA
#define ADDR_GENERAL_COUNTER4               0x00AB
#define ADDR_GENERAL_COUNTER5               0x00AC
#define ADDR_SELECTING_LEVEL_OR_HEIGHT      0x00AD
#define ADDR_ORIGINAL_Y                     0x00AE
#define ADDR_DROP_SPEED                     0x00AF
#define ADDR_TMP_CURRENT_PIECE              0x00B0
#define ADDR_FRAME_COUNTER                  0x00B1 /* 2 bytes */
#define ADDR_OAM_STAGING_LENGTH             0x00B3
#define ADDR_NEWLY_PRESSED_BUTTONS          0x00B5
#define ADDR_HELD_BUTTONS                   0x00B6
#define ADDR_ACTIVE_PLAYER                  0x00B7
#define ADDR_PLAYFIELD_ADDR                 0x00B8 /* 2 bytes */
#define ADDR_ALLEGRO                        0x00BA
#define ADDR_PENDING_GARBAGE                0x00BB
#define ADDR_PENDING_GARBAGE_INACTIVE       0x00BC
#define ADDR_RENDER_MODE                    0x00BD
#define ADDR_NUMBER_OF_PLAYERS              0x00BE
#define ADDR_NEXT_PIECE                     0x00BF
#define ADDR_GAME_MODE                      0x00C0
#define ADDR_GAME_TYPE                      0x00C1
#define ADDR_MUSIC_TYPE                     0x00C2
#define ADDR_SLEEP_COUNTER                  0x00C3
#define ADDR_ENDING                         0x00C4
#define ADDR_ENDING_CUSTOM_VARS             0x00C5
#define ADDR_ENDING_CURRENT_SPRITE          0x00CC
#define ADDR_DEMO_HELD_BUTTONS              0x00CE
#define ADDR_DEMO_REPEATS                   0x00CF
#define ADDR_DEMO_RECORD_FLAG               0x00D0
#define ADDR_DEMO_BUTTONS_ADDR              0x00D1 /* 2 bytes */
#define ADDR_DEMO_INDEX                     0x00D3
#define ADDR_HIGH_SCORE_NAME_OFFSET_LETTER  0x00D4
#define ADDR_HIGH_SCORE_RAW_POS             0x00D5
#define ADDR_HIGH_SCORE_NAME_OFFSET_ROW     0x00D6
#define ADDR_HIGH_SCORE_CURRENT_LETTER      0x00D7
#define ADDR_LINE_CLEAR_STATS_BY_TYPE       0x00D8 /* 4 bytes */
#define ADDR_TOTAL_SCORE                    0x00DC /* 3 bytes */
#define ADDR_DISPLAY_NEXT_PIECE             0x00DF

/* Audio Zero Page Variables ($00E0 - $00EF) */
#define ADDR_AUDIOTMP1                      0x00E0
#define ADDR_AUDIOTMP2                      0x00E1
#define ADDR_AUDIOTMP3                      0x00E2
#define ADDR_AUDIOTMP4                      0x00E3
#define ADDR_AUDIOTMP5                      0x00E4
#define ADDR_MUSIC_CHAN_TMP_ADDR            0x00E6 /* 2 bytes */
#define ADDR_SOUND_RNG_SEED                 0x00EB /* 2 bytes */
#define ADDR_CURRENT_SFX_SLOT               0x00ED
#define ADDR_MUSIC_CHAN_OFFSET              0x00EE
#define ADDR_CURRENT_AUDIO_SLOT             0x00EF

/* Input Zero Page Variables ($00F0 - $00FF) */
#define ADDR_NEWLY_PRESSED_P1               0x00F5
#define ADDR_NEWLY_PRESSED_P2               0x00F6
#define ADDR_HELD_BUTTONS_P1                0x00F7
#define ADDR_HELD_BUTTONS_P2                0x00F8
#define ADDR_JOY1_LOCATION                  0x00FB
#define ADDR_PPU_SCROLL_Y                   0x00FC
#define ADDR_PPU_SCROLL_X                   0x00FD
#define ADDR_CURRENT_PPU_MASK               0x00FE
#define ADDR_CURRENT_PPU_CTRL               0x00FF

/* BSS Memory Regions ($0100 - $07FF) */
#define ADDR_STACK                          0x0100 /* 256 bytes */
#define ADDR_OAM_STAGING                    0x0200 /* 256 bytes */
#define ADDR_STATS_BY_TYPE                  0x03F0 /* 14 bytes: 7 pieces * 2 bytes BCD */
#define ADDR_PLAYFIELD                      0x0400 /* 200 bytes ($C8): 10 x 20 cells */
#define ADDR_PLAYFIELD_P2                   0x0500 /* 200 bytes */
#define ADDR_SOUND_EFFECT_SLOT0_INIT        0x06F0
#define ADDR_SOUND_EFFECT_SLOT1_INIT        0x06F1
#define ADDR_SOUND_EFFECT_SLOT2_INIT        0x06F2
#define ADDR_SOUND_EFFECT_SLOT3_INIT        0x06F3
#define ADDR_SOUND_EFFECT_SLOT4_INIT        0x06F4
#define ADDR_MUSIC_TRACK                    0x06F5
#define ADDR_CURRENTLY_PLAYING_MUSIC_TRACK  0x06FD
#define ADDR_UNREFERENCED_SOUND_RNG_TMP     0x06FF /* 1 byte */
#define ADDR_HIGH_SCORE_NAMES               0x0700 /* 48 bytes ($30): 3 chars * 8 entries * 2 types */
#define ADDR_HIGH_SCORE_A                   0x0730 /* 12 bytes ($0C) */
#define ADDR_HIGH_SCORE_B                   0x073C /* 12 bytes ($0C) */
#define ADDR_HIGH_SCORE_LEVELS              0x0748 /* 8 bytes ($08) */
#define ADDR_INIT_MAGIC                     0x0750 /* 5 bytes */

#ifdef __cplusplus
}
#endif

#endif /* TDECOMP_CONSTANTS_MEMORY_H */
