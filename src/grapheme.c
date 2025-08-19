#include <stdbit.h>

#include "grapheme.h"

enum utf8cpt_type get_utf8cpt_type(const char8_t c)
{
	// https://www.rfc-editor.org/rfc/rfc3629#section-3
	if (stdc_bit_width_uc(c) < 8) {
		return ASCII;
	} else {
		const unsigned char leading_ones = stdc_leading_ones_uc(c);
		if (stdc_first_leading_zero_uc(c) == leading_ones + 1) {
			return leading_ones;
		}
	}

	return INVALID;
}

int count_utf8_graphemes(const char* restrict str, const size_t str_len)
{
	unsigned int count = 0;

	char i_type = INVALID;
	size_t i    = 0;
	while (i < str_len) {
		i_type = get_utf8cpt_type(str[i]);
		if (i_type == INVALID) {
			return -1;
		} else {
			++count;
			i += i_type;
		}
	}

	return count;
}

