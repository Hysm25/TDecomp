#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "nes.h"
#include "game/audio.h"
#include "constants/hardware.h"
#include "constants/memory.h"
#include "constants/gameplay.h"
#include "verification/rom_data.h"

void test_audio_apu_copies(void) {
    NES_State nes;
    nes_init(&nes);

    /* Populate ROM area around $E100 for transfer tests */
    /* $E104 = soundEffectSlot0_gameOverCurtainInitData: 1F 7F 0F C0 */
    nes.prg_rom[0xE104 - 0x8000] = 0x1F;
    nes.prg_rom[0xE105 - 0x8000] = 0x7F;
    nes.prg_rom[0xE106 - 0x8000] = 0x0F;
    nes.prg_rom[0xE107 - 0x8000] = 0xC0;

    /* Copy to Sq1 ($4000) */
    copyToSq1Channel(&nes, 0x04);
    assert(nes.ram[ADDR_AUDIOTMP1] == 0x00);
    assert(nes.ram[ADDR_AUDIOTMP2] == 0x40);
    assert(nes.ram[ADDR_AUDIOTMP3] == 0x04);
    assert(nes.ram[ADDR_AUDIOTMP4] == 0xE1);
    assert(nes.apu_regs[REG_SQ1_VOL - 0x4000] == 0x1F);
    assert(nes.apu_regs[REG_SQ1_SWEEP - 0x4000] == 0x7F);
    assert(nes.apu_regs[REG_SQ1_LO - 0x4000] == 0x0F);
    assert(nes.apu_regs[REG_SQ1_HI - 0x4000] == 0xC0);

    /* Copy to Noise ($400C) */
    copyToNoiseChannel(&nes, 0x04);
    assert(nes.ram[ADDR_AUDIOTMP1] == 0x0C);
    assert(nes.apu_regs[REG_NOISE_VOL - 0x4000] == 0x1F);

    /* Copy to Tri ($4008) */
    copyToTriChannel(&nes, 0x04);
    assert(nes.ram[ADDR_AUDIOTMP1] == 0x08);
    assert(nes.apu_regs[REG_TRI_LINEAR - 0x4000] == 0x1F);

    /* Copy to Sq2 ($4004) */
    copyToSq2Channel(&nes, 0x04);
    assert(nes.ram[ADDR_AUDIOTMP1] == 0x04);
    assert(nes.apu_regs[REG_SQ2_VOL - 0x4000] == 0x1F);
}

void test_audio_compute_method(void) {
    NES_State nes;
    nes_init(&nes);

    /* Set up a mock table at $E020 */
    nes.ram[ADDR_AUDIOTMP1] = 0x20;
    nes.prg_rom[0xE020 - 0x8000] = 0x34;
    nes.prg_rom[0xE021 - 0x8000] = 0x12;
    nes.prg_rom[0xE022 - 0x8000] = 0x78;
    nes.prg_rom[0xE023 - 0x8000] = 0x56;

    /* Index 1 -> entry 0 ($1234) */
    uint16_t addr1 = computeSoundEffMethod(&nes, 1);
    assert(addr1 == 0x1234);
    assert(nes.ram[ADDR_AUDIOTMP3] == 0x34);
    assert(nes.ram[ADDR_AUDIOTMP4] == 0x12);
    assert(nes.ram[ADDR_CURRENT_AUDIO_SLOT] == 1);

    /* Index 2 -> entry 1 ($5678) */
    uint16_t addr2 = computeSoundEffMethod(&nes, 2);
    assert(addr2 == 0x5678);
    assert(nes.ram[ADDR_AUDIOTMP3] == 0x78);
    assert(nes.ram[ADDR_AUDIOTMP4] == 0x56);
    assert(nes.ram[ADDR_CURRENT_AUDIO_SLOT] == 2);
}

