#include "game/audio.h"
#include "game/state.h"
#include "game/render.h"
#include <string.h>

/* Authoritative ROM Data Tables in PRG chunk 2 */
const uint8_t soundEffectSlot0_gameOverCurtainInitData[4] = { 0x1F, 0x7F, 0x0F, 0xC0 };  /* $E104 */
const uint8_t soundEffectSlot0_endingRocketInitData[4]   = { 0x08, 0x7F, 0x0E, 0xC0 };  /* $E108 */
const uint8_t music_pause_sq1_even[4]                    = { 0x9D, 0x7F, 0x7A, 0x28 };  /* $E10C */
const uint8_t music_pause_sq1_odd[4]                     = { 0x9D, 0x7F, 0x40, 0x28 };  /* $E110 */
const uint8_t soundEffectSlot1_rotateTetriminoInitData[4] = { 0x9E, 0x7F, 0xC0, 0x28 }; /* $E114 */
const uint8_t soundEffectSlot1Playing_rotateTetriminoStage3[4] = { 0xB2, 0x7F, 0xC0, 0x08 }; /* $E118 */
const uint8_t soundEffectSlot1_levelUpInitData[4]        = { 0xDE, 0x7F, 0xA8, 0x18 };  /* $E11C */
const uint8_t soundEffectSlot1_lockTetriminoInitData[4]  = { 0x9F, 0x84, 0xFF, 0x0B };  /* $E120 */
const uint8_t soundEffectSlot1_menuOptionSelectInitData[4] = { 0xDB, 0x7F, 0x40, 0x28 };/* $E124 */
const uint8_t soundEffectSlot1Playing_menuOptionSelectStage2[4] = { 0xD2, 0x7F, 0x40, 0x28 }; /* $E128 */
const uint8_t soundEffectSlot1_menuScreenSelectInitData[4] = { 0xD9, 0x7F, 0x84, 0x28 }; /* $E12C */
const uint8_t soundEffectSlot1_tetrisAchievedInitData[4] = { 0x9E, 0x9D, 0xC0, 0x08 };  /* $E130 */
const uint8_t soundEffectSlot1_lineCompletedInitData[4]  = { 0x9C, 0x9A, 0xA0, 0x09 };  /* $E134 */
const uint8_t soundEffectSlot1_lineClearingInitData[4]   = { 0x9E, 0x7F, 0x69, 0x08 };  /* $E138 */
const uint8_t soundEffectSlot1_chirpChirpInitData[4]     = { 0x96, 0x7F, 0x36, 0x20 };  /* $E13C */
const uint8_t soundEffectSlot1Playing_chirpChirpStage2[4] = { 0x82, 0x7F, 0x30, 0xF8 }; /* $E140 */
const uint8_t soundEffectSlot1_shiftTetriminoInitData[4] = { 0x98, 0x7F, 0x80, 0x38 };  /* $E144 */
const uint8_t soundEffectSlot3_fallingAlienInitData[4]   = { 0x30, 0x7F, 0x70, 0x08 };  /* $E148 */
const uint8_t soundEffectSlot3_donkInitData[4]           = { 0x03, 0x7F, 0x3D, 0x18 };  /* $E14C */
const uint8_t soundEffectSlot1_chirpChirpSq1Vol_table[4] = { 0x14, 0x93, 0x94, 0xD3 };  /* $E150 */

const uint8_t noiselo_table[32] = {                       /* $E154 */
    0x7A, 0xDE, 0xFF, 0xEF, 0xFD, 0xDF, 0xFE, 0xEF,
    0xEF, 0xFD, 0xEF, 0xFE, 0xDF, 0xFF, 0xEE, 0xEE,
    0xFF, 0xEF, 0xFF, 0xFF, 0xFF, 0xEF, 0xEF, 0xFF,
    0xFD, 0xDF, 0xDF, 0xEF, 0xFE, 0xDF, 0xEF, 0xFF
};

const uint8_t noisevol_table[32] = {                      /* $E174 */
    0xBF, 0xFF, 0xEE, 0xEF, 0xEF, 0xEF, 0xDF, 0xFB,
    0xBB, 0xAA, 0xAA, 0x99, 0x98, 0x87, 0x76, 0x66,
    0x55, 0x44, 0x44, 0x44, 0x44, 0x43, 0x33, 0x33,
    0x22, 0x22, 0x22, 0x22, 0x21, 0x11, 0x11, 0x11
};

const uint8_t soundEffectSlot1_lineClearing_vol[9] = {    /* $E4B0 */
    0x9E, 0x9B, 0x99, 0x96, 0x94, 0x93, 0x92, 0x91, 0x00
};

const uint8_t soundEffectSlot1_lineClearing_lo[24] = {    /* $E4B9 */
    0x46, 0x37, 0x46, 0x37, 0x46, 0x37, 0x46, 0x37,
    0x70, 0x80, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0,
    0xC0, 0x89, 0xB8, 0x68, 0xA0, 0x50, 0x90, 0x40
};

