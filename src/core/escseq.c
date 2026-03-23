#include <stdbit.h>
#include <string.h>

#include "../../include/core.h"

// From https://stackoverflow.com/questions/9721042/count-number-of-digits-which-method-is-most-efficient/9721401#9721401
uint8_t esc_cntdigits(uint16_t n)
{
	static constexpr uint16_t powers[5]                  = {0, 10, 100, 1000, 10000};
	static constexpr uint8_t maxdigits[UINT16_WIDTH + 1] = {1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5};

	const uint8_t digits = maxdigits[stdc_bit_width_uc(n)];
	return (n < powers[digits - 1]) ? digits - 1 : digits;
}

static size_t utomb(char8_t* dest, uint16_t n)
{
	const size_t digits = esc_cntdigits(n);
	for (size_t i = 0; i < digits; ++i) {
		dest[digits - i - 1] = '0' + n % 10;
		n /= 10;
	}
	return digits;
}

size_t esc_seqcat(char8_t* dest, const struct esc_seqel* elements, size_t n)
{
	size_t ofst = 0;
	for (size_t i = 0; i < n; ++i) {
		switch (elements[i].tag) {
		case ESC_FMT_STR:
			memcpy(dest + ofst, elements[i].str.buf, elements[i].str.len);
			ofst += elements[i].str.len;
			break;
		case ESC_FMT_CHR: dest[ofst++] = elements[i].uint8;               break;
		case ESC_FMT_U8:  ofst += utomb(dest + ofst, elements[i].uint8);  break;
		case ESC_FMT_U16: ofst += utomb(dest + ofst, elements[i].uint16); break;
		}
	}
	return ofst;
}

size_t esc_u8paramseq(char8_t* dest, const uint8_t* params, size_t n, char end)
{
	const size_t el_cnt = n * 2 + 1; // = 1 + n + (n - 1) + 1 (CSI + params + semi-colons + end)
	struct esc_seqel els[el_cnt];
	els[0]          = ESC_SEQ_STRL(CSI);
	els[el_cnt - 1] = ESC_SEQ_CHR(end);
	for (size_t i = 1; i < el_cnt - 1; i++) {
		els[i] = (i & 1) ? ESC_SEQ_U8(params[(i - 1) / 2]) : ESC_SEQ_CHR(';');
	}
	return esc_seqcat(dest, els, el_cnt);
}

size_t esc_u16paramseq(char8_t* dest, const uint16_t* params, size_t n, char end)
{
	const size_t el_cnt = n * 2 + 1; // = 1 + n + (n - 1) + 1 (CSI + params + semi-colons + end)
	struct esc_seqel els[el_cnt];
	els[0]          = ESC_SEQ_STRL(CSI);
	els[el_cnt - 1] = ESC_SEQ_CHR(end);
	for (size_t i = 1; i < el_cnt - 1; i++) {
		els[i] = (i & 1) ? ESC_SEQ_U16(params[(i - 1) / 2]) : ESC_SEQ_CHR(';');
	}
	return esc_seqcat(dest, els, el_cnt);
}

