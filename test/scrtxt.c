#include <stdio.h>

#include "../include/screen.h"
#include "../include/terminal.h"

int main()
{
	init_flags(TERM_ALTBUF | TERM_NO_ECHO | TERM_HIDE_CURSOR);

	setcp(U'é', 5, 5);
	setcp(U'┌', 6, 5);
	setcp(U'a', 15, 8);
	setvis(false, 5, 5);

	setclrpair(CLR_RGB(10, 20, 50), CLR_CODE(31), 15, 8);
	setfgclr(CLR_RGB(125, 25, 50), 10, 8);
	refresh(true);

	getchar();
	cleanup_term();
	return 0;
}

