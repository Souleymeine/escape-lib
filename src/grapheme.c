#include <stdbit.h>
#include <uchar.h>

#include "grapheme.h"

enum cptype get_cptype(char8_t c)
{
	// see https://www.rfc-editor.org/rfc/rfc3629#section-3
	if (stdc_bit_width_uc(c) < 8u) {
		return ASCII;
	} else {
		const unsigned char leading_ones = stdc_leading_ones_uc(c);
		if (leading_ones > 4u) {
			return INVALID;
		} else if (stdc_first_leading_zero_uc(c) == leading_ones + 1u) {
			return leading_ones;
		}
	}

	return INVALID;
}

long count_graphemes(const char* restrict str, size_t strlen)
{
	unsigned int count = 0;

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

