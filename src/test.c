#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#if _WIN32
#include <stdlib.h>
#include <windows.h>
#elif __linux__
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#endif

#if !(_WIN32)
#define max(x, y) x <= y ? x : y
#endif

#define LEFT  0
#define RIGHT 1

#define TOP    0
#define BOTTOM 1

const bool HORIZ_MARGIN_OF_ERR_ALIGN = LEFT;
const bool VIRT_MARGIN_OF_ERR_ALIGN  = BOTTOM;

int main(int argc, char **argv) {
	unsigned short cols, rows;

#if _WIN32
	system("");
	CONSOLE_SCREEN_BUFFER_INFO info;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
	cols = info.dwMaximumWindowSize.X;
	rows = info.dwMaximumWindowSize.Y;
#else
	struct termios term_attr;
	tcgetattr(STDIN_FILENO, &term_attr);
	term_attr.c_lflag &= ~ECHO;
	tcsetattr(STDIN_FILENO, 0, &term_attr);

	struct winsize window;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &window); // Get the window width and height in cells (columns, lines) and in pixels
	cols = window.ws_col;
	rows = window.ws_row;
#endif

	printf("\x1b[?1049h"); // Shows alternate buffer
	printf("\x1b[?25l"); // Hides the cursor

	// Paint half the cells (for visualization purposes)
	for (int i = 0; i < rows * cols; ++i) {
		printf(i % 2 == 0 ? "\x1b[48;5;232m \x1b[m" : " ");
	}
	printf("\x1b[H"); // move cursor to home position

	printf("cols : %hu, rows: %hu\n", cols, rows);

	const char* label = (argc == 1 ? "Hello, World!" : argv[1]);
	const int label_len = strlen(label);

	// When the parity of the label's length and the number of columns isn't the same, a slight offset will be produced
	// either left or right, due to the nature of the integer division.
	// We look for it and check if the HORIZ_MARGIN_OF_ERR_ALIGN property is opposite to the offset
	// We can the apply a correction of + 1 or - 1 to match the desired off-centered alignment
	unsigned short left_gap_len = (cols / 2) - (label_len / 2); 
	const unsigned short right_gap_len = cols - (left_gap_len + label_len);
	
	const char align_error = left_gap_len - right_gap_len; // -1 if left-centered, 1 if right-centered, 0 if perfectly centered

	if (align_error == -1 && HORIZ_MARGIN_OF_ERR_ALIGN == RIGHT) {
		left_gap_len += 1;
	} else if (align_error == 1 && HORIZ_MARGIN_OF_ERR_ALIGN == LEFT) {
		left_gap_len -= 1;
	}

	const unsigned short virt_margin_of_error = ((rows % 2) != 0 ? 1 : VIRT_MARGIN_OF_ERR_ALIGN);
	const unsigned short middle_line = rows / 2 + virt_margin_of_error;

	printf("\x1b[%hu;%huH", middle_line, left_gap_len + 1); // We go to y, x (line, column)
	for (int i = 0; i < label_len; ++i) {
		uint8_t val = max(232 + ((float)i / label_len) * 24, 255); // 8 bit color ID
		printf("\x1b[38;5;%hhu;1;3m%c\x1b[m", val, label[i]); // Bold, italic and grayscaled gradient colored
	}

	getchar();
	printf("\x1b[?1049l"); // Hides the alternate buffer
	printf("\x1b[?25h"); // Shows the cursor
#if __linux__
	term_attr.c_lflag |= ECHO;
	tcsetattr(STDIN_FILENO, 0, &term_attr);
#endif
}

