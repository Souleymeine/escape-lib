#include <string.h>
#if _WIN32
#include <windows.h>
#elif __unix__
#if __linux__
#include <linux/fb.h>
#endif
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#define ESC_SHORTHAND
#include "../../include/core.h"

RESULT(struct esc_fontsize) esc_getfontsize()
{
	const RESULT(struct esc_termsize) size = esc_gettermsize();
	TRY(struct esc_fontsize, size);

	return RESVAL(struct esc_fontsize, {
		.xpix = size.val.xpix / size.val.cols,
		.ypix = size.val.ypix / size.val.rows,
	});
}

RESULT(struct esc_termsize) esc_gettermsize()
{
#if __unix__
	struct fb_var_screeninfo fb_scrinfo;
	const bool iskernel_term = (strncmp(ttyname(STDOUT_FILENO) + sizeof("/dev/") - 1, "tty", 3) == 0);
	if (iskernel_term) {
		// TODO : check if there is another way to get screen size without relying on kernel specific features
		// FB_DEV can be disabled!
		int fb = open("/dev/fb0", O_RDONLY);
		if (fb == -1) {
			return RESERR(struct esc_termsize, ESC_ERR_FBDEV_NOT_AVAILABLE);
		}
		ioctl(fb, FBIOGET_VSCREENINFO, &fb_scrinfo);
	}

	struct winsize size;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
	return RESVAL(struct esc_termsize, {
		.rows = size.ws_row,
		.cols = size.ws_col,
		.xpix = iskernel_term ? fb_scrinfo.xres : size.ws_xpixel,
		.ypix = iskernel_term ? fb_scrinfo.yres : size.ws_ypixel,
	});

#elif _WIN32

	// True if the program is running inside Microsoft's newer "Terminal" terminal emulator.
	const bool is_ms_terminal = (getenv("WT_SESSION") || getenv("WT_PROFILE_ID")); // Undefined envvars if false
	const HWND conwin_hndl = is_ms_terminal ? GetWindow(conwin_hndl, GW_OWNER) : GetConsoleWindow();

	RECT conwin_rect;
	GetWindowRect(conwin_hndl, &conwin_rect);

	CONSOLE_SCREEN_BUFFER_INFO scrbuf_info;
	GetConsoleScreenBufferInfo(esc_getstdout_h(), &scrbuf_info);

	return RESVAL(struct esc_termsize, {
		.rows = scrbuf_info.dwSize.Y,
		.cols = scrbuf_info.dwSize.X,
		.xpix = conwin_rect.right - conwin_rect.left,
		.ypix = conwin_rect.bottom - conwin_rect.top,
	});

#endif
}
