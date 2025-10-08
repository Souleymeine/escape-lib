#pragma once

#include <stddef.h>
#include <uchar.h>

/**
 * Describes the role of a codepoint (aka 'cp') in a utf8 string.
 * This enum can be used to process utf8 strings grapheme by grapheme instead of one 'cp' at a time.
 * try the fonction `count_graphemes`.
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
 * Return the corresponding `enum cpt_type` for the given character.
 * Defaults to `INVALID` if none of the conditions are met.
 */
enum cptype get_cptype(char8_t c);

/**
 * Count the number of graphemes with the given utf8 string.
 * Returns -1 if the string is not valid utf8.
 */
size_t count_graphemes(const char* restrict str, size_t strlen);

