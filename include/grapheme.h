#pragma once

#include <stddef.h>
#include <stdio.h>
#include <uchar.h>

#include "_escdef.h"

/**
 * Describes the role of a codepoint (aka 'cp') in a utf8 string.
 * This enum can be used to process utf8 strings grapheme by grapheme instead of one 'cp' at a time.
 * see `count_graphemes`.
 */
enum cptype : char {
	INVALID = -1,
	CONTINUATION,
	ASCII,
	TWO_WIDE_SEQ_START,
	THREE_WIDE_SEQ_START,
	FOUR_WIDE_SEQ_START,
};

/**
 * Return the corresponding `enum cptype` for the given character.
 * Defaults to `INVALID` if none of the conditions are met.
 */
enum cptype get_cptype(char8_t c);

/**
 * Returns the number of graphemes with the given utf8 string `str`,
 * -1 if the string is not valid utf8 : this exclude continuation bytes,
 * which means you could have an invalid utf8 string without count_graphemes returning -1.
 * This is because count_graphems skips continuation bytes, as the utf8 standard allows us to so for this exact reason.
 * NOTE THAT : in this case, str would still be useable with any of escape's functions.
 * If you want to check for the full integrity of `str` as a utf8 string, use `get_invalid_cp`.
 * `get_invalid_cp` could also be handy if `count_graphemes` does return -1.
 */
long count_graphemes(const char* restrict str, size_t strlen);

/**
 * Returns the index of the first invalid utf8 codepoint found in str, -1 if there are none.
 */
long get_invalid_cp(const char* restrict str, size_t strlen);

