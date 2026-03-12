#include <stdio.h>

#include "../include/termsize.h"

int main()
{
	const struct termsize size = get_termsize();
	printf("cols: %d, rows: %d\nwidth: %dpx, heigh: %dpx\n", size.cols, size.rows, size.xpix, size.ypix);
	return 0;
}
