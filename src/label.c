#include <stdio.h>

#include "grapheme.h"
#include "label.h"
#include "termsize.h"

#include "_escdef.h"

// Thanks to 'HumansAreWeak'! : https://github.com/HumansAreWeak/ilerp/blob/master/ilerp.h
static inline u8 lerp_u8(u8 delta, u8 a, u8 b)
{
	return (a * (UINT8_MAX - delta) + b * delta) / UINT8_MAX;
}

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
	const ushort top_gap  = (size.rows / 2) - (line_count / 2) + vir_offset * (vir_offset == vir_offcenter);
	const ushort left_gap = (size.cols / 2) - (grapheme_count / 2) + hor_offset * (hor_offset == hor_offcenter);

	printf("\x1b[%hu;%huH", top_gap + 1, left_gap + 1);

	const u8 clr_count         = clr_id2 - clr_id1;
	const u8 graphemes_per_clr = grapheme_count / clr_count;
	enum cptype type           = INVALID;
	// Checking for invalid codepoints is redundant, count_graphemes already did it (see above)
	for (size_t cp_i = 0, grapheme_i = 0, clr_i = 0; cp_i < txtlen; cp_i += type, ++grapheme_i) {
		if (cp_i != 0) {
			printf("\x1b[m");
		}
		// print color only if it has changed since the last grapheme
		if (graphemes_per_clr <= 1 || grapheme_i % graphemes_per_clr == 0) {
			printf("\x1b[38;5;%hhum", lerp_u8((float)clr_i / clr_count, clr_id1, clr_id2));
			++clr_i;
		}
		printf("%.*s", (type = get_cptype(txt[cp_i])), txt + cp_i);
	}

	return 0;
}

