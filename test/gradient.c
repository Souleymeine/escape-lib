#include <stdint.h>
#include <stdio.h>

#include "screen.h"
#include "terminal.h"

int main(int argc, char* argv[])
{
	init_flags(TERM_NO_ECHO | TERM_HIDE_CURSOR | TERM_ALTBUF);
	const char* gphm = (argc == 1) ? "é" : argv[1];

	const struct termsize size = get_termsize();
	for (uint16_t y = 1; y <= size.rows; ++y) {
		for (uint16_t x = 1; x <= size.cols; ++x) {
			setgphm(gphm, x, y);
			setclrpair(CLR_RGB(x * 255 / size.cols, 0, y * 255 / size.rows), CLR_CODE(CLRCODE_YELLOW), x, y);
		}
	}
	refresh();

	getchar();
	cleanup_term();
}

