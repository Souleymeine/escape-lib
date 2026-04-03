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

RESULT(struct esc_termsize) esc_gettermsize()
{
#if __unix__

	struct winsize ws;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
	struct esc_termsize ret = (struct esc_termsize) {
		.rows = ws.ws_row,
		.cols = ws.ws_col,
		.xpix = ws.ws_xpixel,
		.ypix = ws.ws_ypixel,
	};
#  if __linux__ // The Linux kernel doesn't provide ws_xpixel and ws_ypixel in its terminal, we work around that :
	/* Terminal devices are exposed through /dev/tty[0-9][0-9] while terminal emulators use slave (?) devices called /dev/pts/[0-9][0-9].
	 * This is how we differentiate kernel-level terminals and terminal emulators.
	 * See https://www.kernel.org/doc/html/latest/fb/framebuffer.html for more details. */
	const char* tty_abs_path = ttyname(STDOUT_FILENO);
	const bool is_kdev = tty_abs_path ? (strncmp(tty_abs_path + STRLLEN("/dev/"), "tty", 3) == 0) : false;
	if (is_kdev) {
		// Virtually all distros enable FB_DEVICE (which provides /dev/fbX), except some custom kernels (like mine)
		int fb = open("/dev/fb0", O_RDONLY); // TODO : Does the kernel provide fbX for terminals other screens?
		if (fb == -1) {
			return RESERR(struct esc_termsize, ESC_ERR_FB_DEVICE_NOT_SET);
		}

		struct fb_var_screeninfo fbinfo;
		ioctl(fb, FBIOGET_VSCREENINFO, &fbinfo);
		ret.xpix = fbinfo.xres_virtual;
		ret.ypix = fbinfo.yres_virtual;

		close(fb);
	}
#  endif

	return RESOK(struct esc_termsize, ret);

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
