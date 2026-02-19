#pragma once

#include <stdint.h>

struct termsize {
	uint16_t cols;
	uint16_t rows;
	uint16_t xpix;
	uint16_t ypix;
};

void ref_termsize(struct termsize* ref);

struct termsize get_termsize();

