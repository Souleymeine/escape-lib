#include <stdio.h>

#include "escdef.h"
#include "grapheme.h"
#include "screen.h"
#include "terminal.h"

int main()
{
	init_flags(ALTBUF | NO_ECHO | HIDE_CURSOR);
	setc32(gphmtoc32(u8"é"), 5, 5);
	refresh();
	getchar();
	cleanup_term();
}

