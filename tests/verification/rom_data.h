#ifndef TDECOMP_ROM_DATA_H
#define TDECOMP_ROM_DATA_H

#include "common.h"
#include "nes.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Attempts to load reference ROM into nes->prg_rom from standard paths.
 * Returns true if reference ROM was successfully loaded, false otherwise.
 */
bool load_reference_rom(NES_State *nes, const char *custom_path);

/*
 * Verifies that all decompiled tables in C match the exact bytes in the PRG ROM.
 * Returns 0 on success, non-zero on failure.
 */
int test_rom_tables_match(const NES_State *nes);

#ifdef __cplusplus
}
#endif

#endif /* TDECOMP_ROM_DATA_H */
