#if _WIN32
#include <windows.h>
#elif __unix__
#include <termios.h>
#include <unistd.h>
#endif

#define ESC_SHORTHAND
#include "../../include/core.h"


#if __unix__
static struct termios g_termattr;
#elif _WIN32
static HANDLE g_stdin_h;
static HANDLE g_stdout_h;
static HANDLE g_stderr_h;
static DWORD  g_stdin_mode;
static DWORD  g_og_stdin_mode;
static DWORD  g_og_stdout_mode;
static UINT   g_og_output_cp;
#endif

static uint16_t g_flags = 0;
static uint16_t g_og_flags = 0;

RESULT(void) esc_init(OPT(uint16_t) flags)
{
#if __unix__
	tcgetattr(STDIN_FILENO, &g_termattr);
#elif _WIN32
	g_stdin_h  = GetStdHandle(STD_INPUT_HANDLE);
	g_stdout_h = GetStdHandle(STD_OUTPUT_HANDLE);
	g_stderr_h = GetStdHandle(STD_ERROR_HANDLE);
	GetConsoleMode(g_stdin_h,  &g_og_stdin_mode);
	GetConsoleMode(g_stdout_h, &g_og_stdout_mode);

	SetConsoleMode(g_stdout_h, g_og_stdout_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	g_stdin_mode = g_og_stdin_mode | ENABLE_VIRTUAL_TERMINAL_INPUT;
	SetConsoleMode(g_stdin_h, g_stdin_mode);

	g_og_output_cp = GetConsoleOutputCP();
#endif

	// TODO : gather termcaps but without termcap/terminfo
	// Read this : https://lobste.rs/s/m1j4b4/terminfo_at_this_point_time_is_net

	if (flags.exists) {
		TRY(void, esc_settermflags(flags.val));
	}
	return RESNOERR(void);
}

// TODO : set only flags that differ if flags and g_flags are not equal
RESULT(void) esc_settermflags(uint16_t flags)
{
#if __unix__
	g_termattr.c_lflag = (flags & ESC_TERM_NOECHO)
		? g_termattr.c_lflag & (~ECHO)
		: g_termattr.c_lflag | ECHO;
	tcsetattr(STDIN_FILENO, 0, &g_termattr);
#elif _WIN32
	/* "This mode [`ENABLE_ECHO_INPUT`] can be used only if the ENABLE_LINE_INPUT mode is also enabled."
	 * - https://learn.microsoft.com/en-us/windows/console/setconsolemode
	 * TODO : some "side effects" are caused by the combination of those two flags, (presumably
	 * something that has to do with line buffering) and should be studied with more care. */
	SetConsoleMode(g_stdin_h, (g_stdin_mode & (flags & NO_ECHO))
		? (~ENABLE_ECHO_INPUT)
		: (ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT));
#endif

	char8_t seq[16];
	const size_t len = esc_seqcat(seq, ARRARG(struct esc_seqel, {
		SEQSTRL(CSI), SEQSTRL("?1049"), SEQCHR(flags & ESC_TERM_ALTBUF ?   'h' : 'l'),
		SEQSTRL(CSI), SEQSTRL("?25"),   SEQCHR(flags & ESC_TERM_NOCURSOR ? 'l' : 'h'),
	}));

	TRY(void, esc_termwrite(ESC_STDOUT, seq, len));

	g_flags = flags;

	return RESNOERR(void);
}

uint16_t esc_gettermflags() { return g_flags; }

#if _WIN32
HANDLE esc_getstdout_h()  { return g_stdout_h; }
HANDLE esc_getstdin_h()   { return g_stdin_h; }
HANDLE esc_getstderr_h()  { return g_stderr_h; }
#endif

RESULT(void) esc_termwrite(enum esc_stdstream stream, const void* buf, size_t len)
{
#if __unix__
	const ssize_t bytes_written = write(stream, buf, len); // stream directly maps to POSIX fd
	if (bytes_written == -1) {
		return RESERR(void, ESC_ERR_TERMWRITE_FAILED);
	} else if (bytes_written != (ssize_t)len) {
		return RESERR(void, ESC_ERR_TERMWRITE_TRUNCATED);
	}
#elif _WIN32
	UINT h;
	switch(stream) {
	case STDOUT: h = g_stdout_h; break;
	case STDIN:  h = g_stdin_h;  break;
	case STDERR: h = g_stderr_h; break;
	}
	DWORD bytes_writtern;
	if (!WriteConsole(h, buf, &bytes_written, nullptr)) {
		return RESERR(void, ESC_ERR_TERMWRITE_FAILED);
	} else if (bytes_written != len) {
		return RESERR(void, ESC_ERR_TERMWRITE_TRUNCATED);
	}
#endif
	return RESNOERR(void);
}

void esc_cleanup()
{
	(void)esc_settermflags(g_og_flags);
#if _WIN32
	SetConsoleOutputCP(s_og_output_cp);
	SetConsoleMode(stdout_hndl, s_og_stdout_mode);
	SetConsoleMode(stdin_hndl, s_og_stdin_mode);
#endif
}

