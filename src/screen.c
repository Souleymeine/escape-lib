#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <uchar.h>
#include <unistd.h>

#include "grapheme.h"
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
static size_t s_strbuf_init_size  = 262144; // 2^18
static float s_strbuf_growth_rate = 1.5f;

void scrmemargs(size_t new_init_strbuf_size, float new_strbuf_growth_rate)
{
	s_strbuf_init_size   = new_init_strbuf_size;
	s_strbuf_growth_rate = new_strbuf_growth_rate;
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
#define OFFSET_PTR_BY(baseaddr, offset, ptrbasetype)                               \
	((ptrbasetype*)alignptr((uintptr_t)(baseaddr) + offset, alignof(ptrbasetype)))

#define WORST_SIZEOF(type) (sizeof(type) + alignof(type))


/** Initializes the fields of the given scrbuf struct and the page segment it represents */
static inline void initscrbuf(struct scrbuf* scrbuf, union termclr bg_clr, union termclr fg_clr, size_t charssize,
                              size_t flagssize, size_t clrssize)
{
	scrbuf->bg_clr = bg_clr;
	scrbuf->fg_clr = fg_clr;
	// We assign each buffer pointer to the right location, based on the given address and offset in the arena
	scrbuf->chars     = OFFSET_PTR_BY(scrbuf, sizeof(struct scrbuf), char32_t);
	scrbuf->cellflags = OFFSET_PTR_BY(scrbuf->chars, charssize, uchar);
	scrbuf->bg_clrs   = OFFSET_PTR_BY(scrbuf->cellflags, flagssize, union termclr);
	scrbuf->fg_clrs   = OFFSET_PTR_BY(scrbuf->bg_clrs, clrssize, union termclr);
}

static inline void initstrbuf(struct _scrstrbuf* strbuf, size_t pagesize)
{
	strbuf->pagesize = pagesize;
	strbuf->buf      = OFFSET_PTR_BY(strbuf, sizeof(struct _scrstrbuf), char);
	strbuf->cursor   = 0;
	strbuf->bufsize  = s_strbuf_init_size;
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

	const size_t arena_pagesize  = WORST_SIZEOF(struct _scr_arena) + (1 + (scrflags & SCREEN_USE_VIRTUAL)) * worst_scrbuf_memsize;
	const size_t strbuf_pagesize = WORST_SIZEOF(struct _scrstrbuf) + s_strbuf_init_size;

	struct _scr_arena* arena = heapalloc(arena_pagesize);
	if (!arena) {
		return nullptr;
	}
	struct _scrstrbuf* strbuf = heapalloc(strbuf_pagesize);
	if (!strbuf) {
		heapfree(arena, arena_pagesize);
		return nullptr;
	}

	arena->_termflags = (scrflags & SCREEN_HOLD_TERMFLAGS) ? (scrflags & ~(SCREEN_HOLD_TERMFLAGS | SCREEN_USE_VIRTUAL)) : -1;
	arena->_pagesize  = arena_pagesize;
	arena->_termsize  = termsize;

	arena->_strbuf = strbuf;
	initstrbuf(arena->_strbuf, strbuf_pagesize);

	arena->_pbuf = OFFSET_PTR_BY(arena, sizeof(struct _scr_arena), struct scrbuf);
	initscrbuf(arena->_pbuf, bg_clr, fg_clr, charssize, flagssize, clrssize);
	if (scrflags & SCREEN_USE_VIRTUAL) {
		// arena is 0 initialized, so arena->_vbuf is nullptr by default
		arena->_vbuf = OFFSET_PTR_BY(arena->_pbuf, worst_scrbuf_memsize, struct scrbuf);
		initscrbuf(arena->_vbuf, bg_clr, fg_clr, charssize, flagssize, clrssize);
	}

	return arena;
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
	const size_t newbufsize   = strbuf->bufsize * s_strbuf_growth_rate;
	const size_t new_pagesize = WORST_SIZEOF(struct _scrstrbuf) + newbufsize;

	struct _scrstrbuf* newstrbuf = heapalloc(new_pagesize);
	if (newstrbuf == nullptr) {
		return true;
	}

	newstrbuf->pagesize = new_pagesize;
	newstrbuf->bufsize  = newbufsize;
	newstrbuf->cursor   = strbuf->cursor;
	newstrbuf->buf      = OFFSET_PTR_BY(newstrbuf, sizeof(struct _scrstrbuf), char);
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


inline enum escerr sgetcorderr(const screen* scr, u16 x, u16 y)
{
	if (x > scr->_termsize.cols) return ESC_COORD_X_OUT;
	if (y > scr->_termsize.rows) return ESC_COORD_Y_OUT;
	return ESC_OK;
}

inline long scordtoidx(const screen* scr, u16 x, u16 y)
{
	// We substract 1 to x and y since the first cell is (1, 1), not (0, 0)
	return (sgetcorderr(scr, x, y) == ESC_OK) ? (y - 1) * scr->_termsize.cols + (x - 1) : -1;
}

enum escerr ssetgphm(screen* restrict scr, const char* gphm, u16 x, u16 y)
{
	const long idx = scordtoidx(scr, x, y);
	if (idx != -1) {
		scr->_pbuf->cellflags[idx] |= CELL_VISIBLE;
		scr->_pbuf->chars[idx] = gphmtoc32((unsigned char*)gphm);
	}
	return sgetcorderr(scr, x, y);
}

enum escerr ssetclr(screen* restrict scr, union termclr clr, uchar clrflags, u16 x, u16 y)
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