void test_audio_advance_frame(void) {
    NES_State nes;
    nes_init(&nes);

    nes.ram[ADDR_CURRENT_SFX_SLOT] = 1;
    nes.ram[ADDR_SFX_SLOT0_FRAME_COUNT + 1] = 3;
    nes.ram[ADDR_SFX_SLOT0_FRAME_COUNTER + 1] = 0;

    assert(!advanceAudioSlotFrame(&nes)); /* counter = 1 */
    assert(nes.ram[ADDR_SFX_SLOT0_FRAME_COUNTER + 1] == 1);

    assert(!advanceAudioSlotFrame(&nes)); /* counter = 2 */
    assert(nes.ram[ADDR_SFX_SLOT0_FRAME_COUNTER + 1] == 2);

    assert(advanceAudioSlotFrame(&nes));  /* counter reaches 3 -> wraps to 0 */
    assert(nes.ram[ADDR_SFX_SLOT0_FRAME_COUNTER + 1] == 0);
}

void test_audio_mute(void) {
    NES_State nes;
    nes_init(&nes);

    nes.ram[ADDR_MUSIC_CHAN_INHIBIT] = 1;
    nes.ram[ADDR_MUSIC_CHAN_INHIBIT + 1] = 2;
    nes.ram[ADDR_MUSIC_CHAN_INHIBIT + 2] = 3;
    nes.ram[ADDR_SOUND_EFFECT_SLOT0_PLAYING + 1] = 5;

    soundEffectSlot2_makesNoSound(&nes);

    assert(nes.ram[ADDR_MUSIC_CHAN_INHIBIT] == 0);
    assert(nes.ram[ADDR_MUSIC_CHAN_INHIBIT + 1] == 0);
    assert(nes.ram[ADDR_MUSIC_CHAN_INHIBIT + 2] == 0);
    assert(nes.ram[ADDR_SOUND_EFFECT_SLOT0_PLAYING + 1] == 0);
    assert(nes.apu_regs[REG_SQ1_VOL - 0x4000] == 0x10);
    assert(nes.apu_regs[REG_SQ2_VOL - 0x4000] == 0x10);
    assert(nes.apu_regs[REG_NOISE_VOL - 0x4000] == 0x10);
    assert(nes.apu_regs[REG_TRI_LINEAR - 0x4000] == 0x00);
    assert(nes.apu_regs[REG_DMC_RAW - 0x4000] == 0x00);
}

void test_audio_pause(void) {
    NES_State nes;
    nes_init(&nes);

    /* Frame 1 of pause: uninitialized -> marks initialized and mutes */
    updateAudio_pause(&nes);
    assert(nes.ram[ADDR_AUDIO_INITIALIZED] == 1);
    assert(nes.ram[ADDR_MUSIC_PAUSE_SFX_LEN_COUNTER] == 0);

    /* Advance frames until counter & 3 == 3 */
    updateAudio_pause(&nes); /* len: 0 -> 1 */
    updateAudio_pause(&nes); /* len: 1 -> 2 */
    updateAudio_pause(&nes); /* len: 2 -> 3 */
    updateAudio_pause(&nes); /* len: 3 -> 4, triggers pulse counter increment */
    assert(nes.ram[ADDR_MUSIC_PAUSE_SFX_COUNTER] == 1);
    assert(nes.ram[ADDR_MUSIC_PAUSE_SFX_LEN_COUNTER] == 4);
}

void test_audio_slot1_sfx_lock_and_priority(void) {
    NES_State nes;
    nes_init(&nes);

    /* Priority SFX active (e.g. 4 = Tetris achieved) */
    nes.ram[ADDR_SOUND_EFFECT_SLOT1_PLAYING] = 4;
    assert(LE33B(&nes) == true);

    /* Lock SFX should NOT interrupt */
    soundEffectSlot1_lockTetriminoInit(&nes);
    assert(nes.ram[ADDR_SOUND_EFFECT_SLOT1_PLAYING] == 4);

    /* Non-priority SFX active (e.g. 1 = menu move) */
    nes.ram[ADDR_SOUND_EFFECT_SLOT1_PLAYING] = 1;
    assert(LE33B(&nes) == false);

    /* Lock SFX can now initialize */
    nes.ram[ADDR_CURRENT_SFX_SLOT] = 1;
    nes.ram[ADDR_CURRENT_AUDIO_SLOT] = 7;
    soundEffectSlot1_lockTetriminoInit(&nes);
    assert(nes.ram[ADDR_SFX_SLOT0_FRAME_COUNT + 1] == 0x0F);
    assert(nes.ram[ADDR_SOUND_EFFECT_SLOT1_PLAYING] == 7);
}

