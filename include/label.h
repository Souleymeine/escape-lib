#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * A good reference in terms of colore codes/styles :
 * https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797
 */

enum txtstyle_flags : unsigned char {
	BOLD          = 0x1,
	DIM           = 0x2,
	ITALIC        = 0x4,
	UNDERLINE     = 0x8,
	BLINKING      = 0x10,
	INVERSE       = 0x20,
	HIDDEN        = 0x40,
	STRIKETHROUGH = 0x80,
};

enum color_code : char {
	BLACK,
	RED,
	GREEN,
	YELLOW,
	BLUE,
	MAGENTA,
	CYAN,
	WHITE,
	DEFAULT_COLOR,
};

enum hor_align : unsigned char {
	LEFT,
	CENTER,
	RIGHT,
};

enum vir_align : unsigned char {
	TOP,
	MIDDLE,
	BOTTOM,
};

enum hor_offcenter : char { PREF_LEFT = -1, PREF_RIGHT = 1 };
enum vir_offcenter : char { PREF_TOP = -1, PREF_BOTTOM = 1 };

/**
 * Prints a label in the terminal.
 * Returns -1 if the give txt is not a valid utf8 string
 */
int print_8bit_label_gradient(const char* txt, size_t txtlen, unsigned char txtstyle, uint8_t clr_id1, uint8_t clr_id2,
                              enum hor_offcenter hor_offcenter, enum vir_offcenter vir_offcenter);

