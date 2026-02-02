#include <stdbit.h>
#include <stdio.h>
#include <stdlib.h>
#include <uchar.h>
#include <unistd.h>

#include "esqsec.h"
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
static size_t s_strbuf_init_size  = 131072; // 2^17
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
static inline void initscrbuf(struct scrbuf* scrbuf, struct termclr bg_clr, struct termclr fg_clr, size_t charssize,
                              size_t metassize, size_t clrssize)
{
	scrbuf->def_bg_clr = bg_clr;
	scrbuf->def_fg_clr = fg_clr;
	// We assign each buffer pointer to the right location, based on the given address and offset in the arena
	scrbuf->chars     = OFFSET_PTR_BY(scrbuf, sizeof(struct scrbuf), char32_t);
	scrbuf->cellmetas = OFFSET_PTR_BY(scrbuf->chars, charssize, struct cellmeta);
	scrbuf->bg_clrs   = OFFSET_PTR_BY(scrbuf->cellmetas, metassize, union termclrval);
	scrbuf->fg_clrs   = OFFSET_PTR_BY(scrbuf->bg_clrs, clrssize, union termclrval);
}

static inline void initstrbuf(struct _strbuf* strbuf, size_t pagesize)
{
	strbuf->pagesize = pagesize;
	strbuf->buf      = OFFSET_PTR_BY(strbuf, sizeof(struct _strbuf), char);
	strbuf->cursor   = 0;
	strbuf->bufsize  = s_strbuf_init_size;
}

screen* newscr(struct termclr bgclr, struct termclr fgclr, termstatefl scrflags)
{
	const struct termsize termsize = get_termsize();
	const size_t cell_cnt          = termsize.cols * termsize.rows;

	const size_t charssize = cell_cnt * sizeof(char32_t);
	const size_t clrssize  = cell_cnt * sizeof(union termclrval);
	const size_t metassize = cell_cnt * sizeof(struct cellmeta);
	// size of struct scrbuf + the size of its buffers including worst case padding
	const size_t worst_scrbuf_memsize
		= WORST_SIZEOF(struct scrbuf)
	      + (2 * (clrssize + alignof(union termclrval)) + (charssize + alignof(char32_t)) + (metassize + alignof(uchar)));

	const size_t arena_pagesize  = WORST_SIZEOF(struct _scr_arena) + (1 + (scrflags & SCREEN_USE_VIRTUAL)) * worst_scrbuf_memsize;
	const size_t strbuf_pagesize = WORST_SIZEOF(struct _strbuf) + s_strbuf_init_size;

	struct _scr_arena* arena = heapalloc(arena_pagesize);
	if (!arena) {
		return nullptr;
	}
	struct _strbuf* strbuf = heapalloc(strbuf_pagesize);
	if (!strbuf) {
		heapfree(arena, arena_pagesize);
		return nullptr;
	}

	arena->termflags = (scrflags & SCREEN_HOLD_TERMFLAGS) ? (scrflags & ~(SCREEN_HOLD_TERMFLAGS | SCREEN_USE_VIRTUAL)) : -1;
	arena->pagesize  = arena_pagesize;
	arena->termsize  = termsize;

	arena->strbuf = strbuf;
	initstrbuf(arena->strbuf, strbuf_pagesize);

	arena->pbuf = OFFSET_PTR_BY(arena, sizeof(struct _scr_arena), struct scrbuf);
	initscrbuf(arena->pbuf, bgclr, fgclr, charssize, metassize, clrssize);
	if (scrflags & SCREEN_USE_VIRTUAL) {
		// arena is 0 initialized, so arena->_vbuf is nullptr by default
		arena->vbuf = OFFSET_PTR_BY(arena->pbuf, worst_scrbuf_memsize, struct scrbuf);
		initscrbuf(arena->vbuf, bgclr, fgclr, charssize, metassize, clrssize);
	}

	return arena;
}

bool freescr(screen* scr)
{
	// Don't mess up the order! strbuf THEN scr
	if (heapfree(scr->strbuf, scr->strbuf->pagesize) || heapfree(scr, scr->pagesize)) {
		return true;
	}
	scr = nullptr;
	return false;
}

/* ------------------------ *
 * --- LIBRARY HOT SPOT --- *
 * ------------------------ */
struct _strbuf* strbuf_grow(struct _strbuf* strbuf)
{
	const size_t newbufsize   = strbuf->bufsize * s_strbuf_growth_rate;
	const size_t new_pagesize = WORST_SIZEOF(struct _strbuf) + newbufsize;

	struct _strbuf* newstrbuf = heapalloc(new_pagesize);
	if (!newstrbuf) {
		return nullptr;
	}

