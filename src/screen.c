#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if __unix__
#include <sys/mman.h>
#elif _WIN32
#include <memoryapi.h>

#include "terminal.h"
#endif

#include "../include/_escdef.h"
#include "../include/escdef.h"
#include "../include/escseq.h"
#include "../include/screen.h"
#include "../include/terminal.h"
#include "../include/termsize.h"
#include "../include/utf.h"

// Default values
static usize s_strbuf_init_size   = 4096;
static float s_strbuf_growth_rate = 1.5f;

void scrmemargs(usize new_init_strbuf_size, float new_strbuf_growth_rate)
{
	s_strbuf_init_size   = new_init_strbuf_size;
	s_strbuf_growth_rate = new_strbuf_growth_rate;
}


/** --- Cross plateform heap de/allocator --- */

static inline void* heapalloc(usize pagesize)
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

static inline bool heapfree(void* ptr, usize pagesize)
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
static inline usize alignptr(usize addr, usize align)
{
	// https://en.wikipedia.org/wiki/Data_structure_alignment
	return (addr + (align - 1)) & -align;
}
#define OFFSET_PTR_BY(baseaddr, offset, ptrbasetype) ((ptrbasetype*)alignptr((usize)(baseaddr) + offset, alignof(ptrbasetype)))

#define WORST_SIZEOF(type) (sizeof(type) + alignof(type))


/** Initializes the fields of the given scrbuf struct and the page segment it represents */
static inline void initscrbuf(struct scrbuf* scrbuf, struct termclr bg_clr, struct termclr fg_clr, usize char_and_tags_size,
                              usize clrs_size)
{
	scrbuf->def_bg_clr = bg_clr;
	scrbuf->def_fg_clr = fg_clr;
	// We assign each buffer pointer to the right location, based on the given address and offset in the arena
	scrbuf->tagschars = OFFSET_PTR_BY(scrbuf, sizeof(struct scrbuf), struct tagchar);
	scrbuf->bg_clrs   = OFFSET_PTR_BY(scrbuf->tagschars, char_and_tags_size, union clrval);
	scrbuf->fg_clrs   = OFFSET_PTR_BY(scrbuf->bg_clrs, clrs_size, union clrval);
}

static inline void initstrbuf(struct _strbuf* strbuf, usize pagesize)
{
	strbuf->pagesize = pagesize;
	strbuf->buf      = OFFSET_PTR_BY(strbuf, sizeof(struct _strbuf), c8);
	strbuf->cursor   = 0;
	strbuf->bufsize  = s_strbuf_init_size;
}

