#pragma once

#include <stdint.h>
#include <uchar.h>

#include "err.h"

struct esc_rgb {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

#if __STDC_VERSION__ >= 202311L
typedef enum esc_clrtag : uint8_t {
	ESC_CLRTAG_CODE = 0b01,
	ESC_CLRTAG_RGB  = 0b10,
	ESC_CLRTAG_ID   = 0b11,
} esc_clrtag;
#else
typedef uint8_t esc_clrtag;
#endif

union esc_clrval {
	uint8_t code;
	uint8_t id;
	struct esc_rgb rgb;
};

struct esc_clr {
	esc_clrtag tag;
	union esc_clrval val;
};
#define ESC_CLR_RGB(r, g, b) (struct esc_clr){.tag = ESC_CLRTAG_RGB,  .val.rgb  = {r, g, b}}
#define ESC_CLR_ID(c)        (struct esc_clr){.tag = ESC_CLRTAG_ID,   .val.id   = c        }
#define ESC_CLR_CODE(c)      (struct esc_clr){.tag = ESC_CLRTAG_CODE, .val.code = c + 30   }

enum esc_clrcode {
	ESC_CLRCODE_BLACK,
	ESC_CLRCODE_RED,
	ESC_CLRCODE_GREEN,
	ESC_CLRCODE_YELLOW,
	ESC_CLRCODE_BLUE,
	ESC_CLRCODE_MAGENTA,
	ESC_CLRCODE_CYAN,
	ESC_CLRCODE_WHITE,
	ESC_CLRCODE_DEF = 9,
};

struct esc_coord {
	const uint16_t x;
	const uint16_t y;
};
_ESC_RESULT_DECL(struct esc_coord);

/** Describes the role of a codeunit (aka 'cu') in a utf8 string.
 * This enum can be used to process utf8 strings grapheme by grapheme instead of one 'cu' at a time.
 * see `cp_cnt`. */
enum esc_cu {
	ESC_CU_CONTINUATION,
	ESC_CU_ASCII,
	ESC_CU_SEQ_START_2,
	ESC_CU_SEQ_START_3,
	ESC_CU_SEQ_START_4,
};
_ESC_RESULT_DECL(enum esc_cu);

struct esc_strbuf_impl {
	enum esc_strbuf_type {
		ESC_STRBUF_CIRCULAR,
		ESC_STRBUF_GROWABLE,
	} buftype_tag;
	union {
		struct {
			enum {
				ESC_STRBUF_CIRCULAR_STACK,
				ESC_STRBUF_CIRCULAR_HEAP,
			} alloc_tag;
			union {
				size_t heapsize;
				struct {
					char* buf;
					size_t size;
				} stack;
			};
		} circular;
		struct {
			size_t init_size;
			float growth_rate;
		} growable;
	};
};
#define ESC_STRBUF_IMPL_CIRCULAR_STACK(stack_buf) \
&(struct esc_strbuf_impl) {                       \
	.buf_type_tag = ESC_STRBUF_CIRCULAR,          \
	.circular = {                                 \
		.alloc_tag = ESC_STRBUF_CIRCULAR_STACK,   \
		.stack = {                                \
			.buf = stack_buf,                     \
			.size = sizeof(stack_buf)             \
		}                                         \
	},                                            \
}
#define ESC_STRBUF_IMPL_CIRCULAR_HEAP(buf_size)  \
&(struct esc_strbuf_impl) {                      \
	.buf_type_tag = ESC_STRBUF_CIRCULAR,         \
	.circular = {                                \
		.alloc_tag = ESC_STRBUF_CIRCULAR_HEAP,   \
		.heapsize = buf_size,                    \
	},                                           \
}
#define ESC_STRBUF_IMPL_GROWABLE(buf_init_size, buf_growth_rate) \
&(struct esc_strbuf_impl) {                                      \
	.buf_type_tag = ESC_STRBUF_GROWABLE,                         \
	.growable = {                                                \
		.init_size = buf_init_size,                              \
		.growth_rate buf_growth_rate,                            \
	},                                                           \
}

// TODO : explain each possible strbuf_impl
ESC_RESULT(void) esc_initscr(const struct esc_strbuf_impl* strbuf_impl, bool virtual_grid, struct esc_clr bgclr, struct esc_clr fgclr);
ESC_RESULT(void) esc_deinitscr();

ESC_RESULT(void) esc_refresh();

ESC_RESULT(struct esc_coord) esc_idxtocoord(size_t i);
ESC_RESULT(size_t) esc_coordtoidx(uint16_t x, uint16_t y);

ESC_RESULT(void) esc_setcp(char32_t c, uint16_t x, uint16_t y);

ESC_RESULT(void) esc_setbgclr(struct esc_clr clr, uint16_t x, uint16_t y);
ESC_RESULT(void) esc_setfgclr(struct esc_clr clr, uint16_t x, uint16_t y);
ESC_RESULT(void) esc_setclrpair(struct esc_clr bgclr, struct esc_clr fgclr, uint16_t x, uint16_t y);

ESC_RESULT(void) esc_setvis(bool visible, uint16_t x, uint16_t y);
ESC_RESULT(void) esc_togglevis(uint16_t x, uint16_t y);

#define ESC_UTF8(s) (char8_t*)s

#define ESC_MAX_UTF8_CU  4
#define ESC_MAX_CP_WIDTH UINT32_WIDTH

/** Return the corresponding `enum cptype` for the given character.
 * Defaults to `CP_INVALID` if none of the conditions are met. */
ESC_RESULT(enum esc_cu) esc_getcu(char8_t c);

/** Returns the number of graphemes with the given utf8 string `str`,
 * -1 if the string is not valid utf8 : this exclude continuation bytes,
 * which means you could have an invalid utf8 string without count_graphemes returning -1.
 * This is because count_graphems skips continuation bytes, as the utf8 standard allows us to so for this exact reason.
 * NOTE THAT : in this case, str would still be useable with any of escape's functions.
 * If you want to check for the full integrity of `str` as a utf8 string, use `get_invalid_cp`.
 * `get_invalid_cp` could also be handy if `count_graphemes` does return -1. */
ESC_RESULT(size_t) esc_cu_cnt(const char8_t* str, size_t len);

/** Returns the index of the first invalid utf8 codepoint found in str, -1 if there are none. */
ESC_RESULT(size_t) get_inv_cu(const char8_t* str, size_t len);

/** Returns the corresponding UTF32 character given the pointer to the first codepoint of the grapheme, 0 if str is invalid */
ESC_RESULT(char32_t) strtocp(const char8_t* cp);
/** Fills gphm with the codepoints found in c32. Returns the number of codepoints found.
 * gphm should have at least as many bytes as there are in c32. */
ESC_RESULT(size_t) cptostr(char32_t cp, char8_t* dest);

