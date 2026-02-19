#pragma once

#include <stdbit.h>
#include <stddef.h>
#include <uchar.h>

#define UTF8(s) (char8_t*)s

#define MAX_UTF8_CU  4
#define MAX_CP_WIDTH UINT32_WIDTH

/** Describes the role of a codeunit (aka 'cu') in a utf8 string.
 * This enum can be used to process utf8 strings grapheme by grapheme instead of one 'cu' at a time.
 * see `cp_cnt`. */
enum cu_type {
	CU_INVALID = -1,
	CU_CONTINUATION,
	CU_ASCII,
	CU_SEQ_START_2,
	CU_SEQ_START_3,
	CU_SEQ_START_4,
};

/** Return the corresponding `enum cptype` for the given character.
 * Defaults to `CP_INVALID` if none of the conditions are met. */
enum cu_type get_cu_type(char8_t c);

/** Returns the number of graphemes with the given utf8 string `str`,
 * -1 if the string is not valid utf8 : this exclude continuation bytes,
 * which means you could have an invalid utf8 string without count_graphemes returning -1.
 * This is because count_graphems skips continuation bytes, as the utf8 standard allows us to so for this exact reason.
 * NOTE THAT : in this case, str would still be useable with any of escape's functions.
 * If you want to check for the full integrity of `str` as a utf8 string, use `get_invalid_cp`.
 * `get_invalid_cp` could also be handy if `count_graphemes` does return -1. */
long cu_cnt(const char8_t* str, size_t len);

/** Returns the index of the first invalid utf8 codepoint found in str, -1 if there are none. */
long get_inv_cu(const char8_t* str, size_t len);

/** Returns the corresponding UTF32 character given the pointer to the first codepoint of the grapheme, 0 if str is invalid */
char32_t strtocp(const char8_t* cp);
/** Fills gphm with the codepoints found in c32. Returns the number of codepoints found.
 * gphm should have at least as many bytes as there are in c32. */
size_t cptostr(char32_t cp, char8_t* dest);
