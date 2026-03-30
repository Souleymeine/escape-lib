#include <stdio.h>

#include "../include/core.h"
#include "../include/rndr.h"

int main()
{
	esc_init();
	(void)esc_settermflags(ESC_TERM_ALTBUF | ESC_TERM_NO_ECHO | ESC_TERM_HIDE_CURSOR);
	(void)esc_initscr(ESC_STRBUF_IMPL_CIRCULAR_HEAP(1024), false,
		ESC_CLR_CODE(ESC_CLRCODE_DEF), ESC_CLR_CODE(ESC_CLRCODE_DEF));

	(void)esc_setcp(U'é', 5, 5);
	(void)esc_setcp(U'┌', 6, 5);
	(void)esc_setcp(U'a', 15, 8);
	(void)esc_setvis(false, 5, 5);

	(void)esc_setclrpair(ESC_CLR_RGB(10, 20, 50), ESC_CLR_CODE(31), 15, 8);
	(void)esc_setfgclr(ESC_CLR_RGB(125, 25, 50), 10, 8);
	(void)esc_refresh(true);

	getchar();
	esc_cleanup();
	return 0;
}

