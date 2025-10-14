#include <stdio.h>

#include "grapheme.h"
#include "label.h"
#include "termsize.h"

#include "_escdef.h"

#define max(x, y) ((x > y) ? x : y)

uchar label_grad8bit(const char* txt, size_t txtlen, uchar txtstyle [[maybe_unused]], u8 clr_id1, u8 clr_id2,
                     enum hor_offcenter hor_offcenter, enum vir_offcenter vir_offcenter)
{
	const ushort line_count  = 1; // Will get updated when multilne labels are supported
	const int grapheme_count = count_graphemes(txt, txtlen);
	if (grapheme_count == -1) {
		return 1;
	}

	const struct ro_termsize size = get_termsize();

	const char hor_offset = (size.cols & 1) - (grapheme_count & 1);
	const char vir_offset = (size.rows & 1) - (line_count & 1);
	const ushort left_gap = (size.cols / 2) - (grapheme_count / 2) + hor_offset * (hor_offset == hor_offcenter);
	const ushort top_gap  = (size.rows / 2) - (line_count / 2) + vir_offset * (vir_offset == vir_offcenter);

	printf("\x1b[%hu;%huH", top_gap + 1, left_gap + 1);

	const u8 clr_range         = clr_id2 - clr_id1;
	const u8 graphemes_per_clr = max(1, grapheme_count / clr_range);
	const size_t clr_scale     = max(1, grapheme_count - graphemes_per_clr);
	// Checking for invalid codepoints is redundant, count_graphemes already did it (see above)
	enum cptype type = INVALID;
	for (size_t cp_i = 0, grapheme_i = 0; cp_i < txtlen; cp_i += type, ++grapheme_i) {
		if (grapheme_i % graphemes_per_clr == 0) {
			printf("\x1b[38;5;%hhum", clr_id1 + (u8)(grapheme_i * clr_range / clr_scale));
		}
		printf("%.*s", (type = get_cptype(txt[cp_i])), txt + cp_i);
	}
	printf("\x1b[m");

	return 0;
}

