#pragma once

/**
 * Syntax sugar, for INTERNAL USE ONLY.
 * For
 * INTERNAL.
 * USE.
 * ONLY.
 * THOSE SHOULD NEVER BE USED IN HEADER FILES WHERE FUNCTION DECLARATIONS LIVE!!! (otherwise github CI won't pass)
 * More seriously however, being picky about types makes me have a headache when I have 4 "const unsigned short" in a row.
 * Just let that be "const ushort", types were not meant to be sentences, IMHO.
 * It genuinely helps me focus and reduce cognitive load and having to type '_t' every time
 * I specify a size is really anoying over time. This really should be the only syntax sugar allowed internally.
 * Everyone should be able to read the library's code by simply knowing C, nothing more.
 *
 * Don't typedef structs, unions or enums, or else I will find you and make you discard your commits.
 * Anyway, have fun.
 *
 *		Souleymeine
 */

// Copied from https://github.com/Souleymeine/shortypes.h/blob/main/shortypes.h

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <uchar.h>


// Variable size types

typedef intptr_t isize;
typedef size_t usize;

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long uloong;
typedef long long loong;


// Fixed size types

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef char8_t c8;
typedef char16_t c16;
typedef char32_t c32;

#if __GNUC__ || __clang__
typedef _Float16 f16;
typedef __float128 f128;
#endif
typedef float f32;
typedef double f64;
typedef long double f80;


// Statically check type sizes to catch weird compiler implementation specific bugs

#define ASSERT_SIZE_MSG(type, expected_size) \
	static_assert(sizeof(type) == expected_size, "type " #type " has a different size than expected (" #expected_size " bytes).")
#define ASSERT_LEAST_SIZE_MSG(type, expected_size)                                                        \
	static_assert(sizeof(type) >= expected_size,                                                          \
	              "type " #type " has a size smaller than expected (at least " #expected_size " bytes).")
#define ASSERT_EQ_SIZE_MSG(type1, type2)                                                                  \
	static_assert(sizeof(type1) == sizeof(type2),                                                         \
	              "type " #type1 " has a different size than type " #type2 ", which should be the same.")

#if __GNUC__ || __clang__
ASSERT_SIZE_MSG(f16, 2);
ASSERT_SIZE_MSG(f128, 16);
#endif
ASSERT_SIZE_MSG(f32, 4);
ASSERT_SIZE_MSG(f64, 8);
ASSERT_LEAST_SIZE_MSG(f80, 10);
ASSERT_EQ_SIZE_MSG(isize, usize);

