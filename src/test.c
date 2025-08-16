#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "terminal.h"
#if _WIN32
#include <conio.h>
#include <windows.h>
#elif __unix__
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

#if !(_WIN32)
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

// https://stackoverflow.com/questions/8292852/the-best-way-to-define-a-between-macro-in-c
#define between(x, x_min, x_max) ((x) > (x_min) && (x) < (x_max))


#define LEFT  0
#define RIGHT 1

#define TOP    0
#define BOTTOM 1

const bool HORIZ_MARGIN_OF_ERR_ALIGN = LEFT;
const bool VIRT_MARGIN_OF_ERR_ALIGN  = BOTTOM;


typedef enum : char {
	CONTINUATION,
	ASCII,
	DOUBLE_BEGIN,
	TRIPLE_BEGIN,
	QUADRUPLE_BEGIN,
	INVALID = -1,
} utf8_char_type;


utf8_char_type get_utf8_char_type(const unsigned char c)
{
	if (c < 0x7F) return ASCII;
	else if (between(c, 0x80, 0xBF)) return CONTINUATION;
	else if (between(c, 0xC0, 0xDF)) return DOUBLE_BEGIN;
	else if (between(c, 0xE0, 0xEF)) return TRIPLE_BEGIN;
	else if (between(c, 0xF0, 0xF7)) return QUADRUPLE_BEGIN; // Unlikely but defined by utf8
	else return INVALID;
}

size_t utf8_strlen(const char* str, const size_t str_len)
{
	size_t len = 0;

	utf8_char_type uchar_i_type = INVALID;
	size_t i                    = 0;
	while (i < str_len) {
		uchar_i_type = get_utf8_char_type(str[i]);
		if (uchar_i_type == INVALID) {
			return -1;
		} else {
			++len;
			i += uchar_i_type;
		}
	}

	return len;
}

int main(int argc, char** argv)
{
	init_term();
	set_termflags(ALTBUF | HIDE_CURSOR | NO_ECHO);

	unsigned short cols, rows;
#if _WIN32
	const HANDLE* h_out = get_g_stdout_hndl();

	CONSOLE_SCREEN_BUFFER_INFO buf_info;
	GetConsoleScreenBufferInfo(*h_out, &buf_info);
	cols = buf_info.dwMaximumWindowSize.X;
	rows = buf_info.dwMaximumWindowSize.Y;

	SetConsoleOutputCP(CP_UTF8);
#else
	struct winsize window;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &window);
	cols = window.ws_col;
	rows = window.ws_row;
#endif

	// Paint half the cells (for visualization purposes)
	for (int i = 0; i < rows * cols; ++i) {
		printf(i % 2 == 0 ? "\x1b[48;5;232m \x1b[m" : " ");
	}
	printf("\x1b[H"); // move cursor to home position

	printf("cols : %hu, rows: %hu\n", cols, rows);

	/* NOTE : utf8 commandline argument on Windows requires powershell/cmd.exe to use the 65001 code
	 * page, which needs to be set before the program is executed, obviously, but it is not the
	 * default, so parsing argument with characters outside the ASCII range will lead to some weird
	 * bugs. Interesting stack overflow post on the subject :
	 * https://stackoverflow.com/questions/57131654/using-utf-8-encoding-chcp-65001-in-command-prompt-windows-powershell-window
	 */
	const char* label                 = (argc == 1 ? "Всем привет !" : argv[1]);
	const size_t label_code_point_len = strlen(label);
	const int label_code_unit_len     = utf8_strlen(label, label_code_point_len);

	// When the parity of the label's length and the number of columns isn't the same, a slight
	// offset will be produced either left or right, due to the nature of the integer division. We
	// look for it and check if the HORIZ_MARGIN_OF_ERR_ALIGN property is opposite to the offset We
	// can the apply a correction of + 1 or - 1 to match the desired off-centered alignment

	unsigned short left_gap_len        = (cols / 2) - (label_code_unit_len / 2);
	const unsigned short right_gap_len = cols - (left_gap_len + label_code_unit_len);

	// -1 if left-centered, 1 if right-centered, 0 if perfectly centered
	const char align_error = left_gap_len - right_gap_len;

	if (align_error == -1 && HORIZ_MARGIN_OF_ERR_ALIGN == RIGHT) {
		left_gap_len += 1;
	} else if (align_error == 1 && HORIZ_MARGIN_OF_ERR_ALIGN == LEFT) {
		left_gap_len -= 1;
	}

	const unsigned short virt_margin_of_error = ((rows % 2) != 0 ? 1 : VIRT_MARGIN_OF_ERR_ALIGN);
	const unsigned short middle_line          = rows / 2 + virt_margin_of_error;

	printf("\x1b[%hu;%huH", middle_line, left_gap_len + 1); // We go to y, x (line, column)


	utf8_char_type uchar_i_type = INVALID;
	uint8_t val                 = 0;
	size_t code_point_i = 0, code_unit_i = 0;

	while (code_point_i < label_code_point_len) {
		val = min(232 + ((float)code_unit_i / label_code_unit_len) * 24, 255); // 8 bit color ID
		const unsigned char uchar_i = label[code_point_i];
		uchar_i_type                = get_utf8_char_type(uchar_i);

		if (uchar_i_type == INVALID) {
			return -1;
		} else {
			if (code_point_i > 0) {
				printf("\x1b[m"); // End the sequence
			}
			printf("\x1b[38;5;%hhu;1;3m%c", val, uchar_i);
			printf("%.*s", uchar_i_type - 1, label + code_point_i + 1);
			++code_unit_i;
			code_point_i += uchar_i_type;
		}
	}

#if __unix__
	getchar();
#elif _WIN32
	while (getch() != '\r') {
		continue;
	}
#endif

	cleanup_term();
}