void test_audio_slot2_buzz(void) {
    NES_State nes;
    nes_init(&nes);

    soundEffectSlot2_mediumBuzz(&nes);
    assert(nes.apu_regs[REG_DMC_LEN - 0x4000] == 0x3F);
    assert(nes.apu_regs[REG_DMC_START - 0x4000] == 0x60);
    assert(nes.apu_regs[REG_DMC_FREQ - 0x4000] == 0x0F);
    assert(nes.apu_regs[REG_SND_CHN - 0x4000] == 0x1F);

    soundEffectSlot2_lowBuzz(&nes);
    assert(nes.apu_regs[REG_DMC_FREQ - 0x4000] == 0x0E);
}

void test_audio_slot3_donk_and_alien(void) {
    NES_State nes;
    nes_init(&nes);

    nes.ram[ADDR_CURRENT_SFX_SLOT] = 3;
    nes.ram[ADDR_CURRENT_AUDIO_SLOT] = 2;
    soundEffectSlot3_donk(&nes);
    assert(nes.ram[ADDR_SFX_SLOT0_FRAME_COUNT + 3] == 0x02);
    assert(nes.ram[ADDR_SOUND_EFFECT_SLOT3_PLAYING] == 2);

    /* Advance donk to stop */
    advanceAudioSlotFrame(&nes);
    soundEffectSlot3Playing_advance(&nes);
    assert(nes.ram[ADDR_SOUND_EFFECT_SLOT3_PLAYING] == 0);
    assert(nes.apu_regs[REG_TRI_LINEAR - 0x4000] == 0x00);
}

void test_audio_nmi_synchronizers(void) {
    NES_State nes;
    nes_init(&nes);

    nes.ram[ADDR_CURRENT_PPU_MASK] = 0xFF;
    updateAudioWaitForNmiAndDisablePpuRendering(&nes);
    assert(nes.ram[ADDR_CURRENT_PPU_MASK] == 0xE1);
    assert(nes.apu_regs[REG_JOY2_APUFC - 0x4000] == 0xC0);

    updateAudioWaitForNmiAndEnablePpuRendering(&nes);
    assert(nes.ram[ADDR_CURRENT_PPU_MASK] == 0xFF);

    updateAudioWaitForNmiAndResetOamStaging(&nes);
    assert(nes.ram[ADDR_OAM_STAGING] == 0xFF);
    assert(nes.ram[ADDR_OAM_STAGING + 255] == 0xFF);
}

void test_music_init_sq12_and_mute(void) {
    NES_State nes;
    nes_init(&nes);

    /* Test resetSq12ForMusic == 0 -> does nothing */
    nes.ram[ADDR_RESET_SQ12_FOR_MUSIC] = 0;
    initSq12IfTrashedBySoundEffect(&nes);
    assert(nes.apu_regs[REG_SQ1_SWEEP - 0x4000] == 0);

    /* Test resetSq12ForMusic == 1 -> restores SQ1 only */
    nes.ram[ADDR_RESET_SQ12_FOR_MUSIC] = 1;
    nes.ram[ADDR_MUSIC_STAGING_SQ1_LO] = 0x34;
    nes.ram[ADDR_MUSIC_STAGING_SQ1_HI] = 0x12;
    initSq12IfTrashedBySoundEffect(&nes);
    assert(nes.ram[ADDR_RESET_SQ12_FOR_MUSIC] == 0);
    assert(nes.apu_regs[REG_SQ1_SWEEP - 0x4000] == 0x7F);
    assert(nes.apu_regs[REG_SQ1_LO - 0x4000] == 0x34);
    assert(nes.apu_regs[REG_SQ1_HI - 0x4000] == 0x12);
    assert(nes.apu_regs[REG_SQ2_SWEEP - 0x4000] == 0);

    /* Test resetSq12ForMusic == 2 -> restores SQ1 and SQ2 */
    nes.ram[ADDR_RESET_SQ12_FOR_MUSIC] = 2;
    nes.ram[ADDR_MUSIC_STAGING_SQ2_LO] = 0x78;
    nes.ram[ADDR_MUSIC_STAGING_SQ2_HI] = 0x56;
    initSq12IfTrashedBySoundEffect(&nes);
    assert(nes.ram[ADDR_RESET_SQ12_FOR_MUSIC] == 0);
    assert(nes.apu_regs[REG_SQ2_SWEEP - 0x4000] == 0x7F);
    assert(nes.apu_regs[REG_SQ2_LO - 0x4000] == 0x78);
    assert(nes.apu_regs[REG_SQ2_HI - 0x4000] == 0x56);
    assert(nes.apu_regs[REG_SQ1_SWEEP - 0x4000] == 0x7F);

    /* Test updateMusic_noSoundJmp */
    nes.ram[ADDR_MUSIC_CHAN_CONTROL + 2] = 0xFF;
    updateMusic_noSoundJmp(&nes);
    assert(nes.ram[ADDR_MUSIC_CHAN_CONTROL + 2] == 0);
    assert(nes.apu_regs[REG_SQ1_VOL - 0x4000] == 0x10);
}

