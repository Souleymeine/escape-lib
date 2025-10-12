#include <stdbit.h>
#include <uchar.h>

#include "grapheme.h"

#include "_escdef.h"

enum cptype get_cptype(char8_t c)
{
	/**
	 * See https://www.rfc-editor.org/rfc/rfc3629#section-3 :
	 * --------------------+-------------------------------------
	 * Char. number range  |        UTF-8 octet sequence
	 *    (hexadecimal)    |              (binary)
	 * --------------------+-------------------------------------
	 * 0000 0000-0000 007F | 0xxxxxxx
	 * 0000 0080-0000 07FF | 110xxxxx 10xxxxxx
	 * 0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
	 * 0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
	 */
	if (stdc_bit_width_uc(c) < 8u) {
		return ASCII;
	} else {
		const uchar leading_ones = stdc_leading_ones_uc(c);
		if (leading_ones <= 4u && stdc_first_leading_zero_uc(c) == leading_ones + 1u) {
			return leading_ones;
		}
	}

	return INVALID;
}

long count_graphemes(const char* restrict str, size_t strlen)
{
	ulong count = 0;

	enum cptype type = INVALID;
	for (size_t i = 0; i < strlen; i += type, ++count) {
		if ((type = get_cptype(str[i])) == INVALID) {
			return -1;
		}
	}

	return count;
}

long get_invalid_cp(const char* restrict str, size_t strlen)
{
	for (size_t i = 0; i < strlen; ++i) {
		if (get_cptype(str[i]) == INVALID) {
			return i;
		}
	}

	return -1;
}

