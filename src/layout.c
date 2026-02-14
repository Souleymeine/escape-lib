#include "../include/layout.h"

#include "../include/_escdef.h"
#include "../include/screen.h"

/**
 * TODO : doc
 */
static inline char calc_correction(u16 smaller_len, u16 bigger_len, uchar offpref)
{
	const char offset = (bigger_len & 1) - (smaller_len & 1);
	return offset * (offset == offpref); // Return 0 if the offcenter is already matching offpref
}

static inline u16 calc_middle_point(u16 point, u16 len, uchar offpref) {return len / 2 + calc_correction(len, point, offpref); }

void ref_topleft(const struct anchor* anc, enum h_offpref h_off, enum v_offpref v_off, u16* restrict x, u16* restrict y)
{
	if (anc->v_coordsem == TOP && anc->h_coordsem == LEFT) {
		return; // Nothing to do
	}

	const uchar alignments = anc->h_coordsem & anc->v_coordsem;

	if (alignments & RIGHT) {
		*x -= anc->width;
	} else if (alignments & H_MIDDLE) {
		*x -= calc_middle_point(anc->x, anc->width, h_off);
	}

	if (alignments & BOTTOM) {
		*y -= anc->height;
	} else if (alignments & V_MIDDLE) {
		*x -= calc_middle_point(anc->y, anc->height, v_off);
	}
}

void clampcords(const screen* scr, u16* restrict x, u16* restrict y)
{
	*x = CLAMP(*x, 1, scr->termsize.cols);
	*x = CLAMP(*y, 1, scr->termsize.rows);
}

