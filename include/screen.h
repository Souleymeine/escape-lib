#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

#include "escdef.h"
#include "termsize.h"

struct rgb {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

union termclr {
	unsigned char code;
	struct rgb rgb;
	uint8_t id;
};

enum cellflags : unsigned char {
	VISIBLE     = 0x1,
	BG_CLR_CODE = 0x2,
	FG_CLR_CODE = 0x4,
	BG_CLR_RGB  = 0x8,
	FG_CLR_RGB  = 0x10,
	BG_CLR_ID   = 0x20,
	FG_CLR_ID   = 0x40,
};

/**
 * Conceptually, a "screen buffer" represents the current/desired visual state of a terminal. Do not mix it up with a window.
 * Windows are composite by nature, screens are not and are fully opaque, plain, like a canvas. It may also hold terminal flags if
 * you want to. Explanation for the choice of types:
 * - `chars` uses 32 bit wide characters because it is the largest size for a UTF-8 grapheme (so you could say UTF-32).
 *   Using 32 bits is a sacrifice we have to make to be able to address any "visible" character (grapheme) based
 *   on an index/coordinates on our terminal's grid. The additional unused space is only wasteful in memory,
 *   the screen will get translated to be written to stdout anyway + it has the VITAL advantage of not requiring us
 *   to dynamically reallocate the buffer when it's full, because it cannot be full until every cell contains a grapheme.
 * - `bg_clrs` and `fg_clrs` use arrays of unions which represent a general "color" (clr). It is very practical, since a color
 *   could be anything between an rgb struct (three 8 bit unsigned integers) or a single character/uint8 depending on the format.
 *   The union thus allows us to allocate enough memory for those 3 formats by being wide enough for the biggest (rgb) and access
 *   colors in a single array with whatever data type we want.
 * NOTE : The size of `chars`, `bg_clrs` and `fg_clrs` must always be the number of cells in the current terminal.
 *
 * Have fun
 *
 *		Souleymeine
 */
struct scrbuf {
	// properties
	flags termflags;
	union termclr bg_clr;
	union termclr fg_clr;

	// cell buffers
	char32_t* chars;
	unsigned char* cellflags;
	union termclr* bg_clrs;
	union termclr* fg_clrs;
};

struct _scr_arena {
	size_t _pagesize;          // Avoids us from having to compute the page size again when de-allocating the arena
	struct termsize _termsize; // Same
	struct scrbuf* _pbuf;
	struct scrbuf* _vbuf;
};

typedef struct _scr_arena screen;

/** Returns a pointer to the physical buffer of the given screen. */
static inline struct scrbuf* get_pbuf(screen* scr)
{
	return scr->_pbuf;
}
/**
 * Returns a pointer to the virtual buffer of the given screen if the USE_VSCR flag was used with newscr,
 * NULL/nullptr otherwise.
 */
static inline struct scrbuf* get_vbuf(screen* scr)
{
	return scr->_vbuf;
}

enum clrcode : unsigned char {
	BLACK,
	RED,
	GREEN,
	YELLOW,
	BLUE,
	MAGENTA,
	CYAN,
	WHITE,
	DEFAULT_CLRCODE,
};

#define DEF_SCR_BGCLR ((union termclr){.code = BLACK})
#define DEF_SCR_FGCLR ((union termclr){.code = DEFAULT_CLRCODE})

extern screen* stdscr;
// IDK, see : https://stackoverflow.com/questions/76365216/why-are-stderr-stdin-stdout-defined-as-macros
#define stdscr stdscr

/**
 * Returns the pointer to the newly created screen if succesful, NULL / nullptr otherwise.
 * scrflags holds the same flags as termflags with some additional flags for screens exclusively.
 */
screen* newscr(union termclr bg_clr, union termclr fg_clr, flags scrflags);
/** De-allocate the given screen. Returns false (0) if successful and sets scr to NULL/nullptr, true (1) otherwise */
bool freescr(screen* scr);
/** Refresh the given screen */
void srefresh(const screen* scr);
/** Refresh stdscr */
static inline void refresh()
{
	srefresh(stdscr);
}

