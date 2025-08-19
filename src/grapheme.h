#pragma once

#include <stddef.h>

/**
 * An enum which should be used to describe the role of a code point in a utf8 string.
 */
enum cpt_type : char {
	INVALID = -1,
	CONTINUATION,
	ASCII,
	DOUBLE_BEGIN,
	TRIPLE_BEGIN,
	QUADRUPLE_BEGIN,
};

/**
 * Return the corresponding `enum cpt_type` for the given character.
 * Defaults to `INVALID` if none of the conditions are met.
 */
enum cpt_type get_cpt_type(const unsigned char c);

/**
 * Count the number of graphemes with the given utf8 string.
 * Returns -1 if the string is not valid utf8.
 */
int count_graphemes(const char* restrict str, const size_t str_len);

