#include <stdio.h>

#include "screen.h"
#include "terminal.h"

int main()
{
	init_flags(TERM_ALTBUF | TERM_NO_ECHO | TERM_HIDE_CURSOR);

	setgphm("┌", 5, 5);
	setgphm("a", 15, 8);
	refresh();

	getchar();
	cleanup_term();
}

