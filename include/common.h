#ifndef TDECOMP_COMMON_H
#define TDECOMP_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Standard byte / word masks */
#define LOW_BYTE(w)  ((uint8_t)((w) & 0xFF))
#define HIGH_BYTE(w) ((uint8_t)(((w) >> 8) & 0xFF))
#define MAKE_WORD(lo, hi) ((uint16_t)(((uint16_t)(hi) << 8) | (uint16_t)(lo)))

#ifdef __cplusplus
}
#endif

#endif /* TDECOMP_COMMON_H */
