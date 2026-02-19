#include <limits.h>
#include <stdbit.h>

#include "../include/_escdef.h"
#include "../include/utf.h"

enum cu_type get_cu_type(c8 c)
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
		return CU_ASCII;
	} else {
		const uchar leading_ones = stdc_leading_ones(c);
		if (stdc_first_leading_zero(c) == leading_ones + 1u) {
			if (leading_ones == 1) {
				return CU_CONTINUATION; // Continuation bytes don't represent a gphm of size 1 (an ASCII char)
			} else if (leading_ones <= 4) {
				return (enum cu_type)leading_ones;
			}
		}
	}

	return CU_INVALID;
}

isize cu_cnt(const c8* str, usize strlen)
{
	usize count = 0;

	enum cu_type type = CU_INVALID;
	for (usize i = 0; i < strlen; i += type, ++count) {
		if ((type = get_cu_type(str[i])) == CU_INVALID) {
			return -1;
		}
	}

	return count;
}

isize get_inv_cu(const c8* str, usize strlen)
{
	for (usize i = 0; i < strlen; ++i) {
		if (get_cu_type(str[i]) == CU_INVALID) {
			return i;
		}
	}

	return -1;
}

c32 strtocp(const c8* cp)
{
	// from https://ziglang.org/documentation/0.15.2/std/#std.unicode.utf8Decode4
	const enum cu_type cu_t = get_cu_type(cp[0]);
	if (cu_t == CU_ASCII) {
		return cp[0];
	}

	c32 c = cp[0] & 0b00000111;
	for (usize i = 1; i < (usize)cu_t; ++i) {
		c <<= 6;
		c |= cp[i] & 0b00111111;
	}

	return c;
}

usize cptostr(c32 c, char8_t* dest)
{
	// From : https://stackoverflow.com/questions/42012563/convert-unicode-code-points-to-utf-8-and-utf-32
	// TODO : find the core logic and refactor if it's faster
	if (c <= 0x7F) {
		dest[0] = c;
		return 1;
	}
	if (c <= 0x7FF) {
		dest[0] = 0b11000000 | (c >> 6);
		dest[1] = 0b10000000 | (c & 0b111111);
		return 2;
	}
	if (c <= 0xFFFF) {
		dest[0] = 0b11100000 | (c >> 12);
		dest[1] = 0b10000000 | ((c >> 6) & 0b111111);
		dest[2] = 0b10000000 | (c & 0b111111);
		return 3;
	}
	if (c <= 0x10FFFF) {
		dest[0] = 0b11110000 | (c >> 18);
		dest[1] = 0b10000000 | ((c >> 12) & 0b111111);
		dest[2] = 0b10000000 | ((c >> 6) & 0b111111);
		dest[3] = 0b10000000 | (c & 0b111111);
		return 4;
	}
	return 0;
}

