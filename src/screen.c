#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <uchar.h>
#include <unistd.h>
#if __unix__
#include <sys/mman.h>
#elif _WIN32
#include <memoryapi.h>

#include "terminal.h"
#endif

#include "escdef.h"
#include "screen.h"
#include "termsize.h"

#include "_escdef.h"

// Default values
static size_t S_STRBUF_INIT_BUFSIZE = 262144; // 2^18
static float S_STRBUF_GROWTH_RATE   = 1.5f;

void scrmemargs(size_t new_init_strbuf_size, float new_strbuf_growth_rate)
{
	S_STRBUF_INIT_BUFSIZE = new_init_strbuf_size;
	S_STRBUF_GROWTH_RATE  = new_strbuf_growth_rate;
}


/** --- Cross plateform heap de/allocator --- */

static inline void* heapalloc(size_t pagesize)
{
#if __unix__
	// MAP_ANONYMOUS, when used with -1 as fd, has the side effect of initializing the page with 0s, which is desired
	void* page = mmap(nullptr, pagesize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (page == MAP_FAILED) {
		return nullptr;
	}
#elif _WIN32
	LPVOID page = VirtualAlloc(nullptr, pagesize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!page) {
		return nullptr;
	}
#endif

	return page;
}

static inline bool heapfree(void* ptr, size_t pagesize)
{
#if __unix__
	if (munmap(ptr, pagesize) == -1) {
		return true;
	}
#elif _WIN32
	if (VirtualFree(ptr, pagesize, MEM_RELEASE) == 0) {
		return true;
	}
#endif

	return false;
}

/** Return the adress offset to the right to respect the given alignment */
static inline uintptr_t alignptr(uintptr_t addr, size_t align)
{
	// https://en.wikipedia.org/wiki/Data_structure_alignment
	return (addr + (align - 1)) & -align;
}
#define ALIGNED_PTR(type, addr) ((type*)alignptr((uintptr_t)(addr), alignof(type)))

#define WORST_SIZEOF(type) (sizeof(type) + alignof(type))


/** Initializes the fields of the given scrbuf struct and the page segment it represents */
static inline void initscrbuf(struct scrbuf* scrbuf, union termclr bg_clr, union termclr fg_clr, size_t charssize,
                              size_t flagssize, size_t clrssize)
{
	scrbuf->bg_clr = bg_clr;
	scrbuf->fg_clr = fg_clr;
	// We assign each buffer pointer to the right location, based on the given address and offset in the arena
	scrbuf->chars     = ALIGNED_PTR(char32_t, scrbuf + sizeof(struct scrbuf));
	scrbuf->cellflags = ALIGNED_PTR(uchar, scrbuf->chars + charssize);
	scrbuf->bg_clrs   = ALIGNED_PTR(union termclr, scrbuf->cellflags + flagssize);
	scrbuf->fg_clrs   = ALIGNED_PTR(union termclr, scrbuf->bg_clrs + clrssize);
}

static inline void initstrbuf(struct _scrstrbuf* strbuf, size_t pagesize)
{
	strbuf->pagesize = pagesize;
	strbuf->buf      = ALIGNED_PTR(char, strbuf + sizeof(struct _scrstrbuf));
	strbuf->cursor   = 0;
	strbuf->bufsize  = S_STRBUF_INIT_BUFSIZE;
}

screen* newscr(union termclr bg_clr, union termclr fg_clr, termstatefl scrflags)
{
	const struct termsize termsize = get_termsize();
	const size_t cell_cnt          = termsize.cols * termsize.rows;

	const size_t charssize = cell_cnt * sizeof(char32_t);
	const size_t clrssize  = cell_cnt * sizeof(union termclr);
	const size_t flagssize = cell_cnt * sizeof(uchar);
	// size of struct scrbuf + the size of its buffers including worst case padding
	const size_t worst_scrbuf_memsize
		= WORST_SIZEOF(struct scrbuf)
	      + (2 * (clrssize + alignof(union termclr)) + (charssize + alignof(char32_t)) + (flagssize + alignof(uchar)));

	const bool use_vscr = scrflags & USE_VSCR;

	const size_t arena_pagesize  = WORST_SIZEOF(struct _scr_arena) + (1 + use_vscr) * worst_scrbuf_memsize;
	const size_t strbuf_pagesize = WORST_SIZEOF(struct _scrstrbuf) + S_STRBUF_INIT_BUFSIZE;

	struct _scr_arena* scr_arena = heapalloc(arena_pagesize);
	if (!scr_arena) {
		return nullptr;
	}
	struct _scrstrbuf* strbuf = heapalloc(strbuf_pagesize);
	if (!strbuf) {
		heapfree(scr_arena, arena_pagesize);
		return nullptr;
	}

	scr_arena->_termflags = (scrflags & HOLD_TERMFLAGS) ? (scrflags & ~(HOLD_TERMFLAGS | USE_VSCR)) : -1;
	scr_arena->_pagesize  = arena_pagesize;
	scr_arena->_termsize  = termsize;

	scr_arena->_strbuf = strbuf;
	initstrbuf(scr_arena->_strbuf, strbuf_pagesize);

	scr_arena->_pbuf = ALIGNED_PTR(struct scrbuf, scr_arena + sizeof(struct _scr_arena));
	initscrbuf(scr_arena->_pbuf, bg_clr, fg_clr, charssize, flagssize, clrssize);

	scr_arena->_vbuf = nullptr;
	if (use_vscr) {
		scr_arena->_vbuf = ALIGNED_PTR(struct scrbuf, scr_arena->_pbuf + worst_scrbuf_memsize);
		initscrbuf(scr_arena->_vbuf, bg_clr, fg_clr, charssize, flagssize, clrssize);
	}

	return scr_arena;
}

bool freescr(screen* scr)
{
	// Don't mess up the order! strbuf THEN scr
	if (heapfree(scr->_strbuf, scr->_strbuf->pagesize) || heapfree(scr, scr->_pagesize)) {
		return true;
	}
	scr = nullptr;
	return false;
}

/* ------------------------ *
 * --- LIBRARY HOT SPOT --- *
 * ------------------------ */
bool strbuf_realloc(struct _scrstrbuf* restrict strbuf)
{
	const size_t newbufsize   = strbuf->bufsize * S_STRBUF_GROWTH_RATE;
	const size_t new_pagesize = WORST_SIZEOF(struct _scrstrbuf) + newbufsize;

	struct _scrstrbuf* newstrbuf = heapalloc(new_pagesize);
	if (newstrbuf == nullptr) {
		return true;
	}

	newstrbuf->pagesize = new_pagesize;
	newstrbuf->bufsize  = newbufsize;
	newstrbuf->cursor   = strbuf->cursor;
	newstrbuf->buf      = ALIGNED_PTR(char, newstrbuf + sizeof(struct _scrstrbuf));
	// -- CRITICAL : move memory (MUST be faster than or equal to memcpy) -- //
	for (size_t i = 0; i < strbuf->bufsize; ++i) {
		newstrbuf->buf[i] = strbuf->buf[i];
	}

	if (heapfree(strbuf, strbuf->bufsize)) {
		// TODO : Return an error via enum so we know exactly what went wrong instead of just "true"
		heapfree(newstrbuf, new_pagesize); // I fear there is nothing we can do...
		return true;
	}

	strbuf = newstrbuf;
	return false;
}

// static inline void strbufadd(struct _scrstrbuf* restrict strbuf, const char* str, size_t strlen)
// {
// 	if (strbuf->cursor + strlen > strbuf->bufsize) {
// 		strbuf_realloc(strbuf);
// 	}
// 	for (size_t i = 0; i < strlen; ++i) {
// 		strbuf->buf[strbuf->cursor + i] = str[i];
// 	}
// 	strbuf->cursor += strlen;
// }

// TODO : Allow for saving strbuf to any file
/* ------------------------ *
 * --- LIBRARY HOT SPOT --- *
 * --------------------- -- */
bool srefresh(screen* scr)
{
	const size_t cell_cnt = scr->_termsize.cols * scr->_termsize.rows;
	for (size_t i = 0; i < cell_cnt; ++i) {
		// const char* c    = (i & 1) ? "é" : " ";
		// const size_t len = (i & 1) ? 2 : 1;
		// strbufadd(scr->_strbuf, c, len);
	}

	long bytes_written;
	const ulong bytes_to_write = scr->_strbuf->cursor + 1;

#if __unix__
	bytes_written = write(STDOUT_FILENO, scr->_strbuf->buf, bytes_to_write);
	if (bytes_written == -1 || bytes_written != (long)bytes_to_write) {
		return true;
	}
#elif _WIN32
	if (WriteConsole(*get_g_stdout_hndl(), scr._strbuf.buf, &bytes_written, nullptr) || bytes_written != (long)bytes_to_write) {
		return true;
	}
#endif
	if (scr->_vbuf != nullptr) {
		scr->_vbuf = scr->_pbuf;
	}
	scr->_strbuf->cursor = 0;

	return false;
}


inline enum corderr sgetcorderr(const screen* scr, u16 x, u16 y)
{
	if (x > scr->_termsize.cols) return COORD_X_OUT;
	if (y > scr->_termsize.rows) return COORD_Y_OUT;
	return COORDS_OK;
}

inline long scordtoidx(const screen* scr, u16 x, u16 y)
{
	// We substract 1 to x and y since the first cell is (1, 1), not (0, 0)
	return (sgetcorderr(scr, x, y) == COORDS_OK) ? (y - 1) * scr->_termsize.cols + (x - 1) : -1;
}

enum corderr ssetc32(screen* restrict scr, char32_t c32, u16 x, u16 y)
{
	const long idx = scordtoidx(scr, x, y);
	if (idx != -1) {
		scr->_pbuf->cellflags[idx] |= CELL_VISIBLE;
		scr->_pbuf->chars[idx] = c32;
	}
	return sgetcorderr(scr, x, y);
}

enum corderr ssetclr(screen* restrict scr, union termclr clr, uchar clrflags, u16 x, u16 y)
{
	const long idx = scordtoidx(scr, x, y);
	if (idx != -1) {
		scr->_pbuf->cellflags[idx] |= clrflags | CELL_VISIBLE;
		const bool is_bg    = clrflags & (CELL_BG_CLRCODE | CELL_BG_CLRID | CELL_BG_CLRRGB);
		union termclr* cell = is_bg ? &scr->_pbuf->bg_clrs[idx] : &scr->_pbuf->fg_clrs[idx];

		*cell = clr;
	}
	return sgetcorderr(scr, x, y);
}

