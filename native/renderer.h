#ifndef TDECOMP_NATIVE_RENDERER_H
#define TDECOMP_NATIVE_RENDERER_H

#include "game_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize raylib renderer state */
void renderer_init(void);

/* Render a complete frame representing current GameAdapter state */
void renderer_draw(const GameAdapter *adapter);

/* Clean up renderer resources */
void renderer_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TDECOMP_NATIVE_RENDERER_H */
