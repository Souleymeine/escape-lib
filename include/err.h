#pragma once

#include <stddef.h>
#include <uchar.h>

// What you're looking for is probably below that big block

/** ------ Macro magic start (scary) -----
 * Thanks to raccoon7378 on the Zig programming language Discord Server!
 * original link : https://c.godbolt.org/z/TdvrYW53M
 * TODO : figure out how it works precisely */
#define _ESC_CAT(a,b)___ESC___CAT(a,b)
#define ___ESC___CAT(a,b)a##b
#define ___ESC_EMPTY(...)
#define ___ESC_ARG_2(a, b, ...) b
#define ___ESC_PROBE(x) x, 1, ~
#define ___ESC_IS_PROBE(...) ___ESC_ARG_2(__VA_ARGS__, 0)
#define ___ESC_ZERO(x) ___ESC_IS_PROBE(_ESC_CAT(___ESC___ZERO_, x))
#define ___ESC___ZERO_0 ___ESC_PROBE(~)
#define ___ESC_IF(c) _ESC_CAT(___ESC_IF_, ___ESC_ZERO(___ESC_ZERO(c)))
#define ___ESC_IF_1(...) __VA_ARGS__ ___ESC_EMPTY
#define ___ESC_IF_0(...) ___ESC_IF_0_1
#define ___ESC_IF_0_1(...) __VA_ARGS__
#define ___ESC_IS_VOID(...) ___ESC_IS_PROBE(_ESC_CAT(___ESC_IS_VOID_, __VA_ARGS__))
#define ___ESC_IS_VOID_void ___ESC_PROBE(~)
#define _ESC_IF_NOT_VOID(T) ___ESC_IF(___ESC_ZERO(___ESC_IS_VOID(T)))
// Thanks to holyblackcat on Discord!!
#define ESC_RESULT_TYPENAME(T) _ESC_CAT(escres_,ESC_TYPEID(T))
#define ESC_RESULT_TYPENAME_PTR(T) _ESC_CAT(escres_,ESC_TYPEID_PTR(T))
#define ESC_TYPEID_PTR(name) _ESC_CAT(ESC_TYPEID(name),_ptr)
#define ESC_TYPEID(name) ___ESC_TYPEID_(___ESC_TYPEID_(___ESC_TYPEID_(___ESC_TYPEID_(name)))) // types up to 4 words such as unsigned long long int
#define ___ESC_TYPEID_(name_) ___ESC_DETAIL_TYPEID_SELECT(___ESC_DETAIL_TYPEID_ANALYZE(name_), name_, ___ESC_DETAIL_TYPEID_ELABORATED, ___ESC_DETAIL_TYPEID_NOT_ELABORATED, x)

#define ___ESC_DETAIL_TYPEID_ANALYZE(name_) ___ESC_DETAIL_TYPEID_ANALYZE_##name_
#define ___ESC_DETAIL_TYPEID_ANALYZE_unsigned u,
#define ___ESC_DETAIL_TYPEID_ANALYZE_struct struct_,
#define ___ESC_DETAIL_TYPEID_ANALYZE_union union_,
#define ___ESC_DETAIL_TYPEID_ANALYZE_enum enum_,
// Make aliases unique
#define ___ESC_DETAIL_TYPEID_ANALYZE_u uint,
#define ___ESC_DETAIL_TYPEID_ANALYZE_long long,
#define ___ESC_DETAIL_TYPEID_ANALYZE_longint long,
#define ___ESC_DETAIL_TYPEID_ANALYZE_longlong longlong,
#define ___ESC_DETAIL_TYPEID_ANALYZE_longlongint longlong,
#define ___ESC_DETAIL_TYPEID_ANALYZE_ulong ulong,
#define ___ESC_DETAIL_TYPEID_ANALYZE_ulongint ulong,
#define ___ESC_DETAIL_TYPEID_ANALYZE_ulonglong ulonglong,
#define ___ESC_DETAIL_TYPEID_ANALYZE_ulonglongint ulonglong,
#define ___ESC_DETAIL_TYPEID_ANALYZE_short short,
#define ___ESC_DETAIL_TYPEID_ANALYZE_shortint short,
#define ___ESC_DETAIL_TYPEID_ANALYZE_ushort ushort,
#define ___ESC_DETAIL_TYPEID_ANALYZE_ushortint ushort,
#if defined(_MSC_VER) && !defined(__clang__) && (!defined(_MSVC_TRADITIONAL) || _MSVC_TRADITIONAL == 1) // If using legacy MSVC preprocessor...
#  define ___ESC_DETAIL_TYPEID_EMPTY
#  define ___ESC_DETAIL_TYPEID_SELECT(...) ___ESC_DETAIL_TYPEID_SELECT_ ___ESC_DETAIL_TYPEID_EMPTY(__VA_ARGS__)
#else
#  define ___ESC_DETAIL_TYPEID_SELECT(...) ___ESC_DETAIL_TYPEID_SELECT_(__VA_ARGS__)
#endif
#define ___ESC_DETAIL_TYPEID_SELECT_(unused_or_prefix_, name_, unused_, func_, ...) func_(unused_or_prefix_, name_)
#define ___ESC_DETAIL_TYPEID_NOT_ELABORATED(unused_, name_) name_
#define ___ESC_DETAIL_TYPEID_ELABORATED(prefix_, name_) prefix_##name_
/* ------ Macro magic end ------ */

