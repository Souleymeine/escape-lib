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

RESULT(struct esc_cellsize) esc_getcellsize()
{
	const RESULT(struct esc_termsize) size = esc_gettermsize();
	TRY(struct esc_cellsize, size);

	// cell size is always accurate in the kernel as there aren't really "margins" and integer division exludes them
	// TODO : take margins into account for terminal emulators (unstable when zooming)
	return RESOK(struct esc_cellsize, {
		.xpix  = size.val.xpix / size.val.cols,
		.ypix  = size.val.ypix / size.val.rows,
	});
}

#if __unix__
static bool is_dev_tty()
{
	const char* tty_abs_path = ttyname(STDOUT_FILENO);
	return tty_abs_path ? (strncmp(tty_abs_path + STRLLEN("/dev/"), "tty", 3) == 0) : false;
}
#endif

RESULT(struct esc_termsize) esc_gettermsize()
{
#if __unix__
	// Virtually all modern distros enable FB_DEV (which provides /dev/fb0), except custom kernels maybe (like mine),
	// which isn't big of a deal when you use package managers like Portage which tell you what kernel param you need
	// for what package anyway, no worries when it comes to compat.
	struct fb_var_screeninfo fb_scrinfo;
	// Actual terminals (thus managed by the kernel itself) are directly attached to a /dev/tty[0-9][0-9] device
	// while terminal emulators use a slave (???) device called /dev/pts/[0-9][0-9]
	const bool is_dev = is_dev_tty();
	if (is_dev) {
		int fb = open("/dev/fb0", O_RDONLY); // Only support 1 fb at the moment
		if (fb == -1) {
			return RESERR(struct esc_termsize, ESC_ERR_FB_DEV_NOT_SET);
		}
		ioctl(fb, FBIOGET_VSCREENINFO, &fb_scrinfo);
		close(fb);
	}

	struct winsize size;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
	return RESOK(struct esc_termsize, {
		.rows = size.ws_row,
		.cols = size.ws_col,
		.xpix = is_dev ? fb_scrinfo.xres_virtual : size.ws_xpixel,
		.ypix = is_dev ? fb_scrinfo.yres_virtual : size.ws_ypixel,
	});

#elif _WIN32

	// True if the program is running inside Microsoft's newer "Terminal" terminal emulator.
	const bool is_ms_terminal = (getenv("WT_SESSION") || getenv("WT_PROFILE_ID")); // Undefined envvars if false
	const HWND conwin_hndl = is_ms_terminal ? GetWindow(conwin_hndl, GW_OWNER) : GetConsoleWindow();

	RECT conwin_rect;
	GetWindowRect(conwin_hndl, &conwin_rect);

	CONSOLE_SCREEN_BUFFER_INFO scrbuf_info;
	GetConsoleScreenBufferInfo(esc_getstdout_h(), &scrbuf_info);

	return RESOK(struct esc_termsize, {
		.rows = scrbuf_info.dwSize.Y,
		.cols = scrbuf_info.dwSize.X,
		.xpix = conwin_rect.right - conwin_rect.left,
		.ypix = conwin_rect.bottom - conwin_rect.top,
	});

#endif
}
