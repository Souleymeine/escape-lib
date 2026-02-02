#include <stdio.h>
#include <stdlib.h>

#include "esqsec.h"

int main(int argc, char* argv[])
{
	const unsigned int a = (argc == 1) ? 99 : atoi(argv[1]);
	printf("%u has %d digits\n", a, cntdigits(a));

	char seq[100];
	const size_t len = seqcat(seq, (struct seqslice[]){F_U8(254), F_STRL("baaaaaaa"), F_U16(25400), F_CHR('!')}, 4);
	printf("%.*s\n", (int)len, seq);
}

