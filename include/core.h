#pragma once

#include <stdint.h>
#include <uchar.h>
#if _WIN32
#include <windows.h>
#endif

#include "err.h"

// Control Sequence Introducer
#define CSI "\x1b["


// TODO : Solve terminfo.

struct esc_seqel {
	enum {
		ESC_FMT_STR,
		ESC_FMT_CHR,
		ESC_FMT_U16,
		ESC_FMT_U8,
	} tag;
	union {
		struct {
			char* buf;
			size_t len;
		} str;
		uint16_t uint16;
		uint8_t uint8;
	};
};

#define ESC_U16_MAX_DIGITS 5
#define ESC_U8_MAX_DIGITS  3

// Useful to allocate enough memory for a sequence given the amount of parameters
#define ESC_U8_WORST_SEQLEN(param_count)  (2 + param_count * ESC_U8_MAX_DIGITS  + param_count) // = 2 + n * U8_MAX_DIGITS + (n - 1) + 1 (CSI + params + semi-colons + end)
#define ESC_U16_WORST_SEQLEN(param_count) (2 + param_count * ESC_U16_MAX_DIGITS + param_count) // same here

#define ESC_SEQEL_MAX_PARAM 13 // RGB is the biggest known for now

#define ESC_SEQSTR(s, l) (struct esc_seqel){.tag = ESC_FMT_STR, .str.buf = s, .str.len = l}
#define ESC_SEQSTRL(s)   (struct esc_seqel){.tag = ESC_FMT_STR, .str.buf = s, .str.len = sizeof(s) - 1} // FOR STRING LITERALS ONLY!
#define ESC_SEQCHR(c)    (struct esc_seqel){.tag = ESC_FMT_CHR, .uint8 = c}
#define ESC_SEQU8(n)     (struct esc_seqel){.tag = ESC_FMT_U8,  .uint8 = n}
#define ESC_SEQU16(n)    (struct esc_seqel){.tag = ESC_FMT_U16, .uint16 = n}

#define ESC_ARRARG(T, ...) (T[])__VA_ARGS__, sizeof((T[])__VA_ARGS__)/sizeof(T)
#define ESC_STRLARG(s)     s, sizeof(s) - 1

#ifdef ESC_SHORTHAND
#define SEQEL_MAX_PARAM  ESC_SEQEL_MAX_PARAM
#define U16_MAX_DIGITS   ESC_U16_MAX_DIGITS
#define U8_MAX_DIGITS    ESC_U8_MAX_DIGITS
#define U8_WORST_SEQLEN  ESC_U8_WORST_SEQLEN
#define U16_WORST_SEQLEN ESC_U16_WORST_SEQLEN
#define ARRARG  ESC_ARRARG
#define STRLARG ESC_STRLARG
#define SEQSTR  ESC_SEQSTR
#define SEQSTRL ESC_SEQSTRL
#define SEQCHR  ESC_SEQCHR
#define SEQU8   ESC_SEQU8
#define SEQU16  ESC_SEQU16
#endif

/** Returns the number of digits in base 10 of 16 bit unsigned int x */
uint8_t esc_digits(uint16_t n);
size_t esc_seqcat(char8_t* dest, const struct esc_seqel* elements, size_t n);
/**
 * Fills a sequence of fromat CSI p1;p2;...end with pn being an unsigned 8 bit integer (useful for style/color codes)
 * And returns the length of said sequence
 */
size_t esc_u8seq(char8_t* dest, const uint8_t* params, size_t n, char end);
/**
 * Fills a sequence of fromat CSI p1;p2;...end with pn being an unsigned 16 bit integer
 * And returns the length of said sequence
 */
size_t esc_u16seq(char8_t* dest, const uint16_t* params, size_t n, char end);

struct esc_termsize {
	uint16_t cols;
	uint16_t rows;
	uint16_t xpix;
	uint16_t ypix;
}; _ESC_RESULT_DECL(struct esc_termsize);

struct esc_fontsize {
	uint16_t xpix;
	uint16_t ypix;
}; _ESC_RESULT_DECL(struct esc_fontsize);

ESC_RESULT(struct esc_termsize) esc_gettermsize();
ESC_RESULT(struct esc_fontsize) esc_getfontsize();

enum esc_termflags {
	ESC_TERM_ALTBUF   = 0x1,
	ESC_TERM_NOCURSOR = 0x2,
	ESC_TERM_NOECHO   = 0x4,
};

enum esc_stdstream {
	ESC_STDOUT = 0,
	ESC_STDIN  = 1,
	ESC_STDERR = 2,
};

ESC_RESULT(void) esc_init(ESC_OPT(uint16_t) flags);
/* Resets the terminal with none of the flags present in `enum termflags`. */
void esc_cleanup();

/* Sets the given terminal flags via bitmask.
 * Available flags are available int `enum termflags`.
 * Returns 0 if flags were applied, -1 if it was the same as before. */
ESC_RESULT(void) esc_settermflags(uint16_t flags);
uint16_t esc_gettermflags();

/**
 * Simple UTF-8 unbuffered cross platform terminal writer
 * Returns `true` if len doesn't match how many bytes were written or if nothing was written, false otherwise.
 */
ESC_RESULT(void) esc_termwrite(enum esc_stdstream stream, const void* buf, size_t len);
#define ESC_ERRLOG(s) (void)esc_termwrite(ESC_STDERR, s, sizeof(s))

#if _WIN32
/** Returns `GetStdHandle(STD_OUT_HANDLE)`. Already stored by the library, use this function instead */
HANDLE esc_getstdout_h();
/** Returns `GetStdHandle(STD_IN_HANDLE)`. Already stored by the library, use this function instead */
HANDLE esc_getstdin_h();
/** Returns `GetStdHandle(STD_ERR_HANDLE)`. Already stored by the library, use this function instead */
HANDLE esc_getstderr_h();
#endif

