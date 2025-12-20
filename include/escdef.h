#pragma once

#include <stdint.h>
#if __STDC_VERSION__ >= 202311L
#include <uchar.h>
#endif

#define MAX_GPHM_CPTS 4
#define MAX_GPHM_WIDTH UINT32_WIDTH

// Used frequently when manipulating coordinates
#define MIN(x, y)          (x < y ? x : y)
#define MAX(x, y)          (x > y ? x : y)
#define CLAMP(x, min, max) (x < min ? min : x > max ? max : x)

// Control Sequence Introducer
#define CSI "\x1b["

/* Portable typed enums from C23.
 * expands to a regular enum if __STDC_VERSION__ < 202311L) */
#if __STDC_VERSION__ >= 202311L
// clang-format off
#define ENUMTYPE(name, type) name : type
// clang-format on
#else
#define ENUMTYPE(name, type) name
#endif

// #if __STDC_VERSION__ >= 202311L
// #define GPHM(str) (const char8_t*)((const char8_t (*)[4]) u8##str)
// #else
// #define GPHM(str) ((const char (*)[sizeof(str) - 1]) u8##str)
// #endif

/*
 * To represent coordinates/length in a terminal,
 * 8 bits is too small, 16 is too big, so 16 bits is enough.
 * 16 bits is ENOUGH.
 * I genuinely do not beleive that you have 65536p monitor with your terminal so zoomed out each cell is one pixel wide.
 * If you do however, call me.
 * Therefore, the library will handle anything pixel/cell related (coordinates, lengths) with uint16_t.
 */

/** holds bitflags if positive, doesn't if negative */
typedef int termstatefl;

enum ENUMTYPE(termflags, termstatefl) {
	TERM_ALTBUF      = 0x1,
	TERM_HIDE_CURSOR = 0x2,
	TERM_NO_ECHO     = 0x4,
};

/* === scrflags MUST NEVER OVERLAP WITH termflags!!! === */

// IMPORTANT ! : the only requirement for this to work is for these flags
// to be bigger than at least the biggest (last) flag of `enum termflags`

/* Extends `enum termflags` */
enum ENUMTYPE(scrflags, termstatefl) {
	SCREEN_HOLD_TERMFLAGS = 0x8,
	SCREEN_USE_VIRTUAL    = 0x10,
};


enum ENUMTYPE(escerr, unsigned char) {
	ESC_OK = 0,
	ESC_ERR_COORD_X,
	ESC_ERR_COORD_Y,
	ESC_ERR_UTF8,
	ESC_ERR_GPHM,
};
