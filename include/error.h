#pragma once

/**
 * Multi word types don't work here, that's intentional.
 * Short typedefs could be used to do so but that would at least introduce friction.
 * escres_T is a way to give the struct a unique name for each T but is not intended to be written as such,
 * altough nothing prevents you from writing esc_res_int32_t instead of ESC_RESULT(int32_t).
 */
#define ESC_RESULT(T) struct esc_res_##T { \
	enum esc_error err;                    \
	T val;                                 \
}

#define ESC_OK(res) (res.err == 0)

enum esc_error {
// global
	ESC_ERR_OUT_OF_MEMORY = 1,
// io
// rndr
	ESC_ERR_X_OUT_OF_BOUNDS,
	ESC_ERR_Y_OUT_OF_BOUNDS,
	ESC_ERR_XY_OUT_OF_BOUNDS,
// 2d
// img
// 3d
};

