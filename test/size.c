#include <stdio.h>

#define ESC_SHORTHAND
#include "../include/core.h"

int main()
{
	const RESULT(struct esc_termsize) term = esc_gettermsize();
	CATCH(term, err,
		fprintf(stderr, "esc_gettermsize failed to retrieve kernel console pixel size (screen size), error code: %d\n", err);
		return 1;
	);
	printf("cols: %d, rows: %d\nwidth: %dpx, heigh: %dpx\n\n", term.val.cols, term.val.rows, term.val.xpix, term.val.ypix);

	const struct esc_cellsize font = esc_getcellsize().val;
	printf("cellx: %d, celly: %d\nratio: %0.2f\n", font.xpix, font.ypix, (float)font.ypix / font.xpix);
	return 0;
}
