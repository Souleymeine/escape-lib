#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "grapheme.h"
#include "label.h"
#include "termsize.h"

#if !(_WIN32)
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

int print_8bit_label_gradient(const char* txt, size_t txtlen, unsigned char txtstyle [[maybe_unused]], uint8_t clr_id1,
                              uint8_t clr_id2, enum hor_offcenter hor_offcenter, enum vir_offcenter vir_offcenter)
{
	const int grapheme_count = count_graphemes(txt, txtlen);
	if (grapheme_count == -1) {
		return -1;
	}

	const struct ro_termsize size = get_termsize();
	// -1 if left-centered, 1 if right-centered, 0 if perfectly centered
	const char hor_align_err      = (size.cols & 1) - (grapheme_count & 1);
	const unsigned short left_gap = size.cols / 2 - grapheme_count / 2 + (hor_align_err == hor_offcenter) * hor_align_err;
	// -1 if top-aligned, 1 if bottom-aligned, 0 if perfectly aligned
	const unsigned short line_count = 1; // Will get updated when multilne labels will be sopported
	const char vir_align_err        = (size.rows & 1) - (line_count & 1);
	const unsigned short top_gap    = size.rows / 2 + (vir_align_err == vir_offcenter) * vir_align_err;

	printf("\x1b[%hu;%huH", top_gap + 1, left_gap + 1); // We go to y, x (line, column)

	enum cptype type = INVALID;
	for (size_t cp = 0, grapheme = 0; cp < txtlen; cp += type, ++grapheme) {
		type = get_cptype(txt[cp]);
		if (type == INVALID) {
			return -1;
		} else {
			if (cp != 0) {
				printf("\x1b[m");
			}
			printf("\x1b[38;5;%hhu;1;3m%c%.*s", (uint8_t)min(clr_id1 + ((float)grapheme / grapheme_count) * 24, clr_id2), txt[cp],
			       type - 1, txt + cp + 1);
		}
	}

	return 0;
}