void test_music_load_track(void) {
    NES_State nes;
    nes_init(&nes);
    load_reference_rom(&nes, NULL);

    /* Request Track 1 */
    nes.ram[ADDR_CURRENT_AUDIO_SLOT] = 1;
    nes.ram[ADDR_MUSIC_TRACK_DEC] = 0; /* track 1 dec index 0 */
    loadMusicTrack(&nes);

    assert(nes.ram[ADDR_CURRENTLY_PLAYING_MUSIC_TRACK] == 1);
    assert(nes.ram[ADDR_MUSIC_CHAN_NOTE_DURATION_REMAINING] == 1);
    assert(nes.ram[ADDR_MUSIC_CHAN_NOTE_DURATION_REMAINING + 1] == 1);
    assert(nes.ram[ADDR_MUSIC_CHAN_NOTE_DURATION_REMAINING + 2] == 1);
    assert(nes.ram[ADDR_MUSIC_CHAN_NOTE_DURATION_REMAINING + 3] == 1);
    assert(nes.ram[ADDR_MUSIC_DATA_NOTE_TABLE_OFFSET] == 0x0A);
    assert(nes.ram[ADDR_MUSIC_DATA_DURATION_TABLE_OFFSET] == 0x24);

    /* Test updateMusic with track 0 (already playing) vs track 0xFF */
    nes.ram[ADDR_MUSIC_TRACK] = 0;
    updateMusic(&nes);
    assert(nes.ram[ADDR_CURRENTLY_PLAYING_MUSIC_TRACK] == 1);

    nes.ram[ADDR_MUSIC_TRACK] = 0xFF;
    updateMusic(&nes);
    assert(nes.ram[ADDR_MUSIC_CHAN_CONTROL + 2] == 0);
}

void test_music_envelope_processing(void) {
    NES_State nes;
    nes_init(&nes);

    /* Square 1 envelope testing with control 1 */
    nes.ram[ADDR_MUSIC_CHAN_OFFSET] = 0;
    nes.ram[ADDR_MUSIC_CHAN_CONTROL] = 0x01; /* ctrl 1 -> table entry 0 ($EA76) */
    nes.ram[ADDR_MUSIC_CHAN_VOLUME] = 0x80;  /* duty 50% */
    nes.ram[ADDR_MUSIC_CHAN_NOTE] = 0x30;    /* normal note */
    nes.ram[ADDR_MUSIC_CHAN_VOL_FRAME_COUNTER] = 0;
    nes.ram[ADDR_MUSIC_CHAN_INHIBIT] = 0;

    /* Frame 0: first byte of LEA76 is 0x46. High nibble is 4 */
    updateMusicFrame_setChanVol(&nes, 0);
    assert(nes.ram[ADDR_MUSIC_CHAN_VOL_FRAME_COUNTER] == 1);
    assert(nes.apu_regs[REG_SQ1_VOL - 0x4000] == 0x84);

    /* Frame 1: Low nibble is 6 */
    updateMusicFrame_setChanVol(&nes, 0);
    assert(nes.ram[ADDR_MUSIC_CHAN_VOL_FRAME_COUNTER] == 2);
    assert(nes.apu_regs[REG_SQ1_VOL - 0x4000] == 0x86);

    /* Rest note: note == 2 -> mutes to 0x10 */
    nes.ram[ADDR_MUSIC_CHAN_NOTE] = 0x02;
    updateMusicFrame_setChanVol(&nes, 0);
    assert(nes.apu_regs[REG_SQ1_VOL - 0x4000] == 0x10);
    assert(nes.ram[ADDR_MUSIC_CHAN_VOL_FRAME_COUNTER] == 3);
}

