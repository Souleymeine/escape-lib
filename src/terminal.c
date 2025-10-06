#if _WIN32
#include <windows.h>
#elif __unix__
#include <termios.h>
#include <unistd.h>
#endif
#include <stdio.h>

#include "terminal.h"

// Control Sequence Introducer
#define CSI "\x1b["

#if __unix__
static struct termios current_term_attr;
#elif _WIN32
static HANDLE h_stdin;
static HANDLE h_stdout;
static DWORD current_stdin_mode;
static DWORD og_stdin_mode;
static DWORD og_stdout_mode;
static UINT og_output_cp;
#endif

static FLAG_T current_flags = 0;

void init_term()
{
#if __unix__
	tcgetattr(STDIN_FILENO, &current_term_attr);
#elif _WIN32
	h_stdin  = GetStdHandle(STD_INPUT_HANDLE);
	h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);

	GetConsoleMode(h_stdout, &og_stdout_mode);
	GetConsoleMode(h_stdin, &og_stdin_mode);

	SetConsoleMode(h_stdout, og_stdout_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

	const DWORD new_in_mode = og_stdin_mode | ENABLE_VIRTUAL_TERMINAL_INPUT;
	current_stdin_mode      = new_in_mode;
	SetConsoleMode(h_stdin, new_in_mode);

	og_output_cp = GetConsoleOutputCP();
#endif

	// TODO : gather termcaps but without termcap/terminfo
	// Read this : https://lobste.rs/s/m1j4b4/terminfo_at_this_point_time_is_net
}

int set_termflags(const FLAG_T flags)
{
	if (flags == current_flags) {
		return -1;
	}

#if __unix__
	current_term_attr.c_lflag = (flags & NO_ECHO) ? current_term_attr.c_lflag & (~ECHO) : current_term_attr.c_lflag | ECHO;
	tcsetattr(STDIN_FILENO, 0, &current_term_attr);
#elif _WIN32
	/* "This mode [`ENABLE_ECHO_INPUT`] can be used only if the ENABLE_LINE_INPUT mode is also
	 * enabled."
	 * - https://learn.microsoft.com/en-us/windows/console/setconsolemode
	 * TODO : some "side effects" are caused by the combination of those two flags, (presumably
	 * something that has to do with line buffering) and should be studied with more care. */
	SetConsoleMode(h_stdin,
	               current_stdin_mode & (flags & NO_ECHO) ? (~ENABLE_ECHO_INPUT) : (ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT));
#endif

	printf(CSI "?1049%c", flags & ALTBUF ? 'h' : 'l');
	printf(CSI "?25%c", flags & HIDE_CURSOR ? 'l' : 'h');

	current_flags = flags;
	return 0;
}


const FLAG_T* get_termflags()
{
	return &current_flags;
}

#if _WIN32
const HANDLE* get_g_stdin_hndl()
{
	return &h_stdin;
}

const HANDLE* get_g_stdout_hndl()
{
	return &h_stdout;
}
#endif


void cleanup_term()
{
	set_termflags(0);

#if _WIN32
	SetConsoleOutputCP(og_output_cp);
	SetConsoleMode(h_stdout, og_stdout_mode);
	SetConsoleMode(h_stdin, og_stdin_mode);
#endif
}