const uint8_t soundEffectSlot1_levelUp_lo[24] = {         /* $E4F3 */
    0x69, 0xA8, 0x69, 0xA8, 0x8D, 0x53, 0x8D, 0x53,
    0x8D, 0x00, 0xA9, 0x10, 0x8D, 0x04, 0x40, 0xA9,
    0x00, 0x8D, 0xC9, 0x06, 0x8D, 0xFA, 0x06, 0x60
};

const uint8_t trilo_table[4] = {                          /* $E57C */
    0x72, 0x74, 0x77, 0x00
};

/* 16-bit address dispatch tables in PRG chunk 2 */
const uint16_t soundEffectSlot0Init_table[3] = {          /* $E009 */
    0xE2DD, 0xE2F8, 0xE2CC
};
const uint16_t soundEffectSlot0Playing_table[3] = {       /* $E00F */
    0xE2E4, 0xE2FF, 0xE2E4
};
const uint16_t soundEffectSlot1Init_table[10] = {         /* $E015 */
    0xE3CA, 0xE473, 0xE390, 0xE403, 0xE3D1, 0xE4EC, 0xE384, 0xE37F, 0xE42E, 0xE41A
};
const uint16_t soundEffectSlot1Playing_table[10] = {      /* $E029 */
    0xE3B3, 0xE481, 0xE39C, 0xE40E, 0xE3DD, 0xE4D1, 0xE39C, 0xE34F, 0xE43B, 0xE425
};
const uint16_t soundEffectSlot3Init_table[2] = {          /* $E03D */
    0xE56E, 0xE534
};
const uint16_t soundEffectSlot3Playing_table[2] = {       /* $E041 */
    0xE551, 0xE53B
};
const uint16_t soundEffectSlot2Init_table[3] = {          /* $E045 */
    0xE244, 0xE513, 0xE50B
};

/* Forward internal declaration */
static void executeSoundEffectRoutine(NES_State *nes, uint16_t addr);

/*
 * Copies 4 bytes from $E100+src_y to APU channel at $4000+apu_offset.
 * Source address: $E059 (main.asm lines 5915-5930).
 */
void copyToApuChannel(NES_State *nes, uint8_t apu_offset, uint8_t src_y) {
    if (!nes) return;
    nes->ram[ADDR_AUDIOTMP1] = apu_offset;
    nes->ram[ADDR_AUDIOTMP2] = 0x40;
    nes->ram[ADDR_AUDIOTMP3] = src_y;
    nes->ram[ADDR_AUDIOTMP4] = 0xE1;
    for (uint8_t i = 0; i < 4; i++) {
        uint16_t rom_addr = (uint16_t)(0xE100 + src_y + i);
        uint8_t byte = nes_read8(nes, rom_addr);
        nes_write8(nes, (uint16_t)(0x4000 + apu_offset + i), byte);
    }
}

void copyToSq1Channel(NES_State *nes, uint8_t src_y) {
    copyToApuChannel(nes, 0x00, src_y);
}

void copyToTriChannel(NES_State *nes, uint8_t src_y) {
    copyToApuChannel(nes, 0x08, src_y);
}

void copyToNoiseChannel(NES_State *nes, uint8_t src_y) {
    copyToApuChannel(nes, 0x0C, src_y);
}

void copyToSq2Channel(NES_State *nes, uint8_t src_y) {
    copyToApuChannel(nes, 0x04, src_y);
}

/*
 * Resolves 16-bit sound effect method address from dispatch table at $E000+AUDIOTMP1.
 * Source address: $E072 (main.asm lines 5933-5962).
 */
uint16_t computeSoundEffMethod(NES_State *nes, uint8_t index) {
    if (!nes) return 0;
    nes->ram[ADDR_CURRENT_AUDIO_SLOT] = index;
    nes->ram[ADDR_AUDIOTMP2] = 0xE0;

    uint8_t count = index;
    uint8_t y = 0;
    while (1) {
        count--;
        if (count == 0) {
            uint16_t tbl_base = (uint16_t)(0xE000 | nes->ram[ADDR_AUDIOTMP1]);
            uint8_t lo = nes_read8(nes, (uint16_t)(tbl_base + y));
            uint8_t hi = nes_read8(nes, (uint16_t)(tbl_base + y + 1));
            nes->ram[ADDR_AUDIOTMP3] = lo;
            nes->ram[ADDR_AUDIOTMP4] = hi;
            nes->ram[ADDR_CURRENT_AUDIO_SLOT] = index;
            return MAKE_WORD(lo, hi);
        }
        y += 2;
        if (y == 0x22) {
            nes->ram[ADDR_AUDIOTMP3] = 0x91;
            nes->ram[ADDR_AUDIOTMP4] = 0xE0;
            nes->ram[ADDR_CURRENT_AUDIO_SLOT] = index;
            return 0xE091;
        }
    }
}

/*
 * Advances the frame counter for currentSoundEffectSlot.
 * Returns true if counter reached count and wrapped to 0.
 * Source address: $E0B5 (main.asm lines 5979-5987).
 */
bool advanceAudioSlotFrame(NES_State *nes) {
    if (!nes) return false;
    uint8_t slot = nes->ram[ADDR_CURRENT_SFX_SLOT];
    nes->ram[ADDR_SFX_SLOT0_FRAME_COUNTER + slot]++;
    if (nes->ram[ADDR_SFX_SLOT0_FRAME_COUNTER + slot] == nes->ram[ADDR_SFX_SLOT0_FRAME_COUNT + slot]) {
        nes->ram[ADDR_SFX_SLOT0_FRAME_COUNTER + slot] = 0;
        return true;
    }
    return false;
}

