#pragma once

#include <stddef.h>
#include <uchar.h>

#include "core.h"

struct rgb {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

enum clrtag {
	CLRTAG_CODE = 0b01,
	CLRTAG_RGB  = 0b10,
	CLRTAG_ID   = 0b11,
};
union clrval {
	uint8_t code;
	uint8_t id;
	struct rgb rgb;
};
struct clr {
	enum clrtag tag;
	union clrval val;
};

#define CLR_RGB(r, g, b) (struct termclr){.tag = CLRTAG_RGB,  .val.rgb = { r, g, b }}
#define CLR_ID(c)        (struct termclr){.tag = CLRTAG_ID,   .val.id = c}
#define CLR_CODE(c)      (struct termclr){.tag = CLRTAG_CODE, .val.code = c + 30}

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

struct tagchar { // Must fit in 32 bits
	_BitInt(21) c     : 21; // utf8 can't go past 2^21, 11 bits remain always unused even in utf32
	enum clrtag bgtag : 2;
	enum clrtag fgtag : 2;
	bool visible      : 1;
};

// properties
extern struct termclr def_bg_clr;
extern struct termclr def_fg_clr;

// cell buffers
extern struct tagchar tagschars[]; // 32bit bitfields
extern union clrval   bg_clrs[];
extern union clrval   fg_clrs[];

size_t cursor;
char8_t* buf;

struct termsize termsize;
unsigned termflags;

struct scrbuf* pbuf;
struct scrbuf* vbuf;

/** Sets variables relative to the internal memory allocations of the library,
 * where scrstr refers to the screen string buffer, that is the buffer that will contain the string
 * which will be written to stdout to represent whatever screen you want to see with srefresh/refresh for stdscr (default) */
void scrmemargs(size_t scrstr_bufsize, float scrstr_growth_rate);

/** Draws the given screen with the smallest possible sequence based on previous states if available */
bool refresh(bool clear);

enum escerr idxtocord(size_t i, uint16_t* x, uint16_t* y);
/** Returns the index in a screen buffer of the size of scr, -1 if x or y is out of bounds.
 * Use scorderr(x, y) to get more details. */
long cordtoidx(uint16_t x, uint16_t y);

/** Sets UTF32 character c32 in physical scrbuf of scr at (x, y)
 * Returns flags of cordbounderrs given x and y. */
enum escerr setcp(char32_t c, uint16_t x, uint16_t y);

/** Sets the given bg color at (x, y) */
enum escerr setbgclr(struct termclr clr, uint16_t x, uint16_t y);

/** Sets the given fg color at (x, y) */
enum escerr setfgclr(struct termclr clr, uint16_t x, uint16_t y);

/** Sets the given fg and bg colors at (x, y) */
enum escerr setclrpair(struct termclr bgclr, struct termclr fgclr, uint16_t x, uint16_t y);

enum escerr setvis(bool visible, uint16_t x, uint16_t y);

enum escerr togglevis(uint16_t x, uint16_t y);


#define UTF8(s) (char8_t*)s

#define MAX_UTF8_CU  4
#define MAX_CP_WIDTH UINT32_WIDTH

/** Describes the role of a codeunit (aka 'cu') in a utf8 string.
 * This enum can be used to process utf8 strings grapheme by grapheme instead of one 'cu' at a time.
 * see `cp_cnt`. */
enum cu_type {
	CU_INVALID = -1,
	CU_CONTINUATION,
	CU_ASCII,
	CU_SEQ_START_2,
	CU_SEQ_START_3,
	CU_SEQ_START_4,
};

/** Return the corresponding `enum cptype` for the given character.
 * Defaults to `CP_INVALID` if none of the conditions are met. */
enum cu_type get_cu_type(char8_t c);

/** Returns the number of graphemes with the given utf8 string `str`,
 * -1 if the string is not valid utf8 : this exclude continuation bytes,
 * which means you could have an invalid utf8 string without count_graphemes returning -1.
 * This is because count_graphems skips continuation bytes, as the utf8 standard allows us to so for this exact reason.
 * NOTE THAT : in this case, str would still be useable with any of escape's functions.
 * If you want to check for the full integrity of `str` as a utf8 string, use `get_invalid_cp`.
 * `get_invalid_cp` could also be handy if `count_graphemes` does return -1. */
long cu_cnt(const char8_t* str, size_t len);

/** Returns the index of the first invalid utf8 codepoint found in str, -1 if there are none. */
long get_inv_cu(const char8_t* str, size_t len);

/** Returns the corresponding UTF32 character given the pointer to the first codepoint of the grapheme, 0 if str is invalid */
char32_t strtocp(const char8_t* cp);
/** Fills gphm with the codepoints found in c32. Returns the number of codepoints found.
 * gphm should have at least as many bytes as there are in c32. */
size_t cptostr(char32_t cp, char8_t* dest);
