#include <stdio.h>
#include <stdlib.h>

#include "../include/escseq.h"

int main(int argc, char* argv[])
{
	const unsigned int a = (argc == 1) ? 99 : atoi(argv[1]);
	printf("%u has %d digits\n", a, cntdigits(a));

	char seq[100];
	const size_t len = seqcat(seq, (struct seqel[]){SEQ_U8(254), SEQ_STRL("baaaaaaa"), SEQ_U16(25400), SEQ_CHR('!')}, 4);
	printf("%.*s\n", (int)len, seq);

	char paramseq[100];
	const size_t paramseqlen = paramu8seq(paramseq, (uint8_t[]){48, 2, 125, 50, 25}, 5, 'm');
	printf("%.*s\n", (int)paramseqlen, paramseq);
}