/*
 * Generic sound effect slot initializer.
 * Source address: $E287 (main.asm lines 6200-6234).
 */
void initSoundEffectShared(NES_State *nes, uint8_t frame_count, uint8_t init_data_y) {
    if (!nes) return;
    uint8_t slot = nes->ram[ADDR_CURRENT_SFX_SLOT];
    nes->ram[ADDR_SFX_SLOT0_FRAME_COUNT + slot] = frame_count;
    nes->ram[0x06C7 + slot] = slot;
    if (init_data_y != 0) {
        switch (slot) {
            case 0: copyToNoiseChannel(nes, init_data_y); break;
            case 1: copyToSq1Channel(nes, init_data_y); break;
            case 2: copyToSq2Channel(nes, init_data_y); break;
            case 3: copyToTriChannel(nes, init_data_y); break;
            default: break;
        }
    }
    nes->ram[ADDR_SOUND_EFFECT_SLOT0_PLAYING + slot] = nes->ram[ADDR_CURRENT_AUDIO_SLOT];
    nes->ram[ADDR_SFX_SLOT0_FRAME_COUNTER + slot] = 0;
    nes->ram[ADDR_SFX_SLOT0_SECONDARY_COUNTER + slot] = 0;
    nes->ram[ADDR_SFX_SLOT0_TERTIARY_COUNTER + slot] = 0;
    nes->ram[ADDR_SFX_SLOT0_TMP + slot] = 0;
    nes->ram[ADDR_RESET_SQ12_FOR_MUSIC] = 0;
}

/*
 * Dispatches to individual sound effect implementation based on address.
 */
static void executeSoundEffectRoutine(NES_State *nes, uint16_t addr) {
    switch (addr) {
        case 0xE2DD: soundEffectSlot0_makesNoSound(nes); break;
        case 0xE2F8: soundEffectSlot0_gameOverCurtainInit(nes); break;
        case 0xE2CC: soundEffectSlot0_endingRocketInit(nes); break;
        case 0xE2E4: advanceSoundEffectSlot0WithoutUpdate(nes); break;
        case 0xE2FF: updateSoundEffectSlot0_apu(nes); break;

        case 0xE3CA: soundEffectSlot1_menuOptionSelectInit(nes); break;
        case 0xE473: soundEffectSlot1_menuScreenSelectInit(nes); break;
        case 0xE390: soundEffectSlot1_shiftTetriminoInit(nes); break;
        case 0xE403: soundEffectSlot1_tetrisAchievedInit(nes); break;
        case 0xE3D1: soundEffectSlot1_rotateTetriminoInit(nes); break;
        case 0xE4EC: soundEffectSlot1_levelUpInit(nes); break;
        case 0xE384: soundEffectSlot1_lockTetriminoInit(nes); break;
        case 0xE37F: soundEffectSlot1_chirpChirpInit(nes); break;
        case 0xE42E: soundEffectSlot1_lineClearingInit(nes); break;
        case 0xE41A: soundEffectSlot1_lineCompletedInit(nes); break;

        case 0xE3B3: soundEffectSlot1_menuOptionSelectPlaying(nes); break;
        case 0xE481: soundEffectSlot1_menuScreenSelectPlaying(nes); break;
        case 0xE39C: soundEffectSlot1Playing_advance(nes); break;
        case 0xE40E: soundEffectSlot1_tetrisAchievedPlaying(nes); break;
        case 0xE3DD: soundEffectSlot1_rotateTetriminoPlaying(nes); break;
        case 0xE4D1: soundEffectSlot1_levelUpPlaying(nes); break;
        case 0xE34F: soundEffectSlot1_chirpChirpPlaying(nes); break;
        case 0xE43B: soundEffectSlot1_lineClearingPlaying(nes); break;
        case 0xE425: soundEffectSlot1_lineCompletedPlaying(nes); break;

        case 0xE244: soundEffectSlot2_makesNoSound(nes); break;
        case 0xE513: soundEffectSlot2_lowBuzz(nes); break;
        case 0xE50B: soundEffectSlot2_mediumBuzz(nes); break;

        case 0xE56E: soundEffectSlot3_fallingAlien(nes); break;
        case 0xE534: soundEffectSlot3_donk(nes); break;
        case 0xE551: updateSoundEffectSlot3_apu(nes); break;
        case 0xE53B: soundEffectSlot3Playing_advance(nes); break;

        default: break;
    }
}

/*
 * Shared sound effect slot updater.
 * Source address: $E1BF (main.asm lines 6090-6105).
 */
