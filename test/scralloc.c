#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "screen.h"
#include "termsize.h"

static void test_scr(screen* scr, const char* msg)
{
	printf("Trying %s ...\n", msg);
	if (scr == nullptr) {
		fprintf(stderr, "Call to newscr failed, scr is null\n");
	}
	printf("Call to newscr succeded! Address : %p\nPress enter to de-allocate...\n", (void*)scr);

	getchar();

	if (freescr(scr)) {
		fprintf(stderr, "Call to freescr failed, value of errno : %d\n\n", errno);
	} else {
		printf("Call to freescr succeded!\n\n");
	}
}

int main()
{
	const struct termsize size = get_termsize();
	printf("\n-------------------- test --------------------\nsize : %d x %d\n", size.cols, size.rows);
	newscr(DEF_SCR_BGCLR, DEF_SCR_FGCLR, SCREEN_USE_VIRTUAL);
	screen* no_vscr = newscr(DEF_SCR_BGCLR, DEF_SCR_FGCLR, 0);
	test_scr(no_vscr, "screen without virtual screen");
	screen* with_vscr = newscr(DEF_SCR_BGCLR, DEF_SCR_FGCLR, SCREEN_USE_VIRTUAL);
	test_scr(with_vscr, "screen with virtual screen");

	printf("Creating core dump (if your system is setup correctly), program will now crash.\n");
	abort();

	return 0;
}