	newstrbuf->pagesize = new_pagesize;
	newstrbuf->bufsize  = newbufsize;
	newstrbuf->cursor   = strbuf->cursor;
	newstrbuf->buf      = OFFSET_PTR_BY(newstrbuf, sizeof(struct _strbuf), char);
	// -- CRITICAL : move memory (MUST be faster than or equal to memmove) -- //
	for (size_t i = 0; i < strbuf->bufsize; ++i) {
		newstrbuf->buf[i] = strbuf->buf[i];
	}

	if (heapfree(strbuf, strbuf->bufsize)) {
		// TODO : Return an error via enum so we know exactly what went wrong instead of just "true"
		heapfree(newstrbuf, new_pagesize); // I fear there is nothing we can do...
		return nullptr;
	}

	strbuf = nullptr;
	return newstrbuf;
}

static inline void strbufadd(struct _strbuf** strbuf, const char* str, size_t strlen)
{
	if ((*strbuf)->cursor + strlen > (*strbuf)->bufsize) {
		*strbuf = strbuf_grow(*strbuf);
#if DEBUG
		if (!(*strbuf)) {
			fprintf(stderr, "Could not reallocate string buffer, exiting before segfault...\n");
			exit(1);
		}
#endif
	}
	for (size_t i = 0; i < strlen; ++i) {
		(*strbuf)->buf[(*strbuf)->cursor + i] = str[i];
	}
	(*strbuf)->cursor += strlen;
}

static inline void addclrtostrbuf(screen* scr, size_t cell_idx, bool isbg)
{
	const enum clrfmt fmt = isbg ? scr->pbuf->cellmetas[cell_idx].bg_clrfmt : scr->pbuf->cellmetas[cell_idx].fg_clrfmt;
	switch (fmt) {
	case CELL_CLRFMT_CODE:
		char clr_code_seq[U8_PARAM_SEQLEN(1)];
		const uchar clr_code         = isbg ? scr->pbuf->bg_clrs[cell_idx].code + 10 : scr->pbuf->fg_clrs[cell_idx].code;
		const size_t clr_code_seqlen = paramu8seq(clr_code_seq, (uint8_t[]){clr_code}, 1);
		strbufadd(&scr->strbuf, clr_code_seq, clr_code_seqlen);
		break;
	case CELL_CLRFMT_RGB:
		char clr_rgb_seq[5 + U8_PARAM_SEQLEN(3)];
		const struct rgb clr_rgb    = isbg ? scr->pbuf->bg_clrs[cell_idx].rgb : scr->pbuf->fg_clrs[cell_idx].rgb;
		const size_t clr_rgb_seqlen = paramu8seq(clr_rgb_seq, (uint8_t[]){isbg ? 48 : 38, 2, clr_rgb.r, clr_rgb.g, clr_rgb.b}, 5);
		strbufadd(&scr->strbuf, clr_rgb_seq, clr_rgb_seqlen);
		break;
	case CELL_CLRFMT_ID:
		char clr_id_seq[5 + U8_PARAM_SEQLEN(1)];
		const u8 clr_id            = isbg ? scr->pbuf->bg_clrs[cell_idx].id : scr->pbuf->fg_clrs[cell_idx].id;
		const size_t clr_id_seqlen = paramu8seq(clr_id_seq, (uint8_t[]){isbg ? 48 : 38, 5, clr_id}, 3);
		strbufadd(&scr->strbuf, clr_id_seq, clr_id_seqlen);
		break;
	}
}


// TODO : Allow for saving strbuf to any file
/* ------------------------ *
 * --- LIBRARY HOT SPOT --- *
 * --------------------- -- */
