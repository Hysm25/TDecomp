#include "audio.h"
#include <raylib.h>
#include <stdlib.h>
#include <math.h>

#define SAMPLE_RATE 44100

static bool s_audio_inited = false;
static bool s_muted = false;

static Sound s_snd_move;
static Sound s_snd_rotate;
static Sound s_snd_lock;
static Sound s_snd_line_clear;
static Sound s_snd_tetris;
static Sound s_snd_level_up;
static Sound s_snd_game_over;

static Sound make_square_sound(float freq, float duration, float vol) {
    unsigned int frames = (unsigned int)(SAMPLE_RATE * duration);
    if (frames == 0) frames = 1;

    short *data = (short *)malloc(frames * sizeof(short));
    if (!data) return (Sound){0};

    float phase = 0.0f;
    float phase_step = freq / (float)SAMPLE_RATE;
    float max_amp = 32767.0f * vol;

    for (unsigned int i = 0; i < frames; i++) {
        float env = 1.0f - ((float)i / (float)frames); /* linear decay */
        float sample = (phase < 0.5f ? 1.0f : -1.0f) * max_amp * env;
        data[i] = (short)sample;

        phase += phase_step;
        if (phase >= 1.0f) phase -= 1.0f;
    }

    Wave wave = {
        .frameCount = frames,
        .sampleRate = SAMPLE_RATE,
        .sampleSize = 16,
        .channels = 1,
        .data = data
    };

    Sound snd = LoadSoundFromWave(wave);
    free(data);
    return snd;
}

static Sound make_chirp_sound(float start_f, float end_f, float duration, float vol) {
    unsigned int frames = (unsigned int)(SAMPLE_RATE * duration);
    if (frames == 0) frames = 1;

    short *data = (short *)malloc(frames * sizeof(short));
    if (!data) return (Sound){0};

    float phase = 0.0f;
    float max_amp = 32767.0f * vol;

    for (unsigned int i = 0; i < frames; i++) {
        float t = (float)i / (float)frames;
        float freq = start_f + (end_f - start_f) * t;
        float phase_step = freq / (float)SAMPLE_RATE;
        float env = 1.0f - t;

        float sample = (phase < 0.5f ? 1.0f : -1.0f) * max_amp * env;
        data[i] = (short)sample;

        phase += phase_step;
        if (phase >= 1.0f) phase -= 1.0f;
    }

    Wave wave = {
        .frameCount = frames,
        .sampleRate = SAMPLE_RATE,
        .sampleSize = 16,
        .channels = 1,
        .data = data
    };

    Sound snd = LoadSoundFromWave(wave);
    free(data);
    return snd;
}

static Sound make_arpeggio_sound(const float *freqs, int count, float note_dur, float vol) {
    unsigned int frames_per_note = (unsigned int)(SAMPLE_RATE * note_dur);
    unsigned int total_frames = frames_per_note * (unsigned int)count;
    if (total_frames == 0) total_frames = 1;

    short *data = (short *)malloc(total_frames * sizeof(short));
    if (!data) return (Sound){0};

    float max_amp = 32767.0f * vol;

    for (int n = 0; n < count; n++) {
        float freq = freqs[n];
        float phase = 0.0f;
        float phase_step = freq / (float)SAMPLE_RATE;
        unsigned int start_idx = (unsigned int)n * frames_per_note;

        for (unsigned int i = 0; i < frames_per_note; i++) {
            float env = 1.0f - ((float)i / (float)frames_per_note) * 0.7f;
            float sample = (phase < 0.5f ? 1.0f : -1.0f) * max_amp * env;
            data[start_idx + i] = (short)sample;

            phase += phase_step;
            if (phase >= 1.0f) phase -= 1.0f;
        }
    }

    Wave wave = {
        .frameCount = total_frames,
        .sampleRate = SAMPLE_RATE,
        .sampleSize = 16,
        .channels = 1,
        .data = data
    };

    Sound snd = LoadSoundFromWave(wave);
    free(data);
    return snd;
}

void native_audio_init(bool enabled) {
    if (!enabled) {
        s_audio_inited = false;
        return;
    }

    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        s_audio_inited = false;
        return;
    }

    s_audio_inited = true;
    s_muted = false;

    /* Generate procedural retro sound effects */
    s_snd_move       = make_square_sound(320.0f, 0.025f, 0.25f);
    s_snd_rotate     = make_chirp_sound(480.0f, 720.0f, 0.040f, 0.28f);
    s_snd_lock       = make_chirp_sound(160.0f, 90.0f, 0.070f, 0.35f);

    const float line_freqs[2] = {523.25f, 659.25f}; /* C5 -> E5 */
    s_snd_line_clear = make_arpeggio_sound(line_freqs, 2, 0.080f, 0.35f);

    const float tetris_freqs[4] = {523.25f, 659.25f, 783.99f, 1046.50f}; /* C5 -> E5 -> G5 -> C6 */
    s_snd_tetris     = make_arpeggio_sound(tetris_freqs, 4, 0.075f, 0.40f);

    const float lvl_freqs[3] = {440.0f, 659.25f, 880.0f}; /* A4 -> E5 -> A5 */
    s_snd_level_up   = make_arpeggio_sound(lvl_freqs, 3, 0.100f, 0.40f);

    const float go_freqs[3] = {329.63f, 246.94f, 164.81f}; /* E4 -> B3 -> E3 */
    s_snd_game_over  = make_arpeggio_sound(go_freqs, 3, 0.150f, 0.35f);
}

void native_audio_play(AudioEventType type) {
    if (!s_audio_inited || s_muted) return;

    switch (type) {
        case AUDIO_EVENT_MOVE:
            PlaySound(s_snd_move);
            break;
        case AUDIO_EVENT_ROTATE:
            PlaySound(s_snd_rotate);
            break;
        case AUDIO_EVENT_LOCK:
            PlaySound(s_snd_lock);
            break;
        case AUDIO_EVENT_LINE_CLEAR:
            PlaySound(s_snd_line_clear);
            break;
        case AUDIO_EVENT_TETRIS:
            PlaySound(s_snd_tetris);
            break;
        case AUDIO_EVENT_LEVEL_UP:
            PlaySound(s_snd_level_up);
            break;
        case AUDIO_EVENT_GAME_OVER:
            PlaySound(s_snd_game_over);
            break;
        case AUDIO_EVENT_NONE:
        default:
            break;
    }
}

void native_audio_toggle_mute(void) {
    s_muted = !s_muted;
}

bool native_audio_is_muted(void) {
    return s_muted;
}

void native_audio_shutdown(void) {
    if (!s_audio_inited) return;

    UnloadSound(s_snd_move);
    UnloadSound(s_snd_rotate);
    UnloadSound(s_snd_lock);
    UnloadSound(s_snd_line_clear);
    UnloadSound(s_snd_tetris);
    UnloadSound(s_snd_level_up);
    UnloadSound(s_snd_game_over);

    CloseAudioDevice();
    s_audio_inited = false;
}
