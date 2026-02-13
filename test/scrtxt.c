#include <stdio.h>

#include "../include/screen.h"
#include "../include/terminal.h"

int main()
{
	init_flags(TERM_ALTBUF | TERM_NO_ECHO | TERM_HIDE_CURSOR);

	setgphm("┌", 5, 5);
	setgphm("┌", 6, 5);
	setgphm("a", 15, 8);
	refresh();
	setgphm("", 5, 5);
	clearefresh();

	setclrpair(CLR_RGB(125, 200, 50), CLR_CODE(31), 15, 8);
	setfgclr(CLR_RGB(125, 25, 50), 10, 8);
	refresh();

	getchar();
	cleanup_term();
}

