#ifndef TDECOMP_NATIVE_RENDERER_H
#define TDECOMP_NATIVE_RENDERER_H

#include "game_adapter.h"
#include "rom_assets.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize raylib renderer state with extracted ROM assets */
void renderer_init(const RomAssets *assets);

/* Rebuild/load tile textures after runtime ROM import */
void renderer_load_rom_assets(const RomAssets *assets);

/* Render a complete frame representing current GameAdapter state */
void renderer_draw(const GameAdapter *adapter, const char *rom_error_msg);

/* Clean up renderer resources */
void renderer_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TDECOMP_NATIVE_RENDERER_H */