bool srefresh(screen* scr)
{
	// TODO : test if filling the strbuf directly (without any temporary buffers) is more efficient
	strbufadd(&scr->strbuf, CSI "H", 2);
	const size_t cell_cnt = scr->termsize.cols * scr->termsize.rows;
	uint16_t last_col = 0, last_row = 0;
	for (size_t i = 0; i < cell_cnt; ++i) {
		if (!scr->pbuf->cellmetas[i].is_visible) {
			continue;
		}

		addclrtostrbuf(scr, i, true);
		addclrtostrbuf(scr, i, false);

		const uint16_t row = i / scr->termsize.cols;
		const uint16_t col = i - row * scr->termsize.cols;
		if (last_col == scr->termsize.cols - 1 && col == 0) {
			strbufadd(&scr->strbuf, "\n", 1);
		} else if (col != last_col + 1 || row != last_row) {
			// TODO : remove calls to stdio
			char mvseq[2 + U16_PARAM_SEQLEN(2)];
			const size_t mvseq_len = seqcat(
				mvseq, (struct seqel[]){SEQ_STRL(CSI), SEQ_U16(row + 1), SEQ_CHR(';'), SEQ_U16(col + 1), SEQ_CHR('H')}, 5);
			strbufadd(&scr->strbuf, mvseq, mvseq_len);
		}

		const bool cell_has_clr = (scr->pbuf->cellmetas[i].bg_clrfmt || scr->pbuf->cellmetas[i].fg_clrfmt);
		if (!scr->pbuf->chars[i] && cell_has_clr) {
			strbufadd(&scr->strbuf, " ", 1);
		} else {
			char gphm[MAX_GPHM_CPTS];
			size_t cp_cnt = c32togphm(scr->pbuf->chars[i], gphm);
			strbufadd(&scr->strbuf, gphm, cp_cnt);
		}

		if (cell_has_clr) {
			strbufadd(&scr->strbuf, CSI "m", 3);
		}
		last_col = col;
		last_row = row;
	}

	long bytes_written;
	const ulong bytes_to_write = scr->strbuf->cursor;

#if __unix__
	bytes_written = write(STDOUT_FILENO, scr->strbuf->buf, bytes_to_write);
	if (bytes_written == -1 || bytes_written != (long)bytes_to_write) {
		return true;
	}
#elif _WIN32
	if (WriteConsole(*get_g_stdout_hndl(), scr._strbuf.buf, &bytes_written, nullptr) || bytes_written != (long)bytes_to_write) {
		return true;
	}
#endif
	if (scr->vbuf != nullptr) {
		scr->vbuf = scr->pbuf;
	}
	scr->strbuf->cursor = 0;

	return false;
}


inline enum escerr sgetcorderr(const screen* scr, u16 x, u16 y)
{
	if (x > scr->termsize.cols)
		return ESC_ERR_COORD_X;
	if (y > scr->termsize.rows)
		return ESC_ERR_COORD_Y;
	return ESC_OK;
}

inline long scordtoidx(const screen* scr, u16 x, u16 y)
{
	// We substract 1 to x and y since the first cell is (1, 1), not (0, 0)
	return (sgetcorderr(scr, x, y) == ESC_OK) ? (y - 1) * scr->termsize.cols + (x - 1) : -1;
}

enum escerr ssetgphm(screen* restrict scr, const char* gphm, u16 x, u16 y)
{
	const long idx = scordtoidx(scr, x, y);
	if (idx != -1) {
		const char32_t c32 = gphmtoc32((unsigned char*)gphm);
		if (!c32) {
			return ESC_ERR_GPHM;
		}
		scr->pbuf->cellmetas[idx].is_visible = true;

		scr->pbuf->chars[idx] = c32;
	}
	return sgetcorderr(scr, x, y);
}

enum escerr ssetbgclr(screen* restrict scr, struct termclr clr, uint16_t x, uint16_t y)
{
	const long idx = scordtoidx(scr, x, y);
	if (idx != -1) {
		scr->pbuf->cellmetas[idx].is_visible = true;
		scr->pbuf->cellmetas[idx].bg_clrfmt  = clr.fmt;

		scr->pbuf->bg_clrs[idx] = clr.value;
	}
	return sgetcorderr(scr, x, y);
}
enum escerr ssetfgclr(screen* restrict scr, struct termclr clr, uint16_t x, uint16_t y)
{
	const long idx = scordtoidx(scr, x, y);
	if (idx != -1) {
		scr->pbuf->cellmetas[idx].is_visible = true;
		scr->pbuf->cellmetas[idx].fg_clrfmt  = clr.fmt;

		scr->pbuf->fg_clrs[idx] = clr.value;
	}
	return sgetcorderr(scr, x, y);
}
enum escerr ssetclrpair(screen* restrict scr, struct termclr bgclr, struct termclr fgclr, uint16_t x, uint16_t y)
{
	const long idx = scordtoidx(scr, x, y);
	if (idx != -1) {
		scr->pbuf->cellmetas[idx].is_visible = true;
		scr->pbuf->cellmetas[idx].bg_clrfmt  = bgclr.fmt;
		scr->pbuf->cellmetas[idx].fg_clrfmt  = fgclr.fmt;

		scr->pbuf->bg_clrs[idx] = bgclr.value;
		scr->pbuf->fg_clrs[idx] = fgclr.value;
	}
	return sgetcorderr(scr, x, y);
}

enum escerr saddstr(screen* restrict scr, const char* str, size_t strlen, u16 x, u16 y)
{
	enum cptype type = CP_INVALID;
	for (size_t i = 0; i < strlen; i += type) {
		if ((type = get_cptype(str[i])) == CP_INVALID) {
			return ESC_ERR_UTF8;
		}
		ssetgphm(scr, str + i, x, y);
	}
	return ESC_OK;
}

