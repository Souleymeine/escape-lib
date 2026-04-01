#include <stdio.h>

#define ESC_SHORTHAND
#include "../include/core.h"

int main()
{
	const RESULT(struct esc_termsize) size = esc_gettermsize();
	CATCH(size, err,
		fprintf(stderr, "gettermsize failed, error code: %d\n", err);
		return 1;
	);
	printf("cols: %d, rows: %d\nwidth: %dpx, heigh: %dpx\n", size.val.cols, size.val.rows, size.val.xpix, size.val.ypix);
	return 0;
}
