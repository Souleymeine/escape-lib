#include <stdio.h>
#include <stdlib.h>

#define ESC_SHORTHAND
#include "../include/core.h"
#include "../include/rndr.h"

int main(int argc, char* argv[])
{
	const unsigned int a = (argc == 1) ? 99 : atoi(argv[1]);
	printf("%u has %d digits\n", a, esc_digits(a));

	char8_t seq[100];
	const size_t len = esc_seqcat(seq, ARRARG(struct esc_seqel, { SEQU8(254), SEQSTRL("baaaaaaa"), SEQU16(25400), SEQCHR('!') }));
	printf("%.*s\n", (int)len, seq);

	// char8_t paramseq[ESC_U8_WORST_PARAMSEQ_LEN(5)];
	// const size_t paramseqlen = esc_u8paramseq(paramseq, ESC_ARRARG(uint8_t, { 48, 2, 125, 50, 25 }), 'm');
	// printf("%.*s\n", (int)paramseqlen, paramseq);

	const RESULT(char32_t) arg_cp = esc_mbtocp(ESC_UTF8(argv[1]));
	CATCH(arg_cp, err,
		fprintf(stderr, "couldn't parse 1st arg as Unicode a codepoint, error code (esc_mbtocp): %d\nexiting", err);
		return 1;
	);
	const char32_t cp = (argc == 1) ? U'é' : arg_cp.val;

	char8_t utf8[4];
	const RESULT(size_t) cu_count = esc_cptomb(utf8, cp);
	CATCH(cu_count, err,
		fprintf(stderr, "Bad codepoint, err code: %d\nexiting", err);
		return 1;
	);

	printf("%.*s is made of %zu bytes\n", (int)cu_count.val, utf8, cu_count.val);

	return 0;
}

