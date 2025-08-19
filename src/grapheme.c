#include <stdbit.h>
#include <stddef.h>

#include "grapheme.h"

enum cpt_type get_cpt_type(const unsigned char c)
{
	// https://www.rfc-editor.org/rfc/rfc3629#section-3
	if (stdc_bit_width_uc(c) < 8) {
		return ASCII;
	} else {
		const unsigned char leading_ones = stdc_leading_ones_uc(c);
		// Check if there's a zero after leading one(s)
		if (stdc_first_leading_zero_uc(c) == (unsigned char)(leading_ones + 1)) {
			return leading_ones;
		}
	}

	return INVALID;
}

int count_graphemes(const char* restrict str, const size_t str_len)
{
	unsigned int len = 0;

	enum cpt_type i_type = INVALID;
	size_t i             = 0;
	while (i < str_len) {
		i_type = get_cpt_type(str[i]);

		if (i_type == INVALID) {
			return -1;
		} else {
			++len;
			i += i_type;
		}
	}

	return len;
}

