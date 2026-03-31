#include <limits.h>
#include <stdbit.h>
#include <uchar.h>

#include "../../include/rndr.h"

ESC_RESULT(enum esc_cu) esc_getcu(char8_t c)
{
	/** See https://www.rfc-editor.org/rfc/rfc3629#section-3
	 *  and https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-3/#G27288
	 * --------------------+-------------------------------------
	 * Char. number range  |        UTF-8 octet sequence
	 *    (hexadecimal)    |              (binary)
	 * --------------------+-------------------------------------
	 * 0000 0000-0000 007F | 0xxxxxxx
	 * 0000 0080-0000 07FF | 110xxxxx 10xxxxxx
	 * 0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
	 * 0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
	 */
	if (stdc_bit_width(c) < 8) {
		return ESC_RESVAL(enum esc_cu, ESC_CU_ASCII);
	} else {
		const uint8_t leading_ones = stdc_leading_ones_uc(c);
		if (stdc_first_leading_zero_uc(c) == leading_ones + 1) {
			if (leading_ones == 1) {
				return ESC_RESVAL(enum esc_cu, ESC_CU_CONTINUATION); // Continuation bytes don't represent a gphm of size 1 (an ASCII char)
			} else if (leading_ones <= 4) {
				return ESC_RESVAL(enum esc_cu, (enum esc_cu)leading_ones);
			}
		}
	}

	return ESC_RESERR(enum esc_cu, ESC_ERR_INVALID_UTF8_CU);
}

ESC_RESULT(size_t) esc_getinvcu(const char8_t* str, size_t len)
{
	for (size_t i = 0; i < len; ++i) {
		if (esc_getcu(str[i]).err == ESC_ERR_INVALID_UTF8_CU) {
			return ESC_RESVAL(size_t, i);
		}
	}
	return ESC_RESVAL(size_t, ESC_ERR_NO_INVALID_UTF8_CU);
}

constexpr uint8_t USED_MASK = 0b00111111;

ESC_RESULT(char32_t) esc_mbtocp(const char8_t* cp)
{
	// from https://ziglang.org/documentation/0.15.2/std/#std.unicode.utf8Decode4
	const ESC_RESULT(enum esc_cu) cu_type = esc_getcu(cp[0]);
	ESC_TRY(char32_t, cu_type);

	if (cu_type.val == ESC_CU_ASCII) {
		return ESC_RESVAL(char32_t, cp[0]);
	}

	char32_t c = cp[0] & 0b00000111;
	for (size_t i = 1; i < cu_type.val; ++i) {
		c <<= 6;
		c |= cp[i] & USED_MASK;
	}

	return ESC_RESVAL(char32_t, c);
}

ESC_RESULT(size_t) esc_cptomb(char8_t* dest, char32_t c)
{
	// From : https://stackoverflow.com/questions/42012563/convert-unicode-code-points-to-utf-8-and-utf-32
	// TODO : find the core logic and refactor if it's faster
	constexpr uint8_t UNUSED_MASK = 0b10000000;
	if (c <= 0x7F) {
		dest[0] = c;
		return ESC_RESVAL(size_t, 1);
	} else if (c <= 0x7FF) {
		dest[0] = 0b11000000  | (c >> 6);
		dest[1] = UNUSED_MASK | (c & USED_MASK);
		return ESC_RESVAL(size_t, 2);
	} else if (c <= 0xFFFF) {
		dest[0] = 0b11100000  | (c >> 12);
		dest[1] = UNUSED_MASK | ((c >> 6) & USED_MASK);
		dest[2] = UNUSED_MASK | (c        & USED_MASK);
		return ESC_RESVAL(size_t, 3);
	} else if (c <= 0x10FFFF) {
		dest[0] = 0b11110000  | (c >> 18);
		dest[1] = UNUSED_MASK | ((c >> 12) & USED_MASK);
		dest[2] = UNUSED_MASK | ((c >> 6)  & USED_MASK);
		dest[3] = UNUSED_MASK | (c         & USED_MASK);
		return ESC_RESVAL(size_t, 4);
	}
	return ESC_RESERR(size_t, ESC_ERR_INVALID_UNICODE_CODEPOINT);
}