void updateSoundEffectSlotShared(NES_State *nes, uint8_t slot, uint8_t init_table_lo, uint8_t play_table_lo) {
    if (!nes) return;
    nes->ram[ADDR_AUDIOTMP1] = init_table_lo;
    nes->ram[ADDR_CURRENT_SFX_SLOT] = slot;
    uint8_t init_val = nes->ram[ADDR_SOUND_EFFECT_SLOT0_INIT + slot];
    if (init_val != 0) {
        uint16_t target_addr = computeSoundEffMethod(nes, init_val);
        executeSoundEffectRoutine(nes, target_addr);
        return;
    }

    uint8_t play_val = nes->ram[ADDR_SOUND_EFFECT_SLOT0_PLAYING + slot];
    if (play_val != 0) {
        nes->ram[ADDR_AUDIOTMP1] = play_table_lo;
        uint16_t target_addr = computeSoundEffMethod(nes, play_val);
        executeSoundEffectRoutine(nes, target_addr);
    }
}

void updateSoundEffectSlot0(NES_State *nes) {
    updateSoundEffectSlotShared(nes, 0, 0x09, 0x0F);
}

void updateSoundEffectSlot1(NES_State *nes) {
    if (!nes) return;
    if (nes->ram[ADDR_SOUND_EFFECT_SLOT4_PLAYING] != 0) return;
    updateSoundEffectSlotShared(nes, 1, 0x15, 0x29);
}

void updateSoundEffectSlot2(NES_State *nes) {
    updateSoundEffectSlotShared(nes, 2, 0x45, 0x45);
}

void updateSoundEffectSlot3(NES_State *nes) {
    updateSoundEffectSlotShared(nes, 3, 0x3D, 0x41);
}

/*
 * Mutes APU channels and clears DMC raw volume.
 * Source address: $E271 (main.asm lines 6188-6197).
 */
void muteAudio(NES_State *nes) {
    if (!nes) return;
    nes_write8(nes, REG_DMC_RAW, 0x00);
    nes_write8(nes, REG_SQ1_VOL, 0x10);
    nes_write8(nes, REG_SQ2_VOL, 0x10);
    nes_write8(nes, REG_NOISE_VOL, 0x10);
    nes_write8(nes, REG_TRI_LINEAR, 0x00);
}

/*
 * Clears playing status for all 6 sound effect slots ($06F8 - $06FD).
 * Source address: $E265 (main.asm lines 6180-6186).
 */
void LE265(NES_State *nes) {
    if (!nes) return;
    for (uint8_t y = 0; y < 6; y++) {
        nes->ram[ADDR_SOUND_EFFECT_SLOT0_PLAYING + y] = 0;
    }
}

/*
 * Resets channel inhibits and clears playing sound effects.
 * Source address: $E253 (main.asm lines 6173-6186).
 */
void LE253(NES_State *nes) {
    if (!nes) return;
    nes->ram[ADDR_MUSIC_CHAN_INHIBIT] = 0;
    nes->ram[ADDR_MUSIC_CHAN_INHIBIT + 1] = 0;
    nes->ram[ADDR_MUSIC_CHAN_INHIBIT + 2] = 0;
    nes->ram[ADDR_MUSIC_STAGING_NOISE_LO] = 0;
    nes->ram[ADDR_RESET_SQ12_FOR_MUSIC] = 0;
    LE265(nes);
}

/*
 * Mutes audio and clears triangle channel control.
 * Source address: $E247 (main.asm lines 6166-6171).
 */
void muteAudioAndClearTriControl(NES_State *nes) {
    if (!nes) return;
    muteAudio(nes);
    nes_write8(nes, REG_DMC_RAW, 0x00);
    nes->ram[ADDR_MUSIC_CHAN_CONTROL + 2] = 0;
}

/*
 * Sound effect slot 2 dummy / mute handler.
 * Source address: $E244 (main.asm lines 6164-6171).
 */
void soundEffectSlot2_makesNoSound(NES_State *nes) {
    if (!nes) return;
    LE253(nes);
    muteAudioAndClearTriControl(nes);
}

/*
 * Boot / reset audio initializer.
 * Source address: $E1D8 (main.asm lines 6107-6112).
 */
void LE1D8(NES_State *nes) {
    if (!nes) return;
    nes_write8(nes, REG_SND_CHN, 0x0F);
    nes->ram[ADDR_SOUND_RNG_SEED] = 0x55;
    soundEffectSlot2_makesNoSound(nes);
}

/*
 * Audio pause initialization helper.
 * Source address: $E1E5 (main.asm lines 6114-6118).
 */
void initAudioAndMarkInited(NES_State *nes) {
    if (!nes) return;
    nes->ram[ADDR_AUDIO_INITIALIZED]++;
    muteAudio(nes);
    nes->ram[ADDR_MUSIC_PAUSE_SFX_LEN_COUNTER] = 0;
}

/*
 * Handles pause sound effect pulse alternation.
 * Source address: $E1EF (main.asm lines 6120-6139).
 */
void updateAudio_pause(NES_State *nes) {
    if (!nes) return;
    if (nes->ram[ADDR_AUDIO_INITIALIZED] == 0) {
        initAudioAndMarkInited(nes);
        return;
    }
    if (nes->ram[ADDR_MUSIC_PAUSE_SFX_LEN_COUNTER] == 0x12) {
        return;
    }
    if ((nes->ram[ADDR_MUSIC_PAUSE_SFX_LEN_COUNTER] & 0x03) == 0x03) {
        nes->ram[ADDR_MUSIC_PAUSE_SFX_COUNTER]++;
        if (nes->ram[ADDR_MUSIC_PAUSE_SFX_COUNTER] & 1) {
            copyToSq1Channel(nes, 0x10); /* music_pause_sq1_odd = $E110 */
        } else {
            copyToSq1Channel(nes, 0x0C); /* music_pause_sq1_even = $E10C */
        }
    }
    nes->ram[ADDR_MUSIC_PAUSE_SFX_LEN_COUNTER]++;
}

