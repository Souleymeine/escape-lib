#if _WIN32
#include <windows.h>
#elif __unix__
#include <termios.h>
#include <unistd.h>
#endif

#include "../include/escseq.h"
#include "../include/screen.h"
#include "../include/terminal.h"


#if __unix__
static struct termios s_termattr;
#elif _WIN32
static HANDLE stdin_hndl;
static HANDLE stdout_hndl;
static DWORD s_stdin_mode;
static DWORD s_og_stdin_mode;
static DWORD s_og_stdout_mode;
static UINT s_og_output_cp;
#endif

static termstatefl s_flags    = 0;
static bool S_STDSCR_USE_VSCR = false;
static bool s_enabled_altbuf = false;

screen* stdscr;

void usevscr()
{
	S_STDSCR_USE_VSCR = true;
}

void init_term()
{
#if __unix__
	tcgetattr(STDIN_FILENO, &s_termattr);
#elif _WIN32
	stdin_hndl  = GetStdHandle(STD_INPUT_HANDLE);
	stdout_hndl = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleMode(stdin_hndl, &s_og_stdin_mode);
	GetConsoleMode(stdout_hndl, &s_og_stdout_mode);

	SetConsoleMode(stdout_hndl, s_og_stdout_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

	const DWORD new_in_mode = s_og_stdin_mode | ENABLE_VIRTUAL_TERMINAL_INPUT;
	current_stdin_mode      = new_in_mode;
	SetConsoleMode(stdin_hndl, new_in_mode);

	s_og_output_cp = GetConsoleOutputCP();
#endif

	// TODO : gather termcaps but without termcap/terminfo
	// Read this : https://lobste.rs/s/m1j4b4/terminfo_at_this_point_time_is_net
}

// TODO : set only flags that differ if flags and g_flags are not equal
int set_termflags(termstatefl flags)
{
	if (flags == s_flags) {
		return -1;
	}

#if __unix__
	s_termattr.c_lflag = (flags & TERM_NO_ECHO) ? s_termattr.c_lflag & (~ECHO) : s_termattr.c_lflag | ECHO;
	tcsetattr(STDIN_FILENO, 0, &s_termattr);
#elif _WIN32

	/* "This mode [`ENABLE_ECHO_INPUT`] can be used only if the ENABLE_LINE_INPUT mode is also enabled."
	 * - https://learn.microsoft.com/en-us/windows/console/setconsolemode
	 * TODO : some "side effects" are caused by the combination of those two flags, (presumably
	 * something that has to do with line buffering) and should be studied with more care. */
	SetConsoleMode(stdin_hndl,
	               current_stdin_mode & (flags & NO_ECHO) ? (~ENABLE_ECHO_INPUT) : (ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT));
#endif

	if (flags & TERM_ALTBUF) {
		if (!s_enabled_altbuf) {
			stdscr = newscr(DEF_SCR_BGCLR, DEF_SCR_FGCLR, S_STDSCR_USE_VSCR);
		}
		s_enabled_altbuf = true;
	}

	char seq[14];
	const size_t seqlen = seqcat(seq,
	                             (struct seqel[]){SEQ_STRL(CSI), SEQ_STRL("?1049"), SEQ_CHR(flags & TERM_ALTBUF ? 'h' : 'l'),
	                                              SEQ_STRL(CSI), SEQ_STRL("?25"), SEQ_CHR(flags & TERM_HIDE_CURSOR ? 'l' : 'h')},
	                             6);
	print(seq, seqlen);

	s_flags = flags;
	return 0;
}

inline void init_flags(termstatefl flags)
{
	init_term();
	set_termflags(flags);
}

const termstatefl* get_termflags()
{
	return &s_flags;
}

#if _WIN32
const HANDLE* get_g_stdin_hndl()
{
	return &stdin_hndl;
}

const HANDLE* get_g_stdout_hndl()
{
	return &stdout_hndl;
}
#endif

bool print(const char* restrict buf, size_t len)
{
	long bytes_written;
#if __unix__
	bytes_written = write(STDOUT_FILENO, buf, len);
	if (bytes_written == -1 || bytes_written != (long)len) {
		return true;
	}
#elif _WIN32
	if (WriteConsole(stdout_hndl, buf, &bytes_written, nullptr) || bytes_written != (long)len) {
		return true;
	}
#endif
	return false;
}

void cleanup_term()
{
	freescr(stdscr);
	set_termflags(0);

#if _WIN32
	SetConsoleOutputCP(s_og_output_cp);
	SetConsoleMode(stdout_hndl, s_og_stdout_mode);
	SetConsoleMode(stdin_hndl, s_og_stdin_mode);
#endif
}

