#pragma once

#include <stddef.h>
#include <stdint.h>

#include "escdef.h"

enum fmtspec {
	FMTSPEC_STR,
	FMTSPEC_UINT,
};

// All escape sequences accept either text or unsigned integers (in ASCII)

struct seqslice {
	enum fmtspec type;
	union {
		struct {
			char* buf;
			size_t len;
		} str;
		uint16_t uint;
	};
};

// TODO : find better names omg
#define SEQ_STR(s)  {.type = FMTSPEC_STR, .str.buf = s, .str.len = sizeof(s) - 1}
#define SEQ_UINT(n) {.type = FMTSPEC_UINT, .uint = n}

// TODO : Solve terminfo.

/** Returns the number of digits in base 10 of 16 bit unsigned int x */
unsigned char cntdigits(uint16_t x);

/** Returns the length of the given sequence */
size_t seqlen(const struct seqslice* slices, size_t n);

/**
 * Basic integer formatting and string concatenation in the context of escape sequences.
 * Fills DEST with the concatenated string of N `seqslice`.
 * DEST must be at least `seqlen(slices, n)` bytes.
 */
void seqcat(char* restrict dest, const struct seqslice* slices, size_t n);