/*
 * Master audio frame update routine.
 * Source address: $E216 (main.asm lines 6141-6162).
 */
void updateAudio(NES_State *nes) {
    if (!nes) return;
    nes_write8(nes, REG_JOY2_APUFC, 0xC0);
    if (nes->ram[ADDR_MUSIC_STAGING_NOISE_HI] == 5) {
        updateAudio_pause(nes);
        return;
    }
    nes->ram[ADDR_AUDIO_INITIALIZED] = 0;
    nes->ram[ADDR_MUSIC_PAUSE_SFX_COUNTER] = 0;

    updateSoundEffectSlot2(nes);
    updateSoundEffectSlot0(nes);
    updateSoundEffectSlot3(nes);
    updateSoundEffectSlot1(nes);

    for (int8_t x = 6; x > 0; x--) {
        nes->ram[0x06EF + x] = 0;
    }
}

/*
 * Slot 0 (Noise channel) routines
 */
void soundEffectSlot0_endingRocketInit(NES_State *nes) {
    if (!nes) return;
    initSoundEffectShared(nes, 0x20, 0x08); /* $E108 = soundEffectSlot0_endingRocketInitData */
}

void setNoiseLo(NES_State *nes, uint8_t lo) {
    if (!nes) return;
    nes_write8(nes, REG_NOISE_LO, lo);
}

void loadNoiseLo(NES_State *nes) {
    if (!nes) return;
    uint8_t nibble = getSoundEffectNoiseNibble(nes, 0x54); /* $E154 = noiselo_table */
    setNoiseLo(nes, nibble);
}

void soundEffectSlot0_makesNoSound(NES_State *nes) {
    if (!nes) return;
    initSoundEffectShared(nes, 0x10, 0x00);
}

void stopSoundEffectSlot0(NES_State *nes) {
    if (!nes) return;
    nes->ram[ADDR_SOUND_EFFECT_SLOT0_PLAYING] = 0;
    nes_write8(nes, REG_NOISE_VOL, 0x10);
}

void advanceSoundEffectSlot0WithoutUpdate(NES_State *nes) {
    if (!nes) return;
    if (advanceAudioSlotFrame(nes)) {
        stopSoundEffectSlot0(nes);
    }
}

void soundEffectSlot0_gameOverCurtainInit(NES_State *nes) {
    if (!nes) return;
    initSoundEffectShared(nes, 0x40, 0x04); /* $E104 = soundEffectSlot0_gameOverCurtainInitData */
}

void updateSoundEffectNoiseAudio(NES_State *nes) {
    if (!nes) return;
    loadNoiseLo(nes);
    uint8_t nibble = getSoundEffectNoiseNibble(nes, 0x74); /* $E174 = noisevol_table */
    nes_write8(nes, REG_NOISE_VOL, (uint8_t)(nibble | 0x10));
    nes->ram[ADDR_SFX_SLOT0_SECONDARY_COUNTER]++;
}

void updateSoundEffectSlot0_apu(NES_State *nes) {
    if (!nes) return;
    if (!advanceAudioSlotFrame(nes)) {
        updateSoundEffectNoiseAudio(nes);
    } else {
        stopSoundEffectSlot0(nes);
    }
}

uint8_t getSoundEffectNoiseNibble(NES_State *nes, uint8_t table_lo) {
    if (!nes) return 0;
    nes->ram[ADDR_AUDIOTMP1] = table_lo;
    nes->ram[ADDR_AUDIOTMP2] = 0xE1;
    uint8_t sec_cnt = nes->ram[ADDR_SFX_SLOT0_SECONDARY_COUNTER];
    uint8_t byte_idx = (uint8_t)(sec_cnt >> 1);
    uint8_t byte_val = nes_read8(nes, (uint16_t)((0xE100 | table_lo) + byte_idx));
    nes->ram[ADDR_AUDIOTMP5] = byte_val;
    if (sec_cnt & 1) {
        return byte_val & 0x0F;
    } else {
        return (byte_val >> 4) & 0x0F;
    }
}

/*
 * Slot 1 (Pulse 1 channel) routines
 */
bool LE33B(NES_State *nes) {
    if (!nes) return false;
    uint8_t playing = nes->ram[ADDR_SOUND_EFFECT_SLOT1_PLAYING];
    if (playing == 4 || playing == 6 || playing == 9 || playing == 10) {
        return true;
    }
    return false;
}

void soundEffectSlot1Playing_stop(NES_State *nes) {
    if (!nes) return;
    nes_write8(nes, REG_SQ1_VOL, 0x10);
    nes->ram[ADDR_MUSIC_CHAN_INHIBIT] = 0;
    nes->ram[ADDR_SOUND_EFFECT_SLOT1_PLAYING] = 0;
    nes->ram[ADDR_RESET_SQ12_FOR_MUSIC]++;
}

