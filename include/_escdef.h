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

#include <stdint.h>

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long uloong;
typedef long long loong;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/*
 * Use it only when it makes sense, when managing memory for example
 * Otherwise just use char or uchar
 */
typedef unsigned char byte;

