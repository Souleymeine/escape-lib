#pragma once

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

struct seqel {
	union {
		struct {
			char* buf;
			size_t len;
		} str;
		char chr;
		uint16_t uint16;
		uint8_t uint8;
	};
	enum {
		FMT_STR,
		FMT_CHAR,
		FMT_U16,
		FMT_U8,
	} type;
};

#define U16_MAX_DIGITS 5
#define U8_MAX_DIGITS  3

// Useful to allocate enough memory for a sequence given the amount of parameters
#define U8_WORST_PARAMSEQ_LEN(n)                                                                            \
	(2 + n * U8_MAX_DIGITS + n) // = 2 + n * U8_MAX_DIGITS + (n - 1) + 1 (CSI + params + semi-colons + end)
#define U16_WORST_PARAMSEQ_LEN(n) (2 + n * U16_MAX_DIGITS + n) // same here

#define SEQ_STR(s, l) {.type = FMT_STR, .str.buf = s, .str.len = l}
// FOR STRING LITERALS ONLY!
#define SEQ_STRL(s) ((struct seqel){.type = FMT_STR, .str.buf = s, .str.len = sizeof(s) - 1})
#define SEQ_CHR(c)  ((struct seqel){.type = FMT_CHAR, .chr = c})
#define SEQ_U16(n)  ((struct seqel){.type = FMT_U16, .uint16 = n})
#define SEQ_U8(n)   ((struct seqel){.type = FMT_U8, .uint8 = n})

/** Returns the number of digits in base 10 of 16 bit unsigned int x */
unsigned char cntdigits(uint16_t n);

/**  */
size_t seqcat(char8_t* dest, const struct seqel* elements, size_t n);

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


struct termsize {
	uint16_t cols;
	uint16_t rows;
	uint16_t xpix;
	uint16_t ypix;
};

struct termsize get_termsize();

/** holds bitflags if positive, doesn't if negative */
typedef int termstatefl;

enum termflags {
	TERM_ALTBUF      = 0x1,
	TERM_HIDE_CURSOR = 0x2,
	TERM_NO_ECHO     = 0x4,
};

/* === scrflags MUST NEVER OVERLAP WITH termflags!!! === */

// IMPORTANT ! : the only requirement for this to work is for these flags
// to be bigger than at least the biggest (last) flag of `enum termflags`

/* Extends `enum termflags` */
enum scrflags {
	SCREEN_HOLD_TERMFLAGS = 0x8,
	SCREEN_USE_VIRTUAL    = 0x10,
};

enum escerr {
	ESC_OK = 0,
	ESC_ERR_COORD_X,
	ESC_ERR_COORD_Y,
	ESC_ERR_UTF8,
	ESC_ERR_CP,
};

enum stdstream {
	STDOUT = 0,
	STDIN  = 1,
	STDERR = 2,
};

/* Initialize some states variables and gathers information about the terminal the program is
 * running in. Required for escape sequences to work properly. */
void init_term();

/* Sets the given terminal flags via bitmask.
 * Available flags are available int `enum termflags`.
 * Returns 0 if flags were applied, -1 if it was the same as before. */
int set_termflags(termstatefl flags);

/* Available for convinience reasons :
 * Calls `init_term` then set_termflags.
 * You almost always want to do both during the initializtion of your program. */
void init_flags(termstatefl flags);

/* Returns a pointer to the program's static terminal flags. */
const termstatefl* get_termflags();


/**
 * Simple UTF-8 unbuffered cross platform terminal writer
 * Returns `true` if len doesn't match how many bytes were written or if nothing was written, false otherwise.
 * */
bool termprint(enum stdstream stream, const char8_t* buf, size_t len);
#define ESC_LOG_ERR(s) termprint(STDERR, s, sizeof(s))

/* If called bedore `init_term`/`init_flags`, stdscr will use a virtual screen by default. */
void usevscr();

#if _WIN32
/* Same as `GetStdHandle(STD_IN_HANDLE)`, but returns a pointer
 * to the already statically stored result of this exact call used internally. */
const HANDLE* get_g_stdin_hndl();
/* Same as `GetStdHandle(STD_OUT_HANDLE)`, but returns a pointer
 * to the already statically stored result of this exact call used internally. */
const HANDLE* get_g_stdout_hndl();
#endif

/* Resets the terminal with none of the flags present in `enum termflags`. */
void cleanup_term();
