#include <stdbit.h>

#include "../include/escseq.h"

#include "../include/_escdef.h"

// From https://stackoverflow.com/questions/9721042/count-number-of-digits-which-method-is-most-efficient/9721401#9721401
uchar cntdigits(u16 n)
{
	static constexpr u16 powers[5]                     = {0, 10, 100, 1000, 10000};
	static constexpr uchar maxdigits[UINT16_WIDTH + 1] = {1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5};

	const uchar digits = maxdigits[stdc_bit_width_us(n)];
	return (n < powers[digits - 1]) ? digits - 1 : digits;
}

// Puts ASCII representation of unsigned int N into DEST and returns the number of digits of N (`cntdigits(n)`)
static size_t utostr(char* restrict dest, uint16_t n)
{
	const size_t digits = cntdigits(n);
	for (size_t i = 0; i < digits; ++i) {
		dest[digits - i - 1] = '0' + n % 10;
		n /= 10;
	}
	return digits;
}

static inline size_t strcpy(char* restrict dest, const char* restrict src, size_t n)
{
	for (size_t i = 0; i < n; ++i) {
		dest[i] = src[i];
	}
	return n;
}

size_t seqcat(char* restrict dest, const struct seqel* restrict elements, size_t n)
{
	size_t ofst = 0;
	for (size_t i = 0; i < n; ++i) {
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

static inline void parambitsbound(struct seqel* restrict elements, size_t len, char end)
{
	elements[0]       = SEQ_STRL(CSI);
	elements[len - 1] = SEQ_CHR(end);
}

size_t paramu8seq(char* restrict dest, const uint8_t* restrict params, size_t n, char end)
{
	const size_t el_cnt = PARAM_SLICE_COUNT(n);
	struct seqel elemnts[el_cnt];
	parambitsbound(elemnts, el_cnt, end);
	for (size_t i = 1; i < el_cnt - 1; ++i) {
		elemnts[i] = (i & 1) ? SEQ_U8(params[(i - 1) / 2]) : SEQ_CHR(';');
	}
	return seqcat(dest, elemnts, el_cnt);
}

size_t paramu16seq(char* restrict dest, const uint16_t* restrict params, size_t n, char end)
{
	const size_t el_cnt = PARAM_SLICE_COUNT(n);
	struct seqel elemnts[el_cnt];
	parambitsbound(elemnts, el_cnt, end);
	for (size_t i = 1; i < el_cnt - 1; ++i) {
		elemnts[i] = (i & 1) ? SEQ_U16(params[(i - 1) / 2]) : SEQ_CHR(';');
	}
	return seqcat(dest, elemnts, el_cnt);
}

