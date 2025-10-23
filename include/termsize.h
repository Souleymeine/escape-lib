#pragma once

struct termsize {
	unsigned int cols;
	unsigned int rows;
	unsigned int xpix;
	unsigned int ypix;
};

void ref_termsize(struct termsize* ref);

struct termsize get_termsize();

