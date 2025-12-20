#include "layout.h"

#include "screen.h"

#include "_escdef.h"

/**
 * TODO : doc
 */
static inline char calc_correction(uint16_t smaller_len, uint16_t bigger_len, uchar offpref)
{
	const char offset = (bigger_len & 1) - (smaller_len & 1);
	return offset * (offset == offpref); // Return 0 if the offcenter is already matching offpref
}

static inline uint16_t calc_middle_point(uint16_t point, uint16_t len, uchar offpref)
{
	return len / 2 + calc_correction(len, point, offpref);
}

void ref_topleft(const struct anchor* anc, enum h_offpref h_off, enum v_offpref v_off, uint16_t* restrict x, uint16_t* restrict y)
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

void clampcords(const screen* scr, uint16_t* restrict x, uint16_t* restrict y)
{
	*x = CLAMP(*x, 1, scr->termsize.cols);
	*x = CLAMP(*y, 1, scr->termsize.rows);
}

