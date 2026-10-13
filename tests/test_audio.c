#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "nes.h"
#include "game/audio.h"
#include "constants/hardware.h"
#include "constants/memory.h"
#include "constants/gameplay.h"

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
    printf("All audio unit tests passed successfully!\n");
    return 0;
}
