#pragma once

struct termsize {
	unsigned int cols, rows, xpix, ypix;
};

struct ro_termsize {
	const unsigned int cols, rows, xpix, ypix;
};

void ref_termsize(struct termsize* ref);

struct ro_termsize get_termsize();

