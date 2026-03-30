#include <stdio.h>
#include <stdlib.h>

#include "../include/core.h"
#include "../include/rndr.h"

int main(int argc, char* argv[])
{
	const unsigned int a = (argc == 1) ? 99 : atoi(argv[1]);
	printf("%u has %d digits\n", a, esc_cntdigits(a));

	char8_t seq[100];
	const size_t len = esc_seqcat(seq, ESC_ARRARG(struct esc_seqel, {
		ESC_SEQU8(254), ESC_SEQSTRL("baaaaaaa"), ESC_SEQU16(25400), ESC_SEQCHR('!')
	}));
	printf("%.*s\n", (int)len, seq);

	// char8_t paramseq[ESC_U8_WORST_PARAMSEQ_LEN(5)];
	// const size_t paramseqlen = esc_u8paramseq(paramseq, ESC_ARRARG(uint8_t, { 48, 2, 125, 50, 25 }), 'm');
	// printf("%.*s\n", (int)paramseqlen, paramseq);

	char32_t cp = (argc == 2) ? esc_mbtocp(ESC_UTF8(argv[1])).val : U'é';
	char8_t utf8[4];
	const ESC_RESULT(size_t) cu_count = esc_cptomb(cp, utf8);
	if (cu_count.err) {
		fprintf(stderr, "Bad codepoint\n");
	}
	printf("%.*s is made of %zu bytes\n", (int)cu_count.val, utf8, cu_count.val);

	return 0;
}

