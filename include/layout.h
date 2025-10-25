#pragma once

#include <stdint.h>

#include "escdef.h"
#include "screen.h"

enum ENUMTYPE(h_offpref, char) {
	PREF_LEFT  = -1,
	PREF_RIGHT = 1,
};
enum ENUMTYPE(v_offpref, char) {
	PREF_TOP    = -1,
	PREF_BOTTOM = 1,
};

/* h_coordsems and v_coordsems MUST NOT overlap. */
enum ENUMTYPE(h_cordsems, unsigned char) {
	LEFT     = 0x1,
	RIGHT    = 0x2,
	H_MIDDLE = 0x4,
};

enum ENUMTYPE(v_cordsems, unsigned char) {
	TOP      = 0x8,
	BOTTOM   = 0x10,
	V_MIDDLE = 0x20,
};

struct anchor {
	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;
	enum h_cordsems h_coordsem;
	enum v_cordsems v_coordsem;
};

void ref_topleft(const struct anchor* anc, enum h_offpref h_off, enum v_offpref v_off, uint16_t* restrict x,
                 uint16_t* restrict y);

void clampcords(const screen* scr, uint16_t* restrict x, uint16_t* restrict y);

