#include <stdio.h>

#define ESC_SHORTHAND
#include "../include/core.h"
#include "../include/rndr.h"

int main()
{
	CATCH(esc_init(OPTSOME(uint16_t, ESC_TERM_NOECHO | ESC_TERM_NOCURSOR | ESC_TERM_ALTBUF)), err,
		fprintf(stderr, "Error code (esc_init): %d\nexiting.", err);
		return 1;
	);
	CATCH(esc_initscr(ESC_STRBUF_RING_HEAP(1024), false, ESC_CLRCODE(ESC_CLRCODE_DEF), ESC_CLRCODE(ESC_CLRCODE_DEF)), err,
		fprintf(stderr, "Error code (esc_initscr): %d\nexiting.", err);
		return 1;
	);

	(void)esc_setcp(U'é', 5, 5);
	(void)esc_setcp(U'┌', 6, 5);
	(void)esc_setcp(U'a', 15, 8);
	(void)esc_setvis(false, 5, 5);
	(void)esc_setclrpair(ESC_CLRRGB(10, 20, 50), ESC_CLRCODE(31), 15, 8);
	(void)esc_setfgclr(ESC_CLRRGB(125, 25, 50), 10, 8);

	CATCH(esc_refresh(false), err, fprintf(stderr, "Error code (esc_refresh): %d", err));

	getchar();
	esc_cleanup();
	return 0;
}