void soundEffectSlot1Playing_advance(NES_State *nes) {
    if (!nes) return;
    if (advanceAudioSlotFrame(nes)) {
        soundEffectSlot1Playing_stop(nes);
    }
}

void soundEffectSlot1_lockTetriminoInit(NES_State *nes) {
    if (!nes) return;
    if (LE33B(nes)) return;
    initSoundEffectShared(nes, 0x0F, 0x20); /* $E120 = lockTetriminoInitData */
}

void soundEffectSlot1_shiftTetriminoInit(NES_State *nes) {
    if (!nes) return;
    if (LE33B(nes)) return;
    initSoundEffectShared(nes, 0x02, 0x44); /* $E144 = shiftTetriminoInitData */
}

void soundEffectSlot1_menuOptionSelectInit(NES_State *nes) {
    if (!nes) return;
    initSoundEffectShared(nes, 0x03, 0x24); /* $E124 = menuOptionSelectInitData */
}

void soundEffectSlot1_menuOptionSelectPlaying(NES_State *nes) {
    if (!nes) return;
    if (!advanceAudioSlotFrame(nes)) return;
    nes->ram[ADDR_SFX_SLOT1_SECONDARY_COUNTER]++;
    if (nes->ram[ADDR_SFX_SLOT1_SECONDARY_COUNTER] == 2) {
        soundEffectSlot1Playing_stop(nes);
    } else {
        copyToSq1Channel(nes, 0x28); /* $E128 = menuOptionSelectStage2 */
    }
}

void soundEffectSlot1_rotateTetriminoInit(NES_State *nes) {
    if (!nes) return;
    if (LE33B(nes)) return;
    initSoundEffectShared(nes, 0x04, 0x14); /* $E114 = rotateTetriminoInitData */
}

void soundEffectSlot1_rotateTetriminoPlaying(NES_State *nes) {
    if (!nes) return;
    if (!advanceAudioSlotFrame(nes)) return;
    uint8_t old = nes->ram[ADDR_SFX_SLOT1_SECONDARY_COUNTER];
    nes->ram[ADDR_SFX_SLOT1_SECONDARY_COUNTER]++;
    if (old == 0) {
        copyToSq1Channel(nes, 0x18); /* stage 3: $E118 */
    } else if (old == 1) {
        copyToSq1Channel(nes, 0x14); /* stage 2: $E114 */
    } else if (old == 2) {
        copyToSq1Channel(nes, 0x18); /* stage 3: $E118 */
    } else if (old == 3) {
        soundEffectSlot1Playing_stop(nes);
    }
}

void soundEffectSlot1_tetrisAchievedInit(NES_State *nes) {
    if (!nes) return;
    initSoundEffectShared(nes, SFX_TETRIS_INIT, 0x30); /* $E130 = tetrisAchievedInitData */
    nes->ram[ADDR_SFX_SLOT1_TERTIARY_COUNTER] = 0x10;
}

void soundEffectSlot1_lineCompletedInit(NES_State *nes) {
    if (!nes) return;
    initSoundEffectShared(nes, SFX_LINE_COMPLETE_INIT, 0x34); /* $E134 = lineCompletedInitData */
    nes->ram[ADDR_SFX_SLOT1_TERTIARY_COUNTER] = 0x08;
}

void soundEffectSlot1_lineClearingInit(NES_State *nes) {
    if (!nes) return;
    initSoundEffectShared(nes, SFX_LINECLEAR_INIT, 0x38); /* $E138 = lineClearingInitData */
    nes->ram[ADDR_SFX_SLOT1_TERTIARY_COUNTER] = 0x00;
}

void LE442(NES_State *nes, uint8_t init_data_y) {
    if (!nes) return;
    copyToSq1Channel(nes, init_data_y);
    uint8_t sec = nes->ram[ADDR_SFX_SLOT1_SECONDARY_COUNTER];
    uint8_t tert = nes->ram[ADDR_SFX_SLOT1_TERTIARY_COUNTER];
    uint8_t idx = (uint8_t)(tert + sec);
    if (idx < sizeof(soundEffectSlot1_lineClearing_lo)) {
        nes_write8(nes, REG_SQ1_LO, soundEffectSlot1_lineClearing_lo[idx]);
    }
    uint8_t vol = (sec < sizeof(soundEffectSlot1_lineClearing_vol)) ? soundEffectSlot1_lineClearing_vol[sec] : 0;
    nes_write8(nes, REG_SQ1_VOL, vol);
    if (vol != 0) {
        nes->ram[ADDR_SFX_SLOT1_SECONDARY_COUNTER]++;
    } else {
        if (nes->ram[ADDR_SOUND_EFFECT_SLOT1_PLAYING] == 4) {
            nes->ram[ADDR_CURRENT_AUDIO_SLOT] = 9;
            soundEffectSlot1_lineClearingInit(nes);
        } else {
            soundEffectSlot1Playing_stop(nes);
        }
    }
}

void soundEffectSlot1_tetrisAchievedPlaying(NES_State *nes) {
    if (!nes) return;
    if (!advanceAudioSlotFrame(nes)) return;
    LE442(nes, 0x30); /* $E130 = tetrisAchievedInitData */
}

