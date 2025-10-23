#include "layout.h"

#include "screen.h"

#include "_escdef.h"

/**
 * TODO : doc
 */
static inline char calc_correction(coord smaller_len, coord bigger_len, uchar offpref)
{
	const char offset = (bigger_len & 1) - (smaller_len & 1);
	return offset * (offset == offpref); // Return 0 if the offcenter is already matching offpref
}

static inline coord calc_middle_point(coord point, coord len, uchar offpref)
{
	return len / 2 + calc_correction(len, point, offpref);
}

void ref_topleft(const struct anchor* anc, enum h_offpref h_off, enum v_offpref v_off, coord* restrict x, coord* restrict y)
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

void clampcords(const screen* scr, coord* restrict x, coord* restrict y)
{
	*x = CLAMP(*x, 1, scr->_termsize.cols);
	*x = CLAMP(*y, 1, scr->_termsize.rows);
}
