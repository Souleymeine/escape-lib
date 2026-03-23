#include <stdio.h>
#include <stdlib.h>

#include "../include/core.h"

int main(int argc, char* argv[])
{
	const unsigned int a = (argc == 1) ? 99 : atoi(argv[1]);
	printf("%u has %d digits\n", a, esc_cntdigits(a));

	char8_t seq[100];
	const size_t len = esc_seqcat(seq, (struct esc_seqel[]){ESC_SEQ_U8(254), ESC_SEQ_STRL("baaaaaaa"), ESC_SEQ_U16(25400), ESC_SEQ_CHR('!')}, 4);
	printf("%.*s\n", (int)len, seq);

	char8_t paramseq[ESC_U8_WORST_PARAMSEQ_LEN(5)];
	const size_t paramseqlen = esc_u8paramseq(paramseq, (uint8_t[]){48, 2, 125, 50, 25}, 5, 'm');
	printf("%.*s\n", (int)paramseqlen, paramseq);

	return 0;
}

