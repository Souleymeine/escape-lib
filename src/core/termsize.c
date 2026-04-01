#if _WIN32
#include <windows.h>
#elif __unix__
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

#include "../../include/core.h"

struct esc_termsize esc_gettermsize()
{
	// TODO : get pixel size on linux/BSD tty
#if __unix__

	struct winsize size;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
	return (struct esc_termsize) {
		.rows = size.ws_row,
		.cols = size.ws_col,
		.xpix = size.ws_xpixel,
		.ypix = size.ws_ypixel,
	};

#elif _WIN32

	// True if the program is running inside Microsoft's newer "Terminal" terminal emulator.
	const bool is_ms_terminal = (getenv("WT_SESSION") || getenv("WT_PROFILE_ID")); // Undefined envvars if false
	const HWND conwin_hndl = is_ms_terminal ? GetWindow(conwin_hndl, GW_OWNER) : GetConsoleWindow();

	RECT conwin_rect;
	GetWindowRect(conwin_hndl, &conwin_rect);

	CONSOLE_SCREEN_BUFFER_INFO scrbuf_info;
	GetConsoleScreenBufferInfo(esc_getstdout_h(), &scrbuf_info);

	return (struct esc_termsize) {
		.rows = scrbuf_info.dwSize.Y,
		.cols = scrbuf_info.dwSize.X,
		.xpix = conwin_rect.right - conwin_rect.left,
		.ypix = conwin_rect.bottom - conwin_rect.top,
	};

#endif
}
