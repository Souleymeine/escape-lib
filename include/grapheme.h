#pragma once

#include <stddef.h>
#include <uchar.h>

/**
 * An enum which should be used to describe the role of a code point in a utf8 string.
 */
enum utf8cpt_type : char {
	INVALID = -1,
	CONTINUATION,
	ASCII,
	TWO_WIDE_SEQ_START,
	THREE_WIDE_SEQ_START,
	FOUR_WIDE_SEQ_START,
};

/**
 * Return the corresponding `enum cpt_type` for the given character.
 * Defaults to `INVALID` if none of the conditions are met.
 */
enum utf8cpt_type get_utf8cpt_type(const char8_t c);

/**
 * Count the number of graphemes with the given utf8 string.
 * Returns -1 if the string is not valid utf8.
 */
int count_utf8_graphemes(const char* restrict str, const size_t str_len);

