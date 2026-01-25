#include <stdio.h>

#include "esqsec.h"

int main()
{
	printf("99 has %d digits\n", cntdigits(99));

	const struct seqslice slices[] = {SEQ_STR("Abc"), SEQ_UINT(105)};

	const size_t len = seqlen(slices, 2);
	char seq[len];
	seqcat(seq, slices, 2);

	printf("%.*s has %lu bytes\n", (int)len, seq, len);
}

