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

// TODO : find a way to make clang-format inline this macro
#define CLR_RGB(r, g, b)                                 \
	((struct termclr){                                   \
		.fmt = CELL_CLRFMT_RGB, .value.rgb = {r, g, b} \
    })
#define CLR_ID(c) ((struct termclr){.fmt = CELL_CLRFMT_ID, .value.id = c})
// see https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797#8-16-colors
#define CLR_CODE(c) ((struct termclr){.fmt = CELL_CLRFMT_CODE, .value.code = c + 30})

enum ENUMTYPE(clrfmt, unsigned char) {
	CELL_CLRFMT_CODE = 1,
	CELL_CLRFMT_RGB,
	CELL_CLRFMT_ID, // 3 bits max 0b100
};

union termclrval {
	unsigned char code;
	struct rgb rgb;
	uint8_t id;
};

struct termclr {
	enum clrfmt fmt;
	union termclrval value;
};

// Must fit in 1 byte! (8 bits max)
struct cellmeta {
	enum clrfmt fg_clrfmt : 3;
	enum clrfmt bg_clrfmt : 3;
	bool is_visible : 1;
};

enum ENUMTYPE(clrcode, unsigned char) {
	CLRCODE_BLACK,
	CLRCODE_RED,
	CLRCODE_GREEN,
	CLRCODE_YELLOW,
	CLRCODE_BLUE,
	CLRCODE_MAGENTA,
	CLRCODE_CYAN,
	CLRCODE_WHITE,
	CLRCODE_DEF = 9,
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
	struct termclr def_bg_clr;
	struct termclr def_fg_clr;

	// cell buffers
	char32_t* chars;
	struct cellmeta* cellmetas;
	union termclrval* bg_clrs;
	union termclrval* fg_clrs;
};

struct scrstr_bufview {
	char* buf;
	size_t size;
};

struct _strbuf {
	size_t pagesize;
	size_t bufsize;
	size_t cursor;
	char* buf;
};

struct _scr_arena {
	size_t pagesize;          // Avoids us from having to compute the page size again when de-allocating the arena
	struct termsize termsize; // Same
	termstatefl termflags;
	struct _strbuf* strbuf;
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
screen* newscr(struct termclr bg_clr, struct termclr fg_clr, termstatefl scrflags);
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

/** Sets the given bg color at (x, y) */
enum escerr ssetbgclr(screen* restrict scr, struct termclr clr, uint16_t x, uint16_t y);
/** Sets the given fg color at (x, y) */
enum escerr ssetfgclr(screen* restrict scr, struct termclr clr, uint16_t x, uint16_t y);
/** Sets the given fg and bg colors at (x, y) */
enum escerr ssetclrpair(screen* restrict scr, struct termclr bgclr, struct termclr fgclr, uint16_t x, uint16_t y);

/** Calls ssetgphm for each grapheme in the string based from the fitst one */
enum escerr saddstr(screen* restrict scr, const char* str, size_t strlen, uint16_t x, uint16_t y);

extern screen* stdscr;
// IDK, see : https://stackoverflow.com/questions/76365216/why-are-stderr-stdin-stdout-defined-as-macros
#define stdscr stdscr

#define DEF_SCR_BGCLR ((struct termclr){.fmt = CELL_CLRFMT_CODE, .value.code = CLRCODE_BLACK})
#define DEF_SCR_FGCLR ((struct termclr){.fmt = CELL_CLRFMT_CODE, .value.code = CLRCODE_DEF})

#define getpbuf()       sgetpbuf(stdscr)
#define getvbuf()       sgetvbuf(stdscr)
#define getstrbufview() sgetstrbufview(stdscr)
#define refresh()       srefresh(stdscr)
#define corderr(...)    scorderr(stdscr, __VA_ARGS__)
#define setgphm(...)    ssetgphm(stdscr, __VA_ARGS__)
#define cordtoidx(...)  scordtoidx(stdscr, __VA_ARGS__)
#define setfgclr(...)   ssetfgclr(stdscr, __VA_ARGS__)
#define setbgclr(...)   ssetbgclr(stdscr, __VA_ARGS__)
#define setclrpair(...) ssetclrpair(stdscr, __VA_ARGS__)
#define addstr(...)     saddstr(stdscr, __VA_ARGS__)

