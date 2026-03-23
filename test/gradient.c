#include <stdint.h>
#include <stdio.h>

#include "../include/rndr.h"
#include "../include/core.h"

int main(int argc, char* argv[])
{
	esc_init();
	(void)esc_settermflags(ESC_TERM_NO_ECHO | ESC_TERM_HIDE_CURSOR | ESC_TERM_ALTBUF);
	(void)esc_initscr(ESC_STRBUF_IMPL_GROWABLE(2048, 0.5f), false, ESC_CLR_CODE(ESC_CLRCODE_DEF), ESC_CLR_CODE(ESC_CLRCODE_DEF));

	const char32_t cp = (argc == 1) ? U'é' : esc_mbtocp(ESC_UTF8(argv[1])).val;

	const struct esc_termsize size = esc_getsize();
	for (uint16_t y = 0; y < size.rows; y++) {
		for (uint16_t x = 0; x < size.cols; x++) {
			(void)esc_setcp(cp, x, y);
			(void)esc_setclrpair(ESC_CLR_RGB(x * 255 / size.cols, y * 255 / size.rows, 0), ESC_CLR_CODE(ESC_CLRCODE_CYAN), x, y);
		}
	}
	(void)esc_refresh(false);

	getchar();
	esc_cleanup();
	return 0;
}

