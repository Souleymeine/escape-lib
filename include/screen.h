#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <uchar.h>

#include "terminal.h"
#include "termsize.h"

struct rgb {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

// TODO : find a way to make clang-format inline this macro
#define CLR_RGB(r, g, b)                          \
	(struct termclr)                              \
	{                                             \
		.tag = CLRTAG_RGB, .val.rgb = { r, g, b } \
	}
#define CLR_ID(c) ((struct termclr){.tag = CLRTAG_ID, .val.id = c})
// see https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797#8-16-colors
#define CLR_CODE(c) ((struct termclr){.tag = CLRTAG_CODE, .val.code = c + 30})

enum clrtag {
	CLRTAG_CODE = 1,
	CLRTAG_RGB,
	CLRTAG_ID, // 2 bits max 0b11
};

union clrval {
	uint8_t code;
	uint8_t id;
	struct rgb rgb;
};

struct termclr {
	enum clrtag tag;
	union clrval val;
};

enum clrcode {
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

#define DEF_SCR_BGCLR ((struct termclr){.tag = CLRTAG_CODE, .val.code = CLRCODE_BLACK})
#define DEF_SCR_FGCLR ((struct termclr){.tag = CLRTAG_CODE, .val.code = CLRCODE_DEF})

// TODO : convert to regular bit flags with a few helper functions
// (bitfields don't have a well defined mem layout unfortunately)
struct tagchar { // Must fit in 32 bits
	bool visible : 1;
	enum clrtag bgtag : 2;
	enum clrtag fgtag : 2;
	char32_t c : 21; // utf8 can't go past 2^21, 11 bits remain always unused even in utf32
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
	struct tagchar* tagschars; // 32bit bitfields
	union clrval* bg_clrs;
	union clrval* fg_clrs;
};

struct strbufview {
	char8_t* buf;
	size_t size;
};

struct _strbuf {
	size_t pagesize;
	size_t bufsize;
	size_t cursor;
	char8_t* buf;
};

struct _scr_arena {
	bool refreshed;           // true if srefresh was called at least once with this screen
	size_t pagesize;          // Avoids us from having to compute the page size again when de-allocating the arena
	struct termsize termsize; // Same
	termstatefl termflags;

	struct _strbuf* strbuf;
	struct scrbuf* pbuf;
	struct scrbuf* vbuf;
};

typedef struct _scr_arena screen;

extern screen* stdscr;
// IDK, see : https://stackoverflow.com/questions/76365216/why-are-stderr-stdin-stdout-defined-as-macros
#define stdscr stdscr


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
bool srefresh(screen* scr, bool clear);
static inline bool refresh(bool clear) { return srefresh(stdscr, clear); }

enum escerr sidxtocord(const screen* scr, size_t i, uint16_t* x, uint16_t* y);
static inline enum escerr idxtocord(size_t i, uint16_t* x, uint16_t* y) { return sidxtocord(stdscr, i, x, y); }

/** Returns a pointer to the physical buffer of the given screen. */
static inline struct scrbuf* sgetpbuf(const screen* scr) { return scr->pbuf; }
static inline struct scrbuf* getpbuf() { return sgetpbuf(stdscr); }

/** Returns a pointer to the virtual buffer of the given screen if the USE_VSCR flag was used with newscr,
 * NULL/nullptr otherwise. */
static inline struct scrbuf* sgetvbuf(const screen* scr) { return scr->vbuf; }
static inline struct scrbuf* getvbuf() { return sgetvbuf(stdscr); }

static inline struct strbufview sgetstrbufview(const screen* scr)
{
	return (struct strbufview){.buf = scr->strbuf->buf, .size = scr->strbuf->cursor};
}
static inline struct strbufview getstrbufview() { return sgetstrbufview(stdscr); }
/** Returns the index in a screen buffer of the size of scr, -1 if x or y is out of bounds.
 * Use scorderr(x, y) to get more details. */
long scordtoidx(const screen* scr, uint16_t x, uint16_t y);
static inline long cordtoidx(uint16_t x, uint16_t y) { return scordtoidx(stdscr, x, y); }

/** Returns flags of cordbounderrs given x and y. */
enum escerr sgetcorderr(const screen* scr, uint16_t x, uint16_t y);
static inline enum escerr getcorderr(uint16_t x, uint16_t y) { return sgetcorderr(stdscr, x, y); }

/** Sets UTF32 character c32 in physical scrbuf of scr at (x, y)
 * Returns flags of cordbounderrs given x and y. */
enum escerr ssetcp(screen* scr, char32_t c, uint16_t x, uint16_t y);
static inline enum escerr setcp(char32_t c, uint16_t x, uint16_t y) { return ssetcp(stdscr, c, x, y); }

/** Sets the given bg color at (x, y) */
enum escerr ssetbgclr(screen* scr, struct termclr clr, uint16_t x, uint16_t y);
static inline enum escerr setbgclr(struct termclr clr, uint16_t x, uint16_t y) { return ssetbgclr(stdscr, clr, x, y); }

/** Sets the given fg color at (x, y) */
enum escerr ssetfgclr(screen* scr, struct termclr clr, uint16_t x, uint16_t y);
static inline enum escerr setfgclr(struct termclr clr, uint16_t x, uint16_t y) { return ssetfgclr(stdscr, clr, x, y); }

/** Sets the given fg and bg colors at (x, y) */
enum escerr ssetclrpair(screen* scr, struct termclr bgclr, struct termclr fgclr, uint16_t x, uint16_t y);
static inline enum escerr setclrpair(struct termclr bgclr, struct termclr fgclr, uint16_t x, uint16_t y) { return ssetclrpair(stdscr, bgclr, fgclr, x, y); }

enum escerr ssetvis(const screen* scr, bool visible, uint16_t x, uint16_t y);
static inline enum escerr setvis(bool visible, uint16_t x, uint16_t y) { return ssetvis(stdscr, visible, x, y); }

enum escerr stogglevis(const screen* scr, uint16_t x, uint16_t y);
static inline enum escerr togglevis(uint16_t x, uint16_t y) { return stogglevis(stdscr, x, y); }

/** Calls ssetgphm for each grapheme in the string based from the fitst one */
void saddstr(screen* scr, const char32_t* str32, size_t strlen, uint16_t x, uint16_t y);
static inline void addstr(const char32_t* str32, size_t strlen, uint16_t x, uint16_t y) { saddstr(stdscr, str32, strlen, x, y); }

