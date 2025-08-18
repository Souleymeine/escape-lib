#if _WIN32
#include <windows.h>

#include "terminal.h"
#elif __unix__
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif
#include "termsize.h"

#if _WIN32
static inline RECT get_conwin_rect()
{
	HWND conwin_hndl = GetConsoleWindow();
	/* Newest Terminal only envvars
	 * true if the program is running inside Microsoft's newer "Terminal" terminal emulator */
	if (getenv("WT_SESSION") != nullptr || getenv("WT_PROFILE_ID") != nullptr) {
		conwin_hndl = GetWindow(conwin_hndl, GW_OWNER);
	}

	RECT conwin_rect;
	GetWindowRect(conwin_hndl, &conwin_rect);

	return conwin_rect;
}
#endif


void ref_termsize(struct termsize* ref)
{
#if __unix__

	struct winsize termsize;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &termsize);

	ref->rows = termsize.ws_row;
	ref->cols = termsize.ws_col;
	ref->xpix = termsize.ws_xpixel;
	ref->ypix = termsize.ws_ypixel;

#elif _WIN32

	CONSOLE_SCREEN_BUFFER_INFO scrbuf_info;
	GetConsoleScreenBufferInfo(*get_g_stdout_hndl(), &scrbuf_info);
	RECT conwin_rect = get_conwin_rect();

	ref->rows = scrbuf_info.dwSize.Y;
	ref->cols = scrbuf_info.dwSize.X;
	ref->xpix = conwin_rect.right - conwin_rect.left;
	ref->ypix = conwin_rect.bottom - conwin_rect.top;

#endif
}

struct ro_termsize get_termsize()
{
#if __unix__

	struct winsize termsize;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &termsize);

	return (struct ro_termsize) {
		.rows = termsize.ws_row,
		.cols = termsize.ws_col,
		.xpix = termsize.ws_xpixel,
		.ypix = termsize.ws_ypixel,
	};

#elif _WIN32

	CONSOLE_SCREEN_BUFFER_INFO scrbuf_info;
	GetConsoleScreenBufferInfo(*get_g_stdout_hndl(), &scrbuf_info);
	RECT conwin_rect = get_conwin_rect();

	return (struct ro_termsize) {
		.rows = scrbuf_info.dwSize.Y,
		.cols = scrbuf_info.dwSize.X,
		.xpix = conwin_rect.right - conwin_rect.left,
		.ypix = conwin_rect.bottom - conwin_rect.top
	};

#endif
}

