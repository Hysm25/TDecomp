#ifndef TDECOMP_NATIVE_AUDIO_H
#define TDECOMP_NATIVE_AUDIO_H

#include "game_adapter.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize raylib procedural audio engine */
void native_audio_init(bool enabled);

/* Play retro procedural sound effect based on event */
void native_audio_play(AudioEventType type);

/* Toggle sound mute */
void native_audio_toggle_mute(void);

/* Check if audio is currently muted */
bool native_audio_is_muted(void);

/* Clean up audio devices and wave buffers */
void native_audio_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TDECOMP_NATIVE_AUDIO_H */
