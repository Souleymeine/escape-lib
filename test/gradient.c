#include <stdint.h>
#include <stdio.h>

#define ESC_SHORTHAND
#include "../include/rndr.h"
#include "../include/core.h"

int main(int argc, char* argv[])
{
	CATCH(esc_init(OPTSOME(uint16_t, ESC_TERM_NOECHO | ESC_TERM_NOCURSOR | ESC_TERM_ALTBUF)), err,
		fprintf(stderr, "Error code (esc_init): %d\nexiting.", err);
		return 1;
	);
	static char strbuf[1024 * 4];
	CATCH(esc_initscr(ESC_STRBUF_RING_STACK(strbuf), false, ESC_CLRCODE(ESC_CLRCODE_DEF), ESC_CLRCODE(ESC_CLRCODE_DEF)), err,
		fprintf(stderr, "Error code (esc_initscr): %d\nexiting.", err);
		return 1;
	);

	const RESULT(char32_t) arg_cp = esc_mbtocp(ESC_UTF8(argv[1]));
	CATCH(arg_cp, err,
		fprintf(stderr, "couldn't parse 1st arg as Unicode a codepoint, error code (esc_mbtocp): %d\nexiting", err);
		return 1;
	);
	const char32_t cp = (argc == 1) ? U'é' : arg_cp.val;

	const struct esc_termsize size = esc_gettermsize().val;
	for (uint16_t y = 0; y < size.rows; y++) {
		for (uint16_t x = 0; x < size.cols; x++) {
			(void)esc_setcp(cp, x, y);
			(void)esc_setclrpair(ESC_CLRRGB(x * 255 / size.cols, y * 255 / size.rows, 0), ESC_CLRCODE(ESC_CLRCODE_CYAN), x, y);
		}
	}
	CATCH(esc_refresh(false), err, fprintf(stderr, "Error code (esc_refresh): %d", err));

	getchar();
	esc_cleanup();
	return 0;
}

