#include <stdint.h>
#include <stdio.h>

#include "screen.h"
#include "terminal.h"

int main()
{
	init_flags(TERM_NO_ECHO | TERM_HIDE_CURSOR | TERM_ALTBUF);

	const struct termsize size = get_termsize();
	for (uint16_t y = 1; y <= size.rows; ++y) {
		for (uint16_t x = 1; x <= size.cols; ++x) {
			setgphm("a", x, y);
			setclrpair(CLR_RGB(x * 255 / size.cols, 0, y * 255 / size.rows), CLR_CODE(CLRCODE_YELLOW), x, y);
		}
	}
	refresh();

	getchar();
	cleanup_term();
}

