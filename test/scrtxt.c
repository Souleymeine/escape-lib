#include <stdio.h>

#include "screen.h"
#include "terminal.h"

int main()
{
	init_flags(TERM_ALTBUF | TERM_NO_ECHO | TERM_HIDE_CURSOR);
	setgphm("é", 5, 5);
	refresh();
	getchar();
	cleanup_term();
}

