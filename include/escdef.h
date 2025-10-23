#pragma once

#include <stdint.h>


/*
 * 8 bits is too small, 16 is too big, so 16 bits is enough.
 * 16 bits is ENOUGH.
 * I genuinely do not beleive that you have 65536p monitor with your terminal so zoomed out each cell is one pixel wide.
 * If you do however, call me.
 */

/**
 * Represents a coordinates/length in the terminal, or anyting related to coordinates/cells
 */
typedef uint16_t coord;

typedef unsigned int flags;

enum termflags : flags {
	ALTBUF      = 0x1,
	HIDE_CURSOR = 0x2,
	NO_ECHO     = 0x4,
};

/* === scrflags MUST NEVER OVERLAP WITH termflags!!! === */

// IMPORTANT ! : the only requirement for this to work is for these flags
// to be bigger than at least the biggest (last) flag of `enum termflags`

/* Extends `enum termflags` */
enum scrflags : flags {
	HOLD_TERMFLAGS = 0x8,
	USE_VSCR       = 0x10,
};

