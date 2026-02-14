#include <stdbit.h>

#include "../include/_escdef.h"
#include "../include/escseq.h"

// From https://stackoverflow.com/questions/9721042/count-number-of-digits-which-method-is-most-efficient/9721401#9721401
uchar cntdigits(u16 n)
{
	static constexpr u16 powers[5]                     = {0, 10, 100, 1000, 10000};
	static constexpr uchar maxdigits[UINT16_WIDTH + 1] = {1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5};

	const uchar digits = maxdigits[stdc_bit_width_us(n)];
	return (n < powers[digits - 1]) ? digits - 1 : digits;
}

// Puts ASCII representation of unsigned int N into DEST and returns the number of digits of N (`cntdigits(n)`)
static usize utostr(char* restrict dest, u16 n)
{
	const usize digits = cntdigits(n);
	for (usize i = 0; i < digits; ++i) {
		dest[digits - i - 1] = '0' + n % 10;
		n /= 10;
	}
	return digits;
}

static inline usize strcpy(char* restrict dest, const char* restrict src, usize n)
{
	for (usize i = 0; i < n; ++i) {
		dest[i] = src[i];
	}
	return n;
}

usize seqcat(char* restrict dest, const struct seqel* restrict elements, usize n)
{
	usize ofst = 0;
	for (usize i = 0; i < n; ++i) {
		switch (elements[i].type) {
		case FMT_STR:  ofst += strcpy(dest + ofst, elements[i].str.buf, elements[i].str.len); break;
		case FMT_U16:  ofst += utostr(dest + ofst, elements[i].uint16); break;
		case FMT_U8:   ofst += utostr(dest + ofst, (u8)elements[i].uint8); break;
		case FMT_CHAR: dest[ofst++] = elements[i].chr; break;
		}
	}
	return ofst;
}

#define PARAM_SLICE_COUNT(n) n * 2 + 1 // = 1 + n + (n - 1) + 1 (CSI + params + semi-colons + end)

static inline void parambitsbound(struct seqel* restrict elements, usize len, char end)
{
	elements[0]       = SEQ_STRL(CSI);
	elements[len - 1] = SEQ_CHR(end);
}

usize paramu8seq(char* restrict dest, const u8* restrict params, usize n, char end)
{
	const usize el_cnt = PARAM_SLICE_COUNT(n);
	struct seqel elemnts[el_cnt];
	parambitsbound(elemnts, el_cnt, end);
	for (usize i = 1; i < el_cnt - 1; ++i) {
		elemnts[i] = (i & 1) ? SEQ_U8(params[(i - 1) / 2]) : SEQ_CHR(';');
	}
	return seqcat(dest, elemnts, el_cnt);
}

usize paramu16seq(char* restrict dest, const u16* restrict params, usize n, char end)
{
	const usize el_cnt = PARAM_SLICE_COUNT(n);
	struct seqel elemnts[el_cnt];
	parambitsbound(elemnts, el_cnt, end);
	for (usize i = 1; i < el_cnt - 1; ++i) {
		elemnts[i] = (i & 1) ? SEQ_U16(params[(i - 1) / 2]) : SEQ_CHR(';');
	}
	return seqcat(dest, elemnts, el_cnt);
}

