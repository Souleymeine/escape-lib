#pragma once

#include <uchar.h>
#if _WIN32
#include <windows.h>
#endif

/** holds bitflags if positive, doesn't if negative */
typedef int termstatefl;

enum termflags {
	TERM_ALTBUF      = 0x1,
	TERM_HIDE_CURSOR = 0x2,
	TERM_NO_ECHO     = 0x4,
};

/* === scrflags MUST NEVER OVERLAP WITH termflags!!! === */

// IMPORTANT ! : the only requirement for this to work is for these flags
// to be bigger than at least the biggest (last) flag of `enum termflags`

/* Extends `enum termflags` */
enum scrflags {
	SCREEN_HOLD_TERMFLAGS = 0x8,
	SCREEN_USE_VIRTUAL    = 0x10,
};

enum escerr {
	ESC_OK = 0,
	ESC_ERR_COORD_X,
	ESC_ERR_COORD_Y,
	ESC_ERR_UTF8,
	ESC_ERR_CP,
};

/* Initialize some states variables and gathers information about the terminal the program is
 * running in. Required for escape sequences to work properly. */
void init_term();

/* Sets the given terminal flags via bitmask.
 * Available flags are available int `enum termflags`.
 * Returns 0 if flags were applied, -1 if it was the same as before. */
int set_termflags(termstatefl flags);

/* Available for convinience reasons :
 * Calls `init_term` then set_termflags.
 * You almost always want to do both during the initializtion of your program. */
void init_flags(termstatefl flags);

/* Returns a pointer to the program's static terminal flags. */
const termstatefl* get_termflags();

/**
 * Simple UTF-8 unbuffered cross platform terminal writer
 * Returns `true` if len doesn't match how many bytes were written or if nothing was written, false otherwise.
 * */
bool termprint(const char8_t* buf, size_t len);

/* If called bedore `init_term`/`init_flags`, stdscr will use a virtual screen by default. */
void usevscr();

#if _WIN32
/* Same as `GetStdHandle(STD_IN_HANDLE)`, but returns a pointer
 * to the already statically stored result of this exact call used internally. */
const HANDLE* get_g_stdin_hndl();
/* Same as `GetStdHandle(STD_OUT_HANDLE)`, but returns a pointer
 * to the already statically stored result of this exact call used internally. */
const HANDLE* get_g_stdout_hndl();
#endif

/* Resets the terminal with none of the flags present in `enum termflags`. */
void cleanup_term();

