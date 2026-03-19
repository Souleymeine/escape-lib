#pragma once

/**
 * escres_T is a way to give the struct a unique name for each T but is not intended to be written as such,
 * altough nothing prevents you from writing
 * escres_int32_t instead of ESC_RESULT(int32_t) or
 * escres_struct_foo instead of ESC_RESULT(struct foo).
 */
#define ESC_RESULT(T) struct ESC_RESULT_TYPENAME(T) { \
	const enum escerror err;                          \
	_ESC_IF_NOT_VOID(T)(const T val;)()               \
}
#define ESC_RESULT_PTR(T) struct ESC_RESULT_TYPENAME_PTR(T) { \
	const enum escerror err;                                  \
	T* val;                                                   \
}

#define ESC_RES_VAL(T, v) (struct ESC_RESULT_TYPENAME(T)) {.val = v, .err = 0}
#define ESC_RES_ERR(T, e) (struct ESC_RESULT_TYPENAME(T)) {.err = e}
#define ESC_RES_NOERR(T)  (struct ESC_RESULT_TYPENAME(T)) {.err = 0}

#define ESC_RESPTR_VAL(T, v) (struct ESC_RESULT_TYPENAME_PTR(T)) {.val = v, .err = 0}
#define ESC_RESPTR_ERR(T, e) (struct ESC_RESULT_TYPENAME_PTR(T)) {.err = e}
#define ESC_RESPTR_NOERR(T)  (struct ESC_RESULT_TYPENAME_PTR(T)) {.err = 0}

#define ESC_TRY(T, res)     if (res.err) return ESC_RES_ERR(T, res.err)
#define ESC_TRY_PTR(T, res) if (res.err) return ESC_RESPTR_ERR(T, res.err)

#if __STDC_VERSION__ >= 202311L
#define _ESC_NODISCARD(reason) [[nodiscard(reason)]]
#elif
#define _ESC_NODISCARD(reason)
#endif

#define ESC_OK(res) (!res.err)
enum escerror : unsigned {
// global
	ESC_ERR_OUT_OF_MEMORY = 1,
	ESC_ERR_CANNOT_FREE_MEMORY,
// io
// rndr
	ESC_ERR_INVALID_UTF8_CU,
	ESC_ERR_X_OUT_OF_BOUNDS,
	ESC_ERR_Y_OUT_OF_BOUNDS,
	ESC_ERR_XY_OUT_OF_BOUNDS,
// 2d
// img
// 3d
};


/** ------ Macro magic start (scary) -----
 *
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
/* 
 * ESC_TYPEID(x)          -> x
 * ESC_TYPEID(unsigned x) -> ux
 * ESC_TYPEID(struct x)   -> struct_x
 * ESC_TYPEID(union x)    -> union_x
 * ESC_TYPEID(enum x)     -> enum_x
 */
// types up to 4 words such as unsigned long long int
#define ESC_RESULT_TYPENAME(T) _ESC_CAT(escres_,ESC_TYPEID(T))
#define ESC_RESULT_TYPENAME_PTR(T) _ESC_CAT(escres_,ESC_TYPEID_PTR(T))

#define ESC_TYPEID_PTR(name) _ESC_CAT(ESC_TYPEID(name),_ptr)
#define ESC_TYPEID(name) ___ESC_TYPEID_(___ESC_TYPEID_(___ESC_TYPEID_(___ESC_TYPEID_(name)))) 
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
