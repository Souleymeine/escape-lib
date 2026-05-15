#include <stdint.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#define ESC_SHORTHAND
#include "../include/rndr.h"
#include "../include/core.h"

int main()
{
	CATCH(esc_init(OPTSOME(uint16_t, ESC_TERM_NOECHO | ESC_TERM_NOCURSOR | ESC_TERM_ALTBUF | ESC_TERM_NOLINEBUFFERING)), err,
		fprintf(stderr, "Error code (esc_init): %d\nexiting.", err);
		return 1;
	);
	static char strbuf[1024 * 4];
	CATCH(esc_initscr(ESC_STRBUF_RING_STACK(strbuf), false, ESC_CLRCODE(ESC_CLRCODE_DEF), ESC_CLRCODE(ESC_CLRCODE_DEF)), err,
		fprintf(stderr, "Error code (esc_initscr): %d\nexiting.", err);
		return 1;
	);

    size_t max_iteration = 100;
	const struct esc_termsize size = esc_gettermsize().val;
	bool done = false;
	while (!done) {
		for (uint16_t cy = 0; cy < size.rows; cy++) {
			for (uint16_t cx = 0; cx < size.cols; cx++) {
				const double x0 = (double)cx / size.cols * 2.47 - 2;
				const double y0 = (double)cy / size.rows * 2.24 - 1.12;
				size_t iteration = 0;
				for (double x = 0, y = 0; x * x + y * y < 4 && iteration < max_iteration; iteration++) {
					const double xtemp = x * x - y * y + x0;
					y = 2 * x * y + y0;
					x = xtemp;
				}
				const uint8_t clr = iteration * 255 / max_iteration;
				(void)esc_setbgclr(ESC_CLRRGB(clr, clr, clr), cx, cy);
			}
		}
		CATCH(esc_refresh(false), err, fprintf(stderr, "Error code (esc_refresh): %d", err));
		switch (getchar()) {
			case '+': max_iteration++; break;
			case '-': if (max_iteration > 1) max_iteration--; break;
			case '\x1b': done = true; break;
		}
	};

	esc_cleanup();
	return 0;
}

