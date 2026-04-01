#include <stdio.h>

#include "../include/core.h"

int main()
{
	const struct esc_termsize size = esc_gettermsize();
	printf("cols: %d, rows: %d\nwidth: %dpx, heigh: %dpx\n", size.cols, size.rows, size.xpix, size.ypix);
	return 0;
}