void soundEffectSlot1_lineCompletedPlaying(NES_State *nes) {
    if (!nes) return;
    if (!advanceAudioSlotFrame(nes)) return;
    LE442(nes, 0x34); /* $E134 = lineCompletedInitData */
}

void soundEffectSlot1_lineClearingPlaying(NES_State *nes) {
    if (!nes) return;
    if (!advanceAudioSlotFrame(nes)) return;
    LE442(nes, 0x38); /* $E138 = lineClearingInitData */
}

void soundEffectSlot1_chirpChirpInit(NES_State *nes) {
    if (!nes) return;
    initSoundEffectShared(nes, 0x00, 0x3C); /* $E13C = chirpChirpInitData */
}

void soundEffectSlot1_chirpChirpPlaying(NES_State *nes) {
    if (!nes) return;
    if (nes->ram[ADDR_SFX_SLOT1_TERTIARY_COUNTER] == 0) {
        uint8_t sec = nes->ram[ADDR_SFX_SLOT1_SECONDARY_COUNTER];
        uint8_t vol = soundEffectSlot1_chirpChirpSq1Vol_table[sec & 0x03];
        nes_write8(nes, REG_SQ1_VOL, vol);
        nes->ram[ADDR_SFX_SLOT1_SECONDARY_COUNTER]++;
        if (nes->ram[ADDR_SFX_SLOT1_SECONDARY_COUNTER] == 0x08) {
            nes->ram[ADDR_SFX_SLOT1_TERTIARY_COUNTER]++;
            copyToSq1Channel(nes, 0x40); /* $E140 = chirpChirpStage2 */
        }
    } else {
        nes->ram[ADDR_SFX_SLOT1_SECONDARY_COUNTER]++;
        if (nes->ram[ADDR_SFX_SLOT1_SECONDARY_COUNTER] == 0x16) {
            soundEffectSlot1Playing_stop(nes);
        }
    }
}

void soundEffectSlot1_menuScreenSelectInit(NES_State *nes) {
    if (!nes) return;
    initSoundEffectShared(nes, 0x03, 0x2C); /* $E12C = menuScreenSelectInitData */
    nes->ram[ADDR_SFX_SLOT1_SECONDARY_COUNTER] = soundEffectSlot1_menuScreenSelectInitData[2]; /* 0x84 */
}

void soundEffectSlot1_menuScreenSelectPlaying(NES_State *nes) {
    if (!nes) return;
    if (!advanceAudioSlotFrame(nes)) return;
    nes->ram[ADDR_SFX_SLOT1_TERTIARY_COUNTER]++;
    if (nes->ram[ADDR_SFX_SLOT1_TERTIARY_COUNTER] == 0x04) {
        soundEffectSlot1Playing_stop(nes);
    } else {
        uint8_t sec = nes->ram[ADDR_SFX_SLOT1_SECONDARY_COUNTER];
        uint8_t tmp = (uint8_t)(sec >> 4);
        nes->ram[ADDR_SFX_SLOT1_TMP] = tmp;
        sec = (uint8_t)(sec - tmp - 1); /* clc; sbc soundEffectSlot1Tmp */
        nes->ram[ADDR_SFX_SLOT1_SECONDARY_COUNTER] = sec;
        nes_write8(nes, REG_SQ1_LO, sec);
        nes_write8(nes, REG_SQ1_HI, 0x28);
    }
}

void soundEffectSlot1_levelUpInit(NES_State *nes) {
    if (!nes) return;
    initSoundEffectShared(nes, SFX_LEVELUP_INIT, 0x1C); /* $E11C = levelUpInitData */
}

void soundEffectSlot1_levelUpPlaying(NES_State *nes) {
    if (!nes) return;
    if (!advanceAudioSlotFrame(nes)) return;
    uint8_t sec = nes->ram[ADDR_SFX_SLOT1_SECONDARY_COUNTER];
    nes->ram[ADDR_SFX_SLOT1_SECONDARY_COUNTER]++;
    uint8_t lo = (sec < sizeof(soundEffectSlot1_levelUp_lo)) ? soundEffectSlot1_levelUp_lo[sec] : 0;
    if (lo == 0) {
        soundEffectSlot1Playing_stop(nes);
    } else {
        nes_write8(nes, REG_SQ1_LO, lo);
        nes_write8(nes, REG_SQ1_HI, 0x28);
    }
}

/*
 * Slot 2 (DMC channel) routines
 */
void LE51B(NES_State *nes, uint8_t len, uint8_t start, uint8_t freq) {
    if (!nes) return;
    nes_write8(nes, REG_DMC_LEN, len);
    nes_write8(nes, REG_DMC_START, start);
    nes_write8(nes, REG_DMC_FREQ, freq);
    nes_write8(nes, REG_SND_CHN, 0x0F);
    nes_write8(nes, REG_DMC_RAW, 0x00);
    nes_write8(nes, REG_SND_CHN, 0x1F);
}

void soundEffectSlot2_mediumBuzz(NES_State *nes) {
    LE51B(nes, 0x3F, 0x60, 0x0F);
}

