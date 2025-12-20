#include <limits.h>
#include <stdbit.h>
#include <stdio.h>
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
		return CP_ASCII;
	} else {
		const uchar leading_ones = stdc_leading_ones_uc(c);
		if (stdc_first_leading_zero_uc(c) == leading_ones + 1u) {
			if (leading_ones == 1) return CP_CONTINUATION; // Continuation bytes don't represent a gphm of size 1 (an ASCII char)
			else if (leading_ones <= 4) return (enum cptype)leading_ones;
		}
	}

	return CP_INVALID;
}

long gphm_cnt(const char* restrict str, size_t strlen)
{
	ulong cnt = 0;

	enum cptype type = CP_INVALID;
	for (size_t i = 0; i < strlen; i += type, ++cnt) {
		if ((type = get_cptype(str[i])) == CP_INVALID) {
			return -1;
		}
	}

	return cnt;
}

long get_inv_cp(const char* restrict str, size_t strlen)
{
	for (size_t i = 0; i < strlen; ++i) {
		if (get_cptype(str[i]) == CP_INVALID) {
			return i;
		}
	}

	return -1;
}

char32_t gphmtoc32(const char8_t* str)
{
	const enum cptype cp_cnt = get_cptype(*str);
	// TODO : err when string too long
	if (cp_cnt <= CP_CONTINUATION) {
		return 0U;
	}
	char32_t c32 = 0U;
	for (uchar i = 0; i < cp_cnt; ++i) {
		c32 |= str[cp_cnt - i - 1] << (i * UCHAR_WIDTH);
	}

	return c32;
}

size_t c32togphm(char32_t c32, char* restrict gphm)
{
	const size_t c32_bw = stdc_bit_width(c32);
	const size_t cp_cnt = c32_bw == 7 ? 1 : c32_bw / UCHAR_WIDTH; // ASCII only has 7 (< 8) bits but counts as 1
	for (size_t i = 0; i < cp_cnt; ++i) {
		const char32_t bitmask = 0x000000ff << (i * UCHAR_WIDTH);
		gphm[cp_cnt - i - 1]   = (c32 & bitmask) >> (i * UCHAR_WIDTH);
	}
	return cp_cnt;
}

