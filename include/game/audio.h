#ifndef TDECOMP_GAME_AUDIO_H
#define TDECOMP_GAME_AUDIO_H

#include "common.h"
#include "nes.h"
#include "constants/hardware.h"
#include "constants/memory.h"
#include "constants/gameplay.h"

#ifdef __cplusplus
extern "C" {
#endif

/* SFX Initialization constants for NTSC (USA) */
#ifndef SFX_LEVELUP_INIT
#define SFX_LEVELUP_INIT        0x06
#endif
#ifndef SFX_LINE_COMPLETE_INIT
#define SFX_LINE_COMPLETE_INIT  0x05
#endif
#ifndef SFX_LINECLEAR_INIT
#define SFX_LINECLEAR_INIT      0x04
#endif
#ifndef SFX_TETRIS_INIT
#define SFX_TETRIS_INIT         0x05
#endif

/* Authoritative ROM Data Tables in PRG chunk 2 */
extern const uint8_t soundEffectSlot0_gameOverCurtainInitData[4];  /* $E104 */
extern const uint8_t soundEffectSlot0_endingRocketInitData[4];    /* $E108 */
extern const uint8_t music_pause_sq1_even[4];                     /* $E10C */
extern const uint8_t music_pause_sq1_odd[4];                      /* $E110 */
extern const uint8_t soundEffectSlot1_rotateTetriminoInitData[4]; /* $E114 */
extern const uint8_t soundEffectSlot1Playing_rotateTetriminoStage3[4]; /* $E118 */
extern const uint8_t soundEffectSlot1_levelUpInitData[4];         /* $E11C */
extern const uint8_t soundEffectSlot1_lockTetriminoInitData[4];   /* $E120 */
extern const uint8_t soundEffectSlot1_menuOptionSelectInitData[4]; /* $E124 */
extern const uint8_t soundEffectSlot1Playing_menuOptionSelectStage2[4]; /* $E128 */
extern const uint8_t soundEffectSlot1_menuScreenSelectInitData[4]; /* $E12C */
extern const uint8_t soundEffectSlot1_tetrisAchievedInitData[4];  /* $E130 */
extern const uint8_t soundEffectSlot1_lineCompletedInitData[4];   /* $E134 */
extern const uint8_t soundEffectSlot1_lineClearingInitData[4];    /* $E138 */
extern const uint8_t soundEffectSlot1_chirpChirpInitData[4];      /* $E13C */
extern const uint8_t soundEffectSlot1Playing_chirpChirpStage2[4]; /* $E140 */
extern const uint8_t soundEffectSlot1_shiftTetriminoInitData[4];  /* $E144 */
extern const uint8_t soundEffectSlot3_fallingAlienInitData[4];    /* $E148 */
extern const uint8_t soundEffectSlot3_donkInitData[4];            /* $E14C */
extern const uint8_t soundEffectSlot1_chirpChirpSq1Vol_table[4];  /* $E150 */
extern const uint8_t noiselo_table[32];                           /* $E154 */
extern const uint8_t noisevol_table[32];                          /* $E174 */
extern const uint8_t soundEffectSlot1_lineClearing_vol[9];        /* $E4B0 */
extern const uint8_t soundEffectSlot1_lineClearing_lo[24];        /* $E4B9 */
extern const uint8_t soundEffectSlot1_levelUp_lo[24];             /* $E4F3 */
extern const uint8_t trilo_table[4];                              /* $E57C */

/* 16-bit address dispatch tables in PRG chunk 2 */
extern const uint16_t soundEffectSlot0Init_table[3];              /* $E009 */
extern const uint16_t soundEffectSlot0Playing_table[3];           /* $E00F */
extern const uint16_t soundEffectSlot1Init_table[10];             /* $E015 */
extern const uint16_t soundEffectSlot1Playing_table[10];          /* $E029 */
extern const uint16_t soundEffectSlot3Init_table[2];              /* $E03D */
extern const uint16_t soundEffectSlot3Playing_table[2];           /* $E041 */
extern const uint16_t soundEffectSlot2Init_table[3];              /* $E045 */

/* APU Channel Output Transfer Routines */
void copyToSq1Channel(NES_State *nes, uint8_t src_y);             /* $E04B */
void copyToTriChannel(NES_State *nes, uint8_t src_y);             /* $E04F */
void copyToNoiseChannel(NES_State *nes, uint8_t src_y);           /* $E053 */
void copyToSq2Channel(NES_State *nes, uint8_t src_y);             /* $E057 */
void copyToApuChannel(NES_State *nes, uint8_t apu_offset, uint8_t src_y); /* $E059 */

/* Dispatch Helpers */
uint16_t computeSoundEffMethod(NES_State *nes, uint8_t index);    /* $E072 */
bool advanceAudioSlotFrame(NES_State *nes);                       /* $E0B5 (returns true if counter reached count) */

/* Sound Effect Slot Dispatchers */
void updateSoundEffectSlot2(NES_State *nes);                      /* $E194 */
void updateSoundEffectSlot3(NES_State *nes);                      /* $E19C */
void updateSoundEffectSlot1(NES_State *nes);                      /* $E1AC */
void updateSoundEffectSlot0(NES_State *nes);                      /* $E1B9 */
void updateSoundEffectSlotShared(NES_State *nes, uint8_t slot, uint8_t init_table_lo, uint8_t play_table_lo); /* $E1BF */

/* Audio Master Routines */
void updateAudio_jmp(NES_State *nes);                             /* $E000 */
void updateAudio2(NES_State *nes);                                /* $E003 */
void LE006(NES_State *nes);                                       /* $E006 */
void LE1D8(NES_State *nes);                                       /* $E1D8 */
void initAudioAndMarkInited(NES_State *nes);                      /* $E1E5 */
void updateAudio_pause(NES_State *nes);                           /* $E1EF */
void updateAudio(NES_State *nes);                                 /* $E216 */

/* Mute & Channel Inhibit Routines */
void soundEffectSlot2_makesNoSound(NES_State *nes);               /* $E244 */
void muteAudioAndClearTriControl(NES_State *nes);                 /* $E247 */
void LE253(NES_State *nes);                                       /* $E253 */
void LE265(NES_State *nes);                                       /* $E265 */
void muteAudio(NES_State *nes);                                   /* $E271 */

/* Sound Effect Slot Initialization */
void initSoundEffectShared(NES_State *nes, uint8_t frame_count, uint8_t init_data_y); /* $E287 */

/* Slot 0 (Noise Channel) Routines */
void soundEffectSlot0_endingRocketInit(NES_State *nes);           /* $E2CC */
void setNoiseLo(NES_State *nes, uint8_t lo);                      /* $E2D3 */
void loadNoiseLo(NES_State *nes);                                 /* $E2D7 */
void soundEffectSlot0_makesNoSound(NES_State *nes);               /* $E2DD */
void advanceSoundEffectSlot0WithoutUpdate(NES_State *nes);        /* $E2E4 */
void stopSoundEffectSlot0(NES_State *nes);                        /* $E2E9 */
void soundEffectSlot0_gameOverCurtainInit(NES_State *nes);        /* $E2F8 */
void updateSoundEffectSlot0_apu(NES_State *nes);                  /* $E2FF */
void updateSoundEffectNoiseAudio(NES_State *nes);                 /* $E307 */
uint8_t getSoundEffectNoiseNibble(NES_State *nes, uint8_t table_lo); /* $E31A */

/* Slot 1 (Pulse 1 Channel) Routines */
bool LE33B(NES_State *nes);                                       /* $E33B (returns true if high-priority SFX is active) */
void soundEffectSlot1_chirpChirpPlaying(NES_State *nes);          /* $E34F */
void soundEffectSlot1_chirpChirpInit(NES_State *nes);             /* $E37F */
void soundEffectSlot1_lockTetriminoInit(NES_State *nes);           /* $E384 */
void soundEffectSlot1_shiftTetriminoInit(NES_State *nes);          /* $E390 */
void soundEffectSlot1Playing_advance(NES_State *nes);             /* $E39C */
void soundEffectSlot1Playing_stop(NES_State *nes);                /* $E3A1 */
void soundEffectSlot1_menuOptionSelectPlaying(NES_State *nes);    /* $E3B3 */
void soundEffectSlot1_menuOptionSelectInit(NES_State *nes);       /* $E3CA */
void soundEffectSlot1_rotateTetriminoInit(NES_State *nes);        /* $E3D1 */
void soundEffectSlot1_rotateTetriminoPlaying(NES_State *nes);     /* $E3DD */
void soundEffectSlot1_tetrisAchievedInit(NES_State *nes);         /* $E403 */
void soundEffectSlot1_tetrisAchievedPlaying(NES_State *nes);      /* $E40E */
void LE417(NES_State *nes, uint8_t frame_count, uint8_t data_y);  /* $E417 */
void soundEffectSlot1_lineCompletedInit(NES_State *nes);          /* $E41A */
void soundEffectSlot1_lineCompletedPlaying(NES_State *nes);       /* $E425 */
void soundEffectSlot1_lineClearingInit(NES_State *nes);           /* $E42E */
void soundEffectSlot1_lineClearingPlaying(NES_State *nes);        /* $E43B */
void LE442(NES_State *nes, uint8_t init_data_y);                  /* $E442 */
void LE46C(NES_State *nes);                                       /* $E46C */
void soundEffectSlot1_menuScreenSelectInit(NES_State *nes);       /* $E473 */
void soundEffectSlot1_menuScreenSelectPlaying(NES_State *nes);    /* $E481 */
void LE493(NES_State *nes);                                       /* $E493 */
void LE4AC(NES_State *nes);                                       /* $E4AC */
void soundEffectSlot1_levelUpPlaying(NES_State *nes);             /* $E4D1 */
void LE4E9(NES_State *nes);                                       /* $E4E9 */
void soundEffectSlot1_levelUpInit(NES_State *nes);                /* $E4EC */

/* Slot 2 (DMC Channel) Routines */
void soundEffectSlot2_mediumBuzz(NES_State *nes);                 /* $E50B */
void soundEffectSlot2_lowBuzz(NES_State *nes);                    /* $E513 */
void LE51B(NES_State *nes, uint8_t len, uint8_t start, uint8_t freq); /* $E51B */

/* Slot 3 (Triangle Channel) Routines */
void soundEffectSlot3_donk(NES_State *nes);                       /* $E534 */
void soundEffectSlot3Playing_advance(NES_State *nes);             /* $E53B */
void soundEffectSlot3Playing_stop(NES_State *nes);                /* $E540 */
void updateSoundEffectSlot3_apu(NES_State *nes);                  /* $E551 */
void soundEffectSlot3_fallingAlien(NES_State *nes);               /* $E56E */

/* NMI Audio Synchronization Routines ($AA2F - $AA5F) */
void updateAudioWaitForNmiAndResetOamStaging(NES_State *nes);     /* $AA2F */
void updateAudioAndWaitForNmi(NES_State *nes);                    /* $AA45 */
void updateAudioWaitForNmiAndDisablePpuRendering(NES_State *nes); /* $AA52 */
void _updatePpuMask(NES_State *nes, uint8_t mask);                /* $AA59 */
void updateAudioWaitForNmiAndEnablePpuRendering(NES_State *nes);  /* $AA5F */

/* Music Subsystem & Bytecode Sequence Interpreter Routines ($E580 - $EA45) */
void updateMusic_noSoundJmp(NES_State *nes);                       /* $E580 */
void updateMusic(NES_State *nes);                                 /* $E583 */
void updateMusicFrame_setChanLo(NES_State *nes, uint8_t chan);    /* $E5D0 */
void musicLoOffset_8AndC(NES_State *nes, uint8_t chan);           /* $E5FA */
void LE613(NES_State *nes, uint8_t chan, uint8_t offset_a);       /* $E613 */
void musicLoOffset_4(NES_State *nes, uint8_t chan);               /* $E617 */
void musicLoOffset_minus2_6(NES_State *nes, uint8_t chan);        /* $E628 */
void updateMusicFrame_setChanLoOffset(NES_State *nes, uint8_t chan); /* $E637 */
void musicLoOffset_setLoAndSaveFrameCounter(NES_State *nes, uint8_t chan, uint8_t y_counter, uint8_t a_val); /* $E671 */
void musicLoOffset_setLo(NES_State *nes, uint8_t chan, uint8_t a_val); /* $E677 */
void copyFFFFToDeref(NES_State *nes, uint8_t chan2);              /* $E66E */
void loadMusicTrack(NES_State *nes);                              /* $E6D7 */
void derefNextAddr(NES_State *nes);                               /* $E711 */
void storeDeref1AndContinue(NES_State *nes, uint8_t chan2, uint8_t hi_byte); /* $E72A */
void initSq12IfTrashedBySoundEffect(NES_State *nes);              /* $E735 */
void updateMusicFrame_setChanVol(NES_State *nes, uint8_t chan);   /* $E766 */
void updateMusicFrame_progLoadNextScript(NES_State *nes, uint8_t chan); /* $E7D9 */
void updateMusicFrame_progEnd(NES_State *nes);                    /* $E7FB */
void updateMusicFrame_ret(NES_State *nes);                        /* $E7FE */
void updateMusicFrame_progNextRoutine(NES_State *nes, uint8_t chan); /* $E7FF */
void updateMusicFrame_progLoadRoutine(NES_State *nes, uint8_t chan, uint8_t offset_y); /* $E818 */
void updateMusicFrame_progNextRoutine_jmp(NES_State *nes, uint8_t chan); /* $E83D */
void updateMusicFrame(NES_State *nes);                            /* $E840 */
void updateMusicFrame_incSlotFromOffset(NES_State *nes, uint8_t chan2); /* $E84A */
void updateMusicFrame_incSlot(NES_State *nes, uint8_t chan);      /* $E84D */
void updateMusicFrame_updateChannel(NES_State *nes, uint8_t chan); /* $E85A */
void playNoise(NES_State *nes, uint8_t note_masked);              /* $E9EB */
void playDmc(NES_State *nes, uint8_t note_val);                   /* $EA03 */
uint8_t musicGetNextInstructionByte(NES_State *nes, uint8_t chan); /* $EA41 */

/* Music ROM Data Tables ($E5AB - $EC90) */
extern const uint8_t noises_table[35];
extern const uint8_t loOff9To0FallTable[12];
extern const uint8_t loOffSlowStartTrillTable[33];
extern const uint8_t loOffTrillNeg2To2Table[10];
extern const uint8_t loOffDescendToNeg11BounceToNeg9Table[16];
extern const uint8_t musicChanVolControlTable[44];
extern const uint8_t envelope_tables_EA76_EB13[157];
extern const uint8_t noteToWaveTable[156];
extern const uint8_t noteDurationTable[116];
extern const uint8_t musicDataTableIndex[10];
extern const uint8_t musicDataTable[100];

#ifdef __cplusplus
}
#endif

#endif /* TDECOMP_GAME_AUDIO_H */
