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

	const RESULT(struct esc_cellsize) font = esc_getcellsize();
	CATCH(font, err,
		fprintf(stderr, "esc_getcellsize failed? error code: %d\n", err);
		return 1;
	);
	printf("cellx: %d, celly: %d\nratio: %0.2f\n", font.val.xpix, font.val.ypix, (float)font.val.ypix / font.val.xpix);
	return 0;
}