void test_music_pitch_bend_modulation(void) {
    NES_State nes;
    nes_init(&nes);

    nes.ram[ADDR_CURRENTLY_PLAYING_MUSIC_TRACK] = 2; /* not track 1 */
    nes.ram[ADDR_MUSIC_CHAN_OFFSET] = 0;             /* Sq1 */
    nes.ram[ADDR_MUSIC_STAGING_SQ1_LO] = 0x50;       /* base LO = $50 */
    nes.ram[ADDR_MUSIC_CHAN_NOTE] = 0x20;
    nes.ram[ADDR_MUSIC_CHAN_INHIBIT] = 0;

    /* Pitch modulation mode 0x20: trill table cycling */
    nes.ram[ADDR_MUSIC_CHAN_CONTROL] = 0x20;
    nes.ram[ADDR_MUSIC_CHAN_LO_FRAME_COUNTER] = 0;

    /* Frame 0: loOffTrillNeg2To2Table[0] == 0 -> SQ1_LO = $50 + 0 = $50 */
    updateMusicFrame_setChanLo(&nes, 0);
    assert(nes.apu_regs[REG_SQ1_LO - 0x4000] == 0x50);
    assert(nes.ram[ADDR_MUSIC_CHAN_LO_FRAME_COUNTER] == 1);

    /* Frame 1: loOffTrillNeg2To2Table[1] == 1 -> SQ1_LO = $50 + 1 = $51 */
    updateMusicFrame_setChanLo(&nes, 0);
    assert(nes.apu_regs[REG_SQ1_LO - 0x4000] == 0x51);
    assert(nes.ram[ADDR_MUSIC_CHAN_LO_FRAME_COUNTER] == 2);

    /* Pitch modulation mode 0x60: constant -2 ($FE) -> $50 + $FE = $4E */
    nes.ram[ADDR_MUSIC_CHAN_CONTROL] = 0x60;
    updateMusicFrame_setChanLo(&nes, 0);
    assert(nes.apu_regs[REG_SQ1_LO - 0x4000] == 0x4E);

    /* Pitch modulation mode 0x40: descend table */
    nes.ram[ADDR_MUSIC_CHAN_CONTROL] = 0x40;
    nes.ram[ADDR_MUSIC_CHAN_LO_FRAME_COUNTER] = 1; /* loOffDescendToNeg11BounceToNeg9Table[1] == 0xFF (-1) */
    updateMusicFrame_setChanLo(&nes, 0);
    assert(nes.apu_regs[REG_SQ1_LO - 0x4000] == 0x4F);
}

