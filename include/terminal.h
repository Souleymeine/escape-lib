#pragma once

#if _WIN32
#include <windows.h>
#endif

#include "escdef.h"

/* Initialize some states variables and gathers information about the terminal the program is
 * running in. Required for escape sequences to work properly. */
void init_term();

/* Sets the given terminal flags via bitmask.
 * Available flags are available int `enum termflags`.
 * Returns 0 if flags were applied, -1 if it was the same as before. */
int set_termflags(flags flags);

/* Available for convinience reasons :
 * Calls `init_term` then set_termflags.
 * You almost always want to do both during the initializtion of your program. */
void init_flags(flags flags);

/* Returns a pointer to the program's static terminal flags. */
const flags* get_termflags();

/* If called bedore init_term, stdscr will use a virtual screen by default. */
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

