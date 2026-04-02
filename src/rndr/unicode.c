#include <stdbit.h>
#include <string.h>
#include <uchar.h>

#define ESC_SHORTHAND
#include "../../include/rndr.h"

#define ISASCII(c) (c < 0b10000000)

RESULT(enum esc_cu) esc_getcu(char8_t c)
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
	if (ISASCII(c)) {
		return RESOK(enum esc_cu, ESC_CU_ASCII);
	} else {
		const uint8_t leading_ones = stdc_leading_ones_uc(c);
		if (stdc_first_leading_zero_uc(c) == leading_ones + 1) {
			if (leading_ones == 1) {
				return RESOK(enum esc_cu, ESC_CU_CONTINUATION); // Continuation bytes don't represent a gphm of size 1 (an ASCII char)
			} else if (leading_ones <= 4) {
				return RESOK(enum esc_cu, leading_ones);
			}
		}
	}

	return RESERR(enum esc_cu, ESC_ERR_UTF8_CU_INVALID);
}

OPT(size_t) esc_getinvcu(const char8_t* str, size_t len)
{
	for (size_t i = 0; i < len; ++i) {
		if (esc_getcu(str[i]).err == ESC_ERR_UTF8_CU_INVALID) {
			return OPTSOME(size_t, i);
		}
	}
	return OPTNONE(size_t);
}

RESULT(char32_t) esc_mbtocp(const char8_t* str)
{
	// Try validate string
	const RESULT(enum esc_cu) cu_type = esc_getcu(str[0]);
	TRY(char32_t, cu_type);
	const size_t byte_len = strlen((char*)str);
	if      (cu_type.val >  byte_len) return RESERR(char32_t, ESC_ERR_UTF8_STR_LONGER_THAN_ONE_CP);
	else if (cu_type.val != byte_len) return RESERR(char32_t, ESC_ERR_UTF8_STR_INVALID);

	// from https://ziglang.org/documentation/0.15.2/std/#std.unicode.utf8Decode4
	if (cu_type.val == ESC_CU_ASCII) {
		return RESOK(char32_t, str[0]);
	}
	char32_t c = str[0] & 0b00000111;
	for (size_t i = 1; i < cu_type.val; ++i) {
		c <<= 6;
		c |= str[i] & 0b00111111;
	}
	return RESOK(char32_t, c);
}

RESULT(size_t) esc_cptomb(char8_t* dest, char32_t c)
{
	// Original from https://www.lua.org/source/5.5/lobject.c.html#luaO_utf8esc
	// Thanks to halalaluyafail3 on the Together C & C++ discord server!

	if (c > 0x10FFFF || (c >= 0xd800 && c <= 0xdfff)) { // Invalid UTF-8 ranges
		return RESERR(size_t, ESC_ERR_UNICODE_CP_INVALID);
	}

	if (ISASCII(c)) {
		dest[0] = c;
		return RESOK(size_t, 1);
	}
	_BitInt(3) n = 1;          // number of bytes put in buffer (backwards)
	uint32_t mfb = 0b00111111; // maximum that fits in first byte
	do {
		dest[n++ - 1] = 0b10000000 | (c & 0b00111111);
		c   >>= 6;  // remove added bits
		mfb >>= 1;  // now there is one less bit available in first byte
	} while (c > mfb); // still needs continuation byte?
	dest[n - 1] = (~mfb << 1) | c;  // add first byte
	// We cannot know how many UTF-8 cu's there are without doing another pass. Reversing never more than 4 bytes is fast anyways :p
	for (_BitInt(3) i = 0; i < n - 1; i++) {
		const char8_t temp = dest[i];
		dest[i] = dest[n - 1 - i];
		dest[n - 1 - i] = temp;
	}

	return RESOK(size_t, n);
}

