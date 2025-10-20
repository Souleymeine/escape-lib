#pragma once

// Avoids replacing the type everywhere when required
#define FLAG_T unsigned int

enum termflags : FLAG_T {
	ALTBUF      = 0x1,
	HIDE_CURSOR = 0x2,
	NO_ECHO     = 0x4,
};

/* === scrflags MUST NEVER OVERLAP WITH termflags!!! === */

// IMPORTANT ! : the only requirement for this to work is for these flags
// to be bigger than at least the biggest (last) flag of `enum termflags`

/* Extends `enum termflags` */
enum scrflags : FLAG_T {
	HOLD_TERMFLAGS = 0x8,
	USE_VSCR       = 0x10,
};

