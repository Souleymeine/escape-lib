#pragma once

#include <stdbit.h>
#include <stddef.h>

#include "escdef.h"

// TODO : Solve terminfo.

struct seqslice {
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

#define F_STR(s, l) {.type = FMT_STR, .str.buf = s, .str.len = l}
// FOR STRING LITERALS ONLY!
#define F_STRL(s) {.type = FMT_STR, .str.buf = s, .str.len = sizeof(s) - 1}
#define F_CHR(c)  {.type = FMT_CHAR, .chr = c}
#define F_U16(n)  {.type = FMT_U16, .uint16 = n}
#define F_U8(n)   {.type = FMT_U8, .uint8 = n}

/** Returns the number of digits in base 10 of 16 bit unsigned int x */
unsigned char cntdigits(uint16_t n);

size_t seqcat(char* restrict dest, const struct seqslice* restrict slices, size_t n);

