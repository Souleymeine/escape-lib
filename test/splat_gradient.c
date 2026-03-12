#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "../include/screen.h"
#include "../include/terminal.h"
#include "../include/utf.h"

int main(int argc, char* argv[])
{
	init_flags(TERM_NO_ECHO | TERM_HIDE_CURSOR | TERM_ALTBUF);
	const char32_t cp = (argc == 1) ? U'é' : strtocp(UTF8(argv[1]));

	const struct termsize size = get_termsize();
	const float increment      = 0.05f;
	float progress             = 0;
	do {
		for (uint16_t y = 1; y <= size.rows; ++y) {
			for (uint16_t x = 1; x <= size.cols; ++x) {
				setcp(cp, x, y);
				setclrpair(CLR_RGB(x * 255 / size.cols, y * 255 / size.rows, ((sin(progress) + 1) / 2) * 255),
				           CLR_CODE(CLRCODE_YELLOW), x, y);
			}
		}
		progress += increment;
		refresh(false);
	} while (getchar() != '\x1b');


	cleanup_term();
	return 0;
}