void test_music_noise_and_dmc(void) {
    NES_State nes;
    nes_init(&nes);

    /* Test playNoise */
    nes.ram[ADDR_SOUND_EFFECT_SLOT0_PLAYING] = 0;
    playNoise(&nes, 0x00);
    assert(nes.apu_regs[REG_NOISE_VOL - 0x4000] == 0x00);
    assert(nes.apu_regs[REG_NOISE_LO - 0x4000] == 0x10);
    assert(nes.apu_regs[REG_NOISE_HI - 0x4000] == 0x01);

    /* With SFX slot 0 active -> noise write blocked */
    nes.ram[ADDR_SOUND_EFFECT_SLOT0_PLAYING] = 1;
    playNoise(&nes, 0x03);
    assert(nes.apu_regs[REG_NOISE_VOL - 0x4000] == 0x00); /* remains unchanged */

    /* Test playDmc */
    playDmc(&nes, 0x40); /* DMC0 */
    assert(nes.apu_regs[REG_DMC_LEN - 0x4000] == 0x07);
    assert(nes.apu_regs[REG_DMC_START - 0x4000] == 0x00);
    assert(nes.apu_regs[REG_DMC_FREQ - 0x4000] == 0x0E);
    assert(nes.apu_regs[REG_SND_CHN - 0x4000] == 0x1F);

    playDmc(&nes, 0x80); /* DMC1 */
    assert(nes.apu_regs[REG_DMC_LEN - 0x4000] == 0x0F);
    assert(nes.apu_regs[REG_DMC_START - 0x4000] == 0x02);
}

void test_music_bytecode_interpreter(void) {
    NES_State nes;
    nes_init(&nes);

    /* Stage custom bytecode at RAM $0300 */
    uint16_t script_addr = 0x0300;
    nes.ram[0x0300] = 0x9F; /* setControlAndVolume */
    nes.ram[0x0301] = 0x02; /* ctrl = 2 */
    nes.ram[0x0302] = 0xBF; /* vol = BF */
    nes.ram[0x0303] = 0x9E; /* setDurationOffset */
    nes.ram[0x0304] = 0x00; /* offset = 0 */
    nes.ram[0x0305] = 0x9C; /* setNoteOffset */
    nes.ram[0x0306] = 0x00; /* note offset = 0 */
    nes.ram[0x0307] = 0xB1; /* duration index 1 (0x06 frames) */
    nes.ram[0x0308] = 0x50; /* note $50 */
    nes.ram[0x0309] = 0x00; /* end of routine */

    /* Set up channel 0 to point to script */
    nes.ram[ADDR_MUSIC_DATA_CHAN_PTR_DEREF] = (uint8_t)(script_addr & 0xFF);
    nes.ram[ADDR_MUSIC_DATA_CHAN_PTR_DEREF + 1] = (uint8_t)(script_addr >> 8);
    nes.ram[ADDR_MUSIC_DATA_CHAN_INSTRUCTION_OFFSET] = 0;
    nes.ram[ADDR_MUSIC_CHAN_NOTE_DURATION_REMAINING] = 1; /* triggers fetch */

    /* Disable channels 1, 2, 3 so test focuses on channel 0 */
    nes.ram[ADDR_MUSIC_DATA_CHAN_PTR_DEREF + 3] = 0xFF;
    nes.ram[ADDR_MUSIC_DATA_CHAN_PTR_DEREF + 5] = 0xFF;
    nes.ram[ADDR_MUSIC_DATA_CHAN_PTR_DEREF + 7] = 0xFF;

    updateMusicFrame(&nes);

    assert(nes.ram[ADDR_MUSIC_CHAN_NOTE_DURATION] == 0x06);
    assert(nes.ram[ADDR_MUSIC_CHAN_NOTE_DURATION_REMAINING] == 0x06);
    assert(nes.apu_regs[REG_SQ1_VOL - 0x4000] == 0xB0);
    assert(nes.apu_regs[REG_SQ1_LO - 0x4000] == 0xBD);
    assert(nes.apu_regs[REG_SQ1_HI - 0x4000] == 0x08);
}

int test_audio_subsystem(void) {
    test_audio_apu_copies();
    test_audio_compute_method();
    test_audio_advance_frame();
    test_audio_mute();
    test_audio_pause();
    test_audio_slot1_sfx_lock_and_priority();
    test_audio_slot2_buzz();
    test_audio_slot3_donk_and_alien();
    test_audio_nmi_synchronizers();
    test_music_init_sq12_and_mute();
    test_music_load_track();
    test_music_envelope_processing();
    test_music_pitch_bend_modulation();
    test_music_noise_and_dmc();
    test_music_bytecode_interpreter();
    printf("All audio unit tests passed successfully!\n");
    return 0;
}