void soundEffectSlot2_lowBuzz(NES_State *nes) {
    LE51B(nes, 0x3F, 0x60, 0x0E);
}

/*
 * Slot 3 (Triangle channel) routines
 */
void soundEffectSlot3Playing_stop(NES_State *nes) {
    if (!nes) return;
    nes_write8(nes, REG_TRI_LINEAR, 0x00);
    nes->ram[ADDR_MUSIC_CHAN_INHIBIT + 2] = 0;
    nes->ram[ADDR_SOUND_EFFECT_SLOT3_PLAYING] = 0;
    nes_write8(nes, REG_TRI_HI, 0x18);
}

void soundEffectSlot3Playing_advance(NES_State *nes) {
    if (!nes) return;
    if (advanceAudioSlotFrame(nes)) {
        soundEffectSlot3Playing_stop(nes);
    }
}

void soundEffectSlot3_donk(NES_State *nes) {
    if (!nes) return;
    initSoundEffectShared(nes, 0x02, 0x4C); /* $E14C = donkInitData */
}

void soundEffectSlot3_fallingAlien(NES_State *nes) {
    if (!nes) return;
    initSoundEffectShared(nes, 0x06, 0x48); /* $E148 = fallingAlienInitData */
    nes->ram[ADDR_SFX_SLOT3_TERTIARY_COUNTER] = soundEffectSlot3_fallingAlienInitData[2]; /* 0x70 */
}

void updateSoundEffectSlot3_apu(NES_State *nes) {
    if (!nes) return;
    if (!advanceAudioSlotFrame(nes)) return;
    uint8_t sec = nes->ram[ADDR_SFX_SLOT3_SECONDARY_COUNTER];
    nes->ram[ADDR_SFX_SLOT3_SECONDARY_COUNTER]++;
    uint8_t lo = (sec < sizeof(trilo_table)) ? trilo_table[sec] : 0;
    if (lo == 0) {
        soundEffectSlot3Playing_stop(nes);
    } else {
        nes_write8(nes, REG_TRI_LO, lo);
        nes->ram[ADDR_SFX_SLOT3_TERTIARY_COUNTER] = lo;
        nes_write8(nes, REG_TRI_HI, soundEffectSlot3_fallingAlienInitData[3]); /* 0x08 */
    }
}

/*
 * Jumps and Aliases
 */
void updateAudio_jmp(NES_State *nes) {
    updateAudio(nes);
}

void updateAudio2(NES_State *nes) {
    soundEffectSlot2_makesNoSound(nes);
}

void LE006(NES_State *nes) {
    LE1D8(nes);
}

void LE417(NES_State *nes, uint8_t frame_count, uint8_t data_y) {
    initSoundEffectShared(nes, frame_count, data_y);
}

void LE46C(NES_State *nes) {
    soundEffectSlot1Playing_stop(nes);
}

void LE4E9(NES_State *nes) {
    soundEffectSlot1Playing_stop(nes);
}

void LE493(NES_State *nes) {
    if (!nes) return;
    uint8_t sec = nes->ram[ADDR_SFX_SLOT1_SECONDARY_COUNTER];
    uint8_t tmp = (uint8_t)(sec >> 4);
    nes->ram[ADDR_SFX_SLOT1_TMP] = tmp;
    sec = (uint8_t)(sec - tmp - 1);
    nes->ram[ADDR_SFX_SLOT1_SECONDARY_COUNTER] = sec;
    nes_write8(nes, REG_SQ1_LO, sec);
    nes_write8(nes, REG_SQ1_HI, 0x28);
}

void LE4AC(NES_State *nes) {
    if (!nes) return;
    nes_write8(nes, REG_SQ1_HI, 0x28);
}

/*
 * NMI Audio Synchronization Routines ($AA2F - $AA5F)
 */
void _updatePpuMask(NES_State *nes, uint8_t mask) {
    if (!nes) return;
    nes_write8(nes, REG_PPUMASK, mask);
    nes->ram[ADDR_CURRENT_PPU_MASK] = mask;
}

void updateAudioAndWaitForNmi(NES_State *nes) {
    if (!nes) return;
    updateAudio(nes);
    nes->ram[ADDR_VERTICAL_BLANK_INTERVAL] = 1;
}

void updateAudioWaitForNmiAndResetOamStaging(NES_State *nes) {
    if (!nes) return;
    updateAudioAndWaitForNmi(nes);
    memset_page(nes, 0xFF, 0x02, 0x02);
}

void updateAudioWaitForNmiAndDisablePpuRendering(NES_State *nes) {
    if (!nes) return;
    updateAudioAndWaitForNmi(nes);
    uint8_t mask = (uint8_t)(nes->ram[ADDR_CURRENT_PPU_MASK] & 0xE1);
    _updatePpuMask(nes, mask);
}

void updateAudioWaitForNmiAndEnablePpuRendering(NES_State *nes) {
    if (!nes) return;
    updateAudioAndWaitForNmi(nes);
    copyCurrentScrollAndCtrlToPPU(nes);
    uint8_t mask = (uint8_t)(nes->ram[ADDR_CURRENT_PPU_MASK] | 0x1E);
    _updatePpuMask(nes, mask);
}