screen* newscr(struct termclr bgclr, struct termclr fgclr, termstatefl scrflags)
{
	const struct termsize size = get_termsize();
	const usize cell_cnt       = size.cols * size.rows;

	const usize tagchars_size = cell_cnt * sizeof(c32);
	const usize clrs_size     = cell_cnt * sizeof(union clrval);
	// size of struct scrbuf + the size of its buffers including worst case padding
	const usize worst_scrbuf_memsize
		= sizeof(struct scrbuf) + (2 * (clrs_size + alignof(union clrval)) + (tagchars_size + alignof(c32)));

	const usize arena_pagesize  = WORST_SIZEOF(struct _scr_arena) + (1 + (scrflags & SCREEN_USE_VIRTUAL)) * worst_scrbuf_memsize;
	const usize strbuf_pagesize = WORST_SIZEOF(struct _strbuf) + s_strbuf_init_size;

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
	arena->termsize  = size;
	// arena is 0 initialized -> arena->refreshed = false;

	arena->strbuf = strbuf;
	initstrbuf(arena->strbuf, strbuf_pagesize);

	arena->pbuf = OFFSET_PTR_BY(arena, sizeof(struct _scr_arena), struct scrbuf);
	initscrbuf(arena->pbuf, bgclr, fgclr, tagchars_size, clrs_size);
	if (scrflags & SCREEN_USE_VIRTUAL) {
		// arena is 0 initialized, so arena->_vbuf is nullptr by default
		arena->vbuf = OFFSET_PTR_BY(arena->pbuf, worst_scrbuf_memsize, struct scrbuf);
		initscrbuf(arena->vbuf, bgclr, fgclr, tagchars_size, clrs_size);
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
struct _strbuf* strbuf_grow(const struct _strbuf* strbuf)
{
	const usize newbufsize   = strbuf->bufsize * s_strbuf_growth_rate;
	const usize new_pagesize = WORST_SIZEOF(struct _strbuf) + newbufsize;

	struct _strbuf* newstrbuf = heapalloc(new_pagesize);
	if (!newstrbuf) {
		return nullptr;
	}

	newstrbuf->pagesize = new_pagesize;
	newstrbuf->bufsize  = newbufsize;
	newstrbuf->cursor   = strbuf->cursor;
	newstrbuf->buf      = OFFSET_PTR_BY(newstrbuf, sizeof(struct _strbuf), c8);
	// -- CRITICAL : move memory (MUST be faster than or equal to memmove) -- //
	for (usize i = 0; i < strbuf->bufsize; ++i) {
		newstrbuf->buf[i] = strbuf->buf[i];
	}

	if (heapfree((struct _strbuf*)strbuf, strbuf->bufsize)) {
		// TODO : Return an error via enum so we know exactly what went wrong instead of just "true"
		heapfree(newstrbuf, new_pagesize); // I fear there is nothing we can do...
		return nullptr;
	}

	strbuf = nullptr;
	return newstrbuf;
}

static inline void strbufadd(struct _strbuf** const strbuf, const c8* str, usize strlen)
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
	for (usize i = 0; i < strlen; ++i) {
		(*strbuf)->buf[(*strbuf)->cursor + i] = str[i];
	}
	(*strbuf)->cursor += strlen;
}

static void addclrtostrbuf(screen* const scr, usize cell_idx, bool isbg)
{
	const enum clrtag tag = isbg ? scr->pbuf->tagschars[cell_idx].bgtag : scr->pbuf->tagschars[cell_idx].fgtag;
	switch (tag) {
	case CLRTAG_CODE:
		c8 clr_code_seq[U8_WORST_PARAMSEQ_LEN(1)];
		const uchar clr_code        = isbg ? scr->pbuf->bg_clrs[cell_idx].code + 10 : scr->pbuf->fg_clrs[cell_idx].code;
		const usize clr_code_seqlen = paramu8seq(clr_code_seq, (u8[]){clr_code}, 1, 'm');
		strbufadd(&scr->strbuf, clr_code_seq, clr_code_seqlen);
		break;
	case CLRTAG_RGB:
		c8 clr_rgb_seq[4 + U8_WORST_PARAMSEQ_LEN(3)];
		const struct rgb clr_rgb   = isbg ? scr->pbuf->bg_clrs[cell_idx].rgb : scr->pbuf->fg_clrs[cell_idx].rgb;
		const usize clr_rgb_seqlen = paramu8seq(clr_rgb_seq, (u8[]){isbg ? 48 : 38, 2, clr_rgb.r, clr_rgb.g, clr_rgb.b}, 5, 'm');
		strbufadd(&scr->strbuf, clr_rgb_seq, clr_rgb_seqlen);
		break;
	case CLRTAG_ID:
		c8 clr_id_seq[4 + U8_WORST_PARAMSEQ_LEN(1)];
		const u8 clr_id           = isbg ? scr->pbuf->bg_clrs[cell_idx].id : scr->pbuf->fg_clrs[cell_idx].id;
		const usize clr_id_seqlen = paramu8seq(clr_id_seq, (u8[]){isbg ? 48 : 38, 5, clr_id}, 3, 'm');
		strbufadd(&scr->strbuf, clr_id_seq, clr_id_seqlen);
		break;
	}
}

enum escerr sidxtocord(const screen* const scr, usize i, u16* x, u16* y)
{
	*y = i / scr->termsize.cols;
	*x = i - *y * scr->termsize.cols;
	(*x)++;
	(*y)++;
	return sgetcorderr(scr, *x, *y);
}

#define SWAP(x, y) \
	x ^= y;        \
	y ^= x;        \
	x ^= y

// TODO : Allow for saving strbuf to any file
/* ------------------------ *
 * --- LIBRARY HOT SPOT --- *
 * --------------------- -- */
bool srefresh(screen* const scr, bool clear)
{
	if (clear && scr->refreshed) { // Won't clear if the screen has never been refreshed
		const usize cell_cnt = scr->termsize.cols * scr->termsize.rows;
		for (usize i = 0; i < cell_cnt; ++i) {
			strbufadd(&scr->strbuf, u8" ", 1);
		}
		strbufadd(&scr->strbuf, CSI u8"H", 2);
	}

	// TODO : Account for changing termflags
	const usize cell_cnt = scr->termsize.cols * scr->termsize.rows;
	u16 last_x = 0, last_y = 0;
	for (usize i = 0; i < cell_cnt; ++i) {
		if (!scr->pbuf->tagschars[i].visible) {
			continue;
		}

		addclrtostrbuf(scr, i, true);
		addclrtostrbuf(scr, i, false);

		u16 x, y;
		sidxtocord(scr, i, &x, &y);
		if (last_x == scr->termsize.cols - 1 && x == 0) {
			strbufadd(&scr->strbuf, u8"\n", 1);
		} else if (x != last_x + 1 || y != last_y) {
			c8 mvseq[U16_WORST_PARAMSEQ_LEN(2)];
			const usize mvseq_len = paramu16seq(mvseq, (u16[]){y, x}, 2, 'H');
			strbufadd(&scr->strbuf, mvseq, mvseq_len);
		}

		const bool has_clr = (scr->pbuf->tagschars[i].bgtag || scr->pbuf->tagschars[i].fgtag);
		if (!scr->pbuf->tagschars[i].c && has_clr) {
			strbufadd(&scr->strbuf, u8" ", 1);
		} else {
			c8 mb[MAX_UTF8_CU];
			usize mb_len = cptoutf8(scr->pbuf->tagschars[i].c, mb);
			strbufadd(&scr->strbuf, mb, mb_len);
		}

		if (has_clr) {
			strbufadd(&scr->strbuf, CSI u8"m", 3);
		}
		last_x = x;
		last_y = y;
	}

	if (termprint(scr->strbuf->buf, scr->strbuf->cursor)) {
		return true;
	}

	// TODO : vscr

	if (scr->vbuf != nullptr) {
		// swap pointers to not move or copy memory (that would be ridiculous)
		// the old pbuf becomes the next vbuf
		uintptr_t addrof_pbuf = (uintptr_t)scr->pbuf;
		uintptr_t addrof_vbuf = (uintptr_t)scr->vbuf;
		SWAP(addrof_pbuf, addrof_vbuf);
		scr->pbuf = (struct scrbuf*)addrof_pbuf;
		scr->vbuf = (struct scrbuf*)addrof_vbuf;
	}
	scr->strbuf->cursor = 0;
	scr->refreshed      = true;

	return false;
}


// TODO : WTF
inline enum escerr sgetcorderr(const screen* scr, u16 x, u16 y)
{
	if (x > scr->termsize.cols)
		return ESC_ERR_COORD_X;
	if (y > scr->termsize.rows)
		return ESC_ERR_COORD_Y;
	return ESC_OK;
}

inline isize scordtoidx(const screen* scr, u16 x, u16 y)
{
	// We substract 1 to x and y since the first cell is (1, 1), not (0, 0)
	return (sgetcorderr(scr, x, y) == ESC_OK) ? (y - 1) * scr->termsize.cols + (x - 1) : -1;
}

enum escerr ssetcp(screen* restrict scr, c32 c, u16 x, u16 y)
{
	const isize idx = scordtoidx(scr, x, y);
	if (idx != -1) {
		scr->pbuf->tagschars[idx].visible = true;
		scr->pbuf->tagschars[idx].c       = c;
	}
	return sgetcorderr(scr, x, y);
}

enum escerr ssetbgclr(screen* restrict scr, struct termclr clr, u16 x, u16 y)
{
	const isize idx = scordtoidx(scr, x, y);
	if (idx != -1) {
		scr->pbuf->tagschars[idx].visible = true;
		scr->pbuf->tagschars[idx].bgtag   = clr.tag;

		scr->pbuf->bg_clrs[idx] = clr.val;
	}
	return sgetcorderr(scr, x, y);
}
enum escerr ssetfgclr(screen* restrict scr, struct termclr clr, u16 x, u16 y)
{
	const isize idx = scordtoidx(scr, x, y);
	if (idx != -1) {
		scr->pbuf->tagschars[idx].visible = true;
		scr->pbuf->tagschars[idx].fgtag   = clr.tag;

		scr->pbuf->fg_clrs[idx] = clr.val;
	}
	return sgetcorderr(scr, x, y);
}
enum escerr ssetclrpair(screen* restrict scr, struct termclr bgclr, struct termclr fgclr, u16 x, u16 y)
{
	const isize idx = scordtoidx(scr, x, y);
	if (idx != -1) {
		scr->pbuf->tagschars[idx].visible = true;
		scr->pbuf->tagschars[idx].bgtag   = bgclr.tag;
		scr->pbuf->tagschars[idx].fgtag   = fgclr.tag;

		scr->pbuf->bg_clrs[idx] = bgclr.val;
		scr->pbuf->fg_clrs[idx] = fgclr.val;
	}
	return sgetcorderr(scr, x, y);
}


enum escerr ssetvis(const screen* scr, bool visible, uint16_t x, uint16_t y)
{
	const isize idx = scordtoidx(scr, x, y);
	if (idx != -1) {
		scr->pbuf->tagschars[idx].visible = visible;
	}
	return sgetcorderr(scr, x, y);
}

void saddstr(screen* restrict scr, const c32* str32, usize strlen, u16 x, u16 y)
{
	for (usize i = 0; i < strlen; ++i) {
		ssetcp(scr, str32[i], x, y);
	}
}

