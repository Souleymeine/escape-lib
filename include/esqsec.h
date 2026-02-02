#pragma once

#include <stdbit.h>
#include <stddef.h>

#include "escdef.h"

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
	ENUMTYPE(enum, unsigned){
		FMT_STR,
		FMT_CHAR,
		FMT_U16,
		FMT_U8,
	} type;
};

#define U16_MAX_DIGITS 5
#define U8_MAX_DIGITS  3

// Useful to allocate enough memory for a sequence given the amount of parameters
#define U8_PARAM_SEQLEN(n)  (2 + n * U8_MAX_DIGITS + n)  // = 2 + n * U8_MAX_DIGITS + (n - 1) + 1 (CSI + params + semi-colons + m)
#define U16_PARAM_SEQLEN(n) (2 + n * U16_MAX_DIGITS + n) // same here

#define SEQ_STR(s, l) {.type = FMT_STR, .str.buf = s, .str.len = l}
// FOR STRING LITERALS ONLY!
#define SEQ_STRL(s) ((struct seqel){.type = FMT_STR, .str.buf = s, .str.len = sizeof(s) - 1})
#define SEQ_CHR(c)  ((struct seqel){.type = FMT_CHAR, .chr = c})
#define SEQ_U16(n)  ((struct seqel){.type = FMT_U16, .uint16 = n})
#define SEQ_U8(n)   ((struct seqel){.type = FMT_U8, .uint8 = n})

/** Returns the number of digits in base 10 of 16 bit unsigned int x */
unsigned char cntdigits(uint16_t n);

/**  */
size_t seqcat(char* restrict dest, const struct seqel* restrict elements, size_t n);

/**
 * Fills a sequence of fromat CSI p1;p2;...m with pn being a unsigned 16 bit integer
 * And returns the length of said sequence
 */
size_t paramu8seq(char* restrict dest, const uint8_t* restrict params, size_t n);

/**
 * Fills a sequence of fromat CSI p1;p2;...m with pn being a unsigned 8 bit integer (useful for style/color codes)
 * And returns the length of said sequence
 */
size_t paramu16seq(char* restrict dest, const uint16_t* restrict params, size_t n);

