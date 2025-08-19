#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#if _WIN32
#include <conio.h>
#include <windows.h>
#elif __unix__
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

#include "grapheme.h"
#include "terminal.h"
#include "termsize.h"

#if !(_WIN32)
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif


#define LEFT  0
#define RIGHT 1

#define TOP    0
#define BOTTOM 1

const bool HORIZ_MARGIN_OF_ERR_ALIGN = LEFT;
const bool VIRT_MARGIN_OF_ERR_ALIGN  = BOTTOM;


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
		printf(i % 2 == 0 ? "\x1b[48;5;232m \x1b[m" : " ");
	}
	printf("\x1b[H"); // move cursor to home position

	printf("cols : %hu, rows: %hu\n", size.cols, size.rows);

	/* NOTE : utf8 commandline argument on Windows requires powershell/cmd.exe to use the 65001 code
	 * page, which needs to be set before the program is executed, obviously, but it is not the
	 * default, so parsing argument with characters outside the ASCII range will lead to some weird
	 * bugs. Interesting stack overflow post on the subject :
	 * https://stackoverflow.com/questions/57131654/using-utf-8-encoding-chcp-65001-in-command-prompt-windows-powershell-window
	 */
	const char* label              = (argc == 1 ? "Всем привет !" : argv[1]);
	const size_t label_cpt_count   = strlen(label);
	const int label_grapheme_count = count_utf8_graphemes(label, label_cpt_count);
	if (label_grapheme_count == -1) {
		return -1;
	}

	/* When the parity of the label's length and the number of columns isn't the same, a slight
	 * offset will be produced either left or right, due to the nature of the integer division. We
	 * look for it and check if the HORIZ_MARGIN_OF_ERR_ALIGN property is opposite to the offset We
	 * can the apply a correction of + 1 or - 1 to match the desired off-centered alignment
	 */

	unsigned short left_gap_len        = (size.cols / 2) - (label_grapheme_count / 2);
	const unsigned short right_gap_len = size.cols - (left_gap_len + label_grapheme_count);

	// -1 if left-centered, 1 if right-centered, 0 if perfectly centered
	const char align_error = left_gap_len - right_gap_len;

	if (align_error == -1 && HORIZ_MARGIN_OF_ERR_ALIGN == RIGHT) {
		left_gap_len += 1;
	} else if (align_error == 1 && HORIZ_MARGIN_OF_ERR_ALIGN == LEFT) {
		left_gap_len -= 1;
	}

	const unsigned short virt_margin_of_error
	    = ((size.rows % 2) != 0 ? 1 : VIRT_MARGIN_OF_ERR_ALIGN);
	const unsigned short middle_line = size.rows / 2 + virt_margin_of_error;

	printf("\x1b[%hu;%huH", middle_line, left_gap_len + 1); // We go to y, x (line, column)


	enum utf8cpt_type cpt_i_type = INVALID;
	uint8_t val              = 0;
	size_t cpt_i = 0, grapheme_i = 0;
	while (cpt_i < label_cpt_count) {
		val = min(232 + ((float)grapheme_i / label_grapheme_count) * 24, 255); // 8 bit color ID
		cpt_i_type = get_utf8cpt_type(label[cpt_i]);
		if (cpt_i_type == INVALID) {
			return -1;
		} else {
			if (cpt_i > 0) {
				printf("\x1b[m"); // End the sequence
			}
			printf("\x1b[38;5;%hhu;1;3m%c", val, label[cpt_i]);
			printf("%.*s", cpt_i_type - 1, label + cpt_i + 1);
			++grapheme_i;
			cpt_i += cpt_i_type;
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

