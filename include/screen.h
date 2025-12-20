#pragma once

#include <stdbool.h>
#include <stddef.h>
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

enum ENUMTYPE(cellflags, unsigned char) {
	CELL_VISIBLE        = 0x1,
	CELL_BG_CLRFMT_CODE = 0x2,
	CELL_FG_CLRFMT_CODE = 0x4,
	CELL_BG_CLRFMT_RGB  = 0x8,
	CELL_FG_CLRFMT_RGB  = 0x10,
	CELL_BG_CLRFMT_ID   = 0x20,
	CELL_FG_CLRFMT_ID   = 0x40,
};

enum ENUMTYPE(termclrcode, unsigned char) {
	BLACK,
	RED,
	GREEN,
	YELLOW,
	BLUE,
	MAGENTA,
	CYAN,
	WHITE,
	DEF_CLRCODE,
};

/**
 * Conceptually, a "screen buffer" represents the current/desired visual state of a terminal. Do not mix it up with a window.
 * Windows are composite by nature, screens are not and are fully opaque, plain, like a canvas. It may also hold terminal flags if
 * you want to. Explanation for the choice of types:
 * - `chars` uses 32 bit wide characters because it is the largest size for a UTF-8 grapheme (so you could say UTF-32).
 *   Using 32 bits is a sacrifice we have to make to be able to address any "visible" character (grapheme) based
 *   on an index/uint16_tinates on our terminal's grid. The additional unused space is only wasteful in memory,
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
	union termclr bg_clr;
	union termclr fg_clr;

	// cell buffers
	char32_t* chars;
	unsigned char* cellflags;
	union termclr* bg_clrs;
	union termclr* fg_clrs;
};

struct scrstr_bufview {
	char* buf;
	size_t size;
};

struct _scrstrbuf {
	size_t pagesize;
	size_t bufsize;
	size_t cursor;
	char* buf;
};

struct _scr_arena {
	size_t pagesize;          // Avoids us from having to compute the page size again when de-allocating the arena
	struct termsize termsize; // Same
	termstatefl termflags;
	struct _scrstrbuf* strbuf;
	struct scrbuf* pbuf;
	struct scrbuf* vbuf;
};

typedef struct _scr_arena screen;

/** Sets variables relative to the internal memory allocations of the library,
 * where scrstr refers to the screen string buffer, that is the buffer that will contain the string
 * which will be written to stdout to represent whatever screen you want to see with srefresh/refresh for stdscr (default) */
void scrmemargs(size_t scrstr_bufsize, float scrstr_growth_rate);

/** Returns the pointer to the newly created screen if succesful, NULL / nullptr otherwise.
 * scrflags holds the same flags as termflags with some additional flags for screens exclusively. */
screen* newscr(union termclr bg_clr, union termclr fg_clr, termstatefl scrflags);
/** De-allocate the given screen. Returns false (0) if successful and sets scr to NULL/nullptr, true (1) otherwise */
bool freescr(screen* scr);

/** Draws the given screen with the smallest possible sequence based on previous states if available */
bool srefresh(screen* scr);

/** Returns a pointer to the physical buffer of the given screen. */
static inline struct scrbuf* sgetpbuf(const screen* scr)
{
	return scr->pbuf;
}
/** Returns a pointer to the virtual buffer of the given screen if the USE_VSCR flag was used with newscr,
 * NULL/nullptr otherwise. */
static inline struct scrbuf* sgetvbuf(const screen* scr)
{
	return scr->vbuf;
}
static inline struct scrstr_bufview sgetstrbufview(const screen* scr)
{
	return (struct scrstr_bufview){.buf = scr->strbuf->buf, .size = scr->strbuf->cursor};
}
/** Returns the index in a screen buffer of the size of scr, -1 if x or y is out of bounds.
 * Use scorderr(x, y) to get more details. */
long scordtoidx(const screen* scr, uint16_t x, uint16_t y);
/** Returns flags of cordbounderrs given x and y. */
enum escerr sgetcorderr(const screen* scr, uint16_t x, uint16_t y);
/** Sets UTF32 character c32 in physical scrbuf of scr at (x, y)
 * Returns flags of cordbounderrs given x and y. */
enum escerr ssetgphm(screen* restrict scr, const char* gphm, uint16_t x, uint16_t y);
/** Sets the given color at (x, y) */
enum escerr ssetclr(screen* restrict scr, union termclr clr, unsigned char cellclrflag, uint16_t x, uint16_t y);

/** Calls ssetgphm for each grapheme in the string based from the fitst one */
enum escerr saddstr(screen* restrict scr, const char* str, size_t strlen, uint16_t x, uint16_t y);

extern screen* stdscr;
// IDK, see : https://stackoverflow.com/questions/76365216/why-are-stderr-stdin-stdout-defined-as-macros
#define stdscr stdscr

#define DEF_SCR_BGCLR ((union termclr){.code = BLACK})
#define DEF_SCR_FGCLR ((union termclr){.code = DEF_CLRCODE})

#define getpbuf()       sgetpbuf(stdscr)
#define getvbuf()       sgetvbuf(stdscr)
#define getstrbufview() sgetstrbufview(stdscr)
#define refresh()       srefresh(stdscr)
#define corderr(...)    scorderr(stdscr, __VA_ARGS__)
#define setgphm(...)    ssetgphm(stdscr, __VA_ARGS__)
#define cordtoidx(...)  scordtoidx(stdscr, __VA_ARGS__)
#define setclr(...)     ssetclr(stdscr, __VA_ARGS__)
#define addstr(...)     saddstr(stdscr, __VA_ARGS__)

