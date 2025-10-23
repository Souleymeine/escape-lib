#pragma once

#include <stdint.h>

// Used frequently when manipulating coordinates
#define MIN(x, y)          (x < y ? x : y)
#define MAX(x, y)          (x > y ? x : y)
#define CLAMP(x, min, max) (x < min ? min : x > max ? max : x)

/* Portable typed enums from C23.
 * expands to a regular enum if __STDC_VERSION__ < 202311L) */
#if __STDC_VERSION__ >= 202311L
// clang-format off
#define ENUMTYPE(name, type) name : type
// clang-format on
#else
#define ENUMTYPE(name, type) name
#endif

/*
 * 8 bits is too small, 16 is too big, so 16 bits is enough.
 * 16 bits is ENOUGH.
 * I genuinely do not beleive that you have 65536p monitor with your terminal so zoomed out each cell is one pixel wide.
 * If you do however, call me.
 */

/** Represents a coordinates/length in the terminal, or anyting related to coordinates/cells. */
typedef uint16_t coord;
typedef unsigned int termstateflag;
/** Type representing a value containg flags relative to coordinate-releated errors with enum cordbounderrs. */
typedef unsigned char errflcord;

enum ENUMTYPE(termflags, termstateflag) {
	ALTBUF      = 0x1,
	HIDE_CURSOR = 0x2,
	NO_ECHO     = 0x4,
};

/* === scrflags MUST NEVER OVERLAP WITH termflags!!! === */

// IMPORTANT ! : the only requirement for this to work is for these flags
// to be bigger than at least the biggest (last) flag of `enum termflags`

/* Extends `enum termflags` */
enum ENUMTYPE(scrflags, termstateflag) {
	HOLD_TERMFLAGS = 0x8,
	USE_VSCR       = 0x10,
};

