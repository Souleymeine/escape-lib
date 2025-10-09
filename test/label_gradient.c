#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "label.h"
#if _WIN32
#include <conio.h>
#include <windows.h>
#elif __unix__
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

#include "terminal.h"
#include "termsize.h"


int main(int argc, char** argv)
{
	init_term();
	set_termflags(ALTBUF | HIDE_CURSOR | NO_ECHO);

	const struct ro_termsize size = get_termsize();

#if _WIN32
	SetConsoleOutputCP(CP_UTF8);
#endif

	// Paint half the cells (for visualization purposes)
	for (unsigned int i = 0; i < size.rows * size.cols; ++i) {
		printf(i & 1 ? "\x1b[48;5;233m \x1b[m" : " ");
	}
	printf("\x1b[H"); // move cursor to home position

	printf("cols : %hu, rows: %hu\n", size.cols, size.rows);

	const char* label = (argc == 1 ? "Всем привет !" : argv[1]);
	print_8bit_label_gradient(label, strlen(label), BOLD | ITALIC, 235, 255, PREF_LEFT, PREF_TOP);

#if __unix__
	getchar();
#elif _WIN32
	while (getch() != '\r') {
		continue;
	}
#endif

	cleanup_term();
}