#if __STDC_VERSION__ >= 202311L
#define _ESC_NODISCARD(reason) [[nodiscard(reason)]]
#elif
#define _ESC_NODISCARD(reason)
#endif

#define _ESC_RESULT_DECL(T) struct _ESC_NODISCARD("Error ignored") \
ESC_RESULT_TYPENAME(T) {                      \
	const enum escerror err;                  \
	_ESC_IF_NOT_VOID(T)(const T val;)()       \
}
#define _ESC_RESULT_PTR_DECL(T) struct _ESC_NODISCARD("Pointer error ignored") \
ESC_RESULT_TYPENAME_PTR(T) {                  \
	const enum escerror err;                  \
	T* val;                                   \
}

#define ESC_OK(res) (!res.err)
enum escerror : unsigned {
// global
	ESC_ERR_OOM = 1,
// core
	ESC_ERR_TERMFLAGS_ALREADY_SET,
	ESC_ERR_TERMWRITE_FAILED,
	ESC_ERR_TERMWRITE_TRUNCATED,
// io
// rndr
	ESC_ERR_INVALID_UTF8_CU,
	ESC_ERR_NO_INVALID_UTF8_CU,
	ESC_ERR_INVALID_UNICODE_CODEPOINT,
	ESC_ERR_CELL_X_OOB,
	ESC_ERR_CELL_Y_OOB,
	ESC_ERR_CELL_XY_OOB,
	ESC_ERR_CELL_INDEX_OOB,
// 2d
// img
// 3d
};

/**
 * List of all the result type declarations for stdc types used in the library
 * This is done upfront so _ESC_RESULT_DECL is only used for structs, enums and unions,
 * placed after the declaration.
 */
_ESC_RESULT_PTR_DECL(char);
_ESC_RESULT_PTR_DECL(char8_t);
_ESC_RESULT_PTR_DECL(char32_t);
_ESC_RESULT_PTR_DECL(void);
_ESC_RESULT_DECL(void);
_ESC_RESULT_DECL(size_t);
_ESC_RESULT_DECL(char8_t);
_ESC_RESULT_DECL(char32_t);

#define ESC_RESULT(T)     struct ESC_RESULT_TYPENAME(T)
#define ESC_RESULT_PTR(T) struct ESC_RESULT_TYPENAME_PTR(T)

// Variadics to remove the necessity of parenthesis when using inline structs
#define ESC_RESVAL(T, ...) (ESC_RESULT(T)) {.val = __VA_ARGS__, .err = 0}
#define ESC_RESERR(T, ...) (ESC_RESULT(T)) {.err = __VA_ARGS__}
#define ESC_RESNOERR(T)    (ESC_RESULT(T)) {.err = 0}

#define ESC_RESPTRVAL(T, ...) (ESC_RESULT_PTR(T)) {.val = __VA_ARGS__, .err = 0}
#define ESC_RESPTRERR(T, ...) (ESC_RESULT_PTR(T)) {.err = __VA_ARGS__}
#define ESC_RESPTRNOERR(T)    (ESC_RESULT_PTR(T)) {.err = 0}

#if __STDC_VERSION__ >= 202311L
#define ESC_TRY(T, res)                            \
	do {                                           \
		const auto r = res;                        \
		if (r.err) return ESC_RESERR(T, r.err);    \
	} while (false)
#define ESC_TRY_PTR(T, res)                        \
	do {                                           \
		const auto r = res;                        \
		if (r.err) return ESC_RESPTRERR(T, r.err); \
	} while (false)
#else
#define ESC_TRY(T, res)                              \
	do {                                             \
		const __typeof(x)__(res) r = res;            \
		if (r.err) return ESC_RESERR(T, r.err);      \
	} while (false)
#define ESC_TRY_PTR(T, res)                          \
	do {                                             \
		const __typeof(x)__(res) r = res;            \
		if (r.err)   return ESC_RESPTRERR(T, r.err); \
	} while (false)
#endif
