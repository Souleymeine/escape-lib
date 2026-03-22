#pragma once

#include "err.h"
#include <stdint.h>
#include <uchar.h>
#if _WIN32
#include <windows.h>
#endif

#include <stdbit.h>
#include <stddef.h>
#include <uchar.h>

// Control Sequence Introducer
#define CSI "\x1b["


// TODO : Solve terminfo.

struct esc_seqel {
	enum {
		ESC_FMT_STR,
		ESC_FMT_CHAR,
		ESC_FMT_U16,
		ESC_FMT_U8,
	} tag;
	union {
		struct {
			char* buf;
			size_t len;
		} str;
		char chr;
		uint16_t uint16;
		uint8_t uint8;
	};
};

#define ESC_U16_MAX_DIGITS 5
#define ESC_U8_MAX_DIGITS  3

// Useful to allocate enough memory for a sequence given the amount of parameters
#define U8_WORST_PARAMSEQ_LEN(n)  (2 + n * U8_MAX_DIGITS + n)  // = 2 + n * U8_MAX_DIGITS + (n - 1) + 1 (CSI + params + semi-colons + end)
#define U16_WORST_PARAMSEQ_LEN(n) (2 + n * U16_MAX_DIGITS + n) // same here

#define SEQ_STR(s, l) (struct seqel){ .type = ESC_FMT_STR,  .str.buf = s, .str.len = l }
#define SEQ_STRL(s)   (struct seqel){ .type = ESC_FMT_STR,  .str.buf = s, .str.len = sizeof(s) - 1 } // FOR STRING LITERALS ONLY!
#define SEQ_CHR(c)    (struct seqel){ .type = ESC_FMT_CHAR, .chr = c }
#define SEQ_U16(n)    (struct seqel){ .type = ESC_FMT_U16,  .uint16 = n }
#define SEQ_U8(n)     (struct seqel){ .type = ESC_FMT_U8,   .uint8 = n }

/** Returns the number of digits in base 10 of 16 bit unsigned int x */
uint8_t cntdigits(uint16_t n);

/**  */
size_t seqcat(char8_t* dest, const struct esc_seqel* elements, size_t n);

/**
 * Fills a sequence of fromat CSI p1;p2;...end with pn being an unsigned 8 bit integer (useful for style/color codes)
 * And returns the length of said sequence
 */
size_t u8paramseq(char8_t* dest, const uint8_t* params, size_t n, char end);

/**
 * Fills a sequence of fromat CSI p1;p2;...end with pn being an unsigned 16 bit integer
 * And returns the length of said sequence
 */
size_t u16paramseq(char8_t* dest, const uint16_t* params, size_t n, char end);


struct esc_termsize {
	const uint16_t cols;
	const uint16_t rows;
	const uint16_t xpix;
	const uint16_t ypix;
};

struct esc_termsize esc_get_termsize();

enum esc_termflags {
	ESC_TERM_ALTBUF      = 0x1,
	ESC_TERM_HIDE_CURSOR = 0x2,
	ESC_TERM_NO_ECHO     = 0x4,
};

enum esc_stdstream {
	ESC_STDOUT = 0,
	ESC_STDIN  = 1,
	ESC_STDERR = 2,
};

void esc_init_term();

/* Sets the given terminal flags via bitmask.
 * Available flags are available int `enum termflags`.
 * Returns 0 if flags were applied, -1 if it was the same as before. */
ESC_RESULT(void) esc_set_termflags(uint16_t flags);

/* Available for convinience reasons :
 * Calls `init_term` then set_termflags.
 * You almost always want to do both during the initializtion of your program. */
void esc_init_flags(uint16_t flags);

/* Returns a pointer to the program's static terminal flags. */
const uint16_t* esc_get_termflags();

/**
 * Simple UTF-8 unbuffered cross platform terminal writer
 * Returns `true` if len doesn't match how many bytes were written or if nothing was written, false otherwise.
 * */
ESC_RESULT(void) esc_termprint(enum esc_stdstream stream, const char8_t* buf, size_t len);
#define ESC_LOG_ERR(s) esc_termprint(ESC_STDERR, s, sizeof(s))

#if _WIN32
/* Same as `GetStdHandle(STD_IN_HANDLE)`, but returns a pointer
 * to the already statically stored result of this exact call used internally. */
const HANDLE* get_g_stdin_hndl();
/* Same as `GetStdHandle(STD_OUT_HANDLE)`, but returns a pointer
 * to the already statically stored result of this exact call used internally. */
const HANDLE* get_g_stdout_hndl();
#endif

/* Resets the terminal with none of the flags present in `enum termflags`. */
void esc_cleanup_term();
