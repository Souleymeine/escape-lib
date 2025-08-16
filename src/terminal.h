#pragma once

#if _WIN32
#include <windows.h>
#endif

// Avoids replacing the type everywhere when required
#define FLAG_T unsigned int


enum termflags : FLAG_T {
	ALTBUF      = 0x1,
	HIDE_CURSOR = 0x2,
	NO_ECHO     = 0x4,
};

/* Initialize some states variables and gathers information about the terminal the program is running in.
 * Required for escape sequences to work properly. */
void init_term();

/* Sets the given terminal flags via bitmask.
 * Available flags are available int `enum termflags`.
 * Returns 0 if flags were applied, -1 if it was the same as before. */
int set_termflags(const FLAG_T flags);

/* Returns a pointer to the program's static terminal flags. */
const FLAG_T* get_termflags();

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

