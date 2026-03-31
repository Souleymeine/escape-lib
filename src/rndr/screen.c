#include <assert.h>
#include <string.h>
#if __unix__
#include <sys/mman.h>
#include <unistd.h>
#elif _WIN32
#include <memoryapi.h>
#endif

#define ESC_SHORTHAND
#include "../../include/core.h"
#include "../../include/rndr.h"

/* --- Global state --- */

typedef struct {
	char32_t c: 21;
	enum esc_clrtag bgclr_tag: 2;
	enum esc_clrtag fgclr_tag: 2;
	bool visible: 1;
} char_and_tag; // I like thinking of bitfields as bitpacked ints, let me stay in my own delusion.

struct grid {
	struct esc_termsize size;
	struct esc_clr def_bgclr;
	struct esc_clr def_fgclr;

	char_and_tag* chars_tags;
	union esc_clrval* bg_clrs;
	union esc_clrval* fg_clrs;
};

struct strbuf {
	char* ptr;
	size_t len;
	size_t capacity;
	bool growable;
	float growth_rate;
};

struct rndr_arena {
	void* bytes;
	size_t heapsize;
};

static struct grid g_pgrid;
static struct grid g_vgrid;
static struct strbuf g_strbuf;
static struct rndr_arena g_rndr_arena;
// indicates if the screen has been refreshed at least once
static bool g_refreshed = false;
static bool g_use_vgrid = false;

/** --- Windows/POSIX basic heap de/allocator --- */
// For testing purposes
// The library's custom allocator should be as fast or faster than malloc
#define STD_MALLOC 0

#if STD_MALLOC
#include <stdlib.h>
#endif

[[nodiscard("What are you doing?")]]
static RESULT_PTR(void) heapalloc(size_t pagesize)
{
#if STD_MALLOC
	return RESPTRVAL(void, malloc(pagesize));
	// TODO : handle all relevant values of errno
#else
#if __unix__
	// MAP_ANONYMOUS, when used with -1 as fd, has the side effect of initializing the page with 0s, which is desired
	void* page = mmap(nullptr, pagesize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (page == MAP_FAILED) {
#elif _WIN32
	LPVOID page = VirtualAlloc(nullptr, pagesize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!page) {
#endif
		return RESPTRERR(void, ESC_ERR_OOM);
	}
	return RESPTRVAL(void, page);
#endif /* STD_MALLOC */
}

static void heapfree(void* ptr, size_t len [[maybe_unused]])
{
#if STD_MALLOC
	free(ptr);
#else
#if __unix__
	munmap(ptr, len);
#elif _WIN32
	VirtualFree(ptr, len, MEM_RELEASE);
#endif
#endif /* STD_MALLOC */
}

#define PADDING_BETWEEN(prev_byte_count, next_T) (prev_byte_count % sizeof(next_T))

static RESULT(void) strbuf_init(const struct esc_strbuf_impl* strbuf_impl, void* page, size_t* ofst)
{
	g_strbuf.len = 0;
	g_strbuf.growable = false;

	switch (strbuf_impl->buf_tag) {
	case ESC_STRBUF_RING_STACK:
		g_strbuf.ptr      = strbuf_impl->circular_stack.buf;
		g_strbuf.capacity = strbuf_impl->circular_stack.size;
		break;
	case ESC_STRBUF_HEAP:
		if (strbuf_impl->heap.growth_rate == 0.0f) { // heap circular buffer
			g_strbuf.ptr    = (char*)page + *ofst;
			g_strbuf.capacity = strbuf_impl->heap.size;
			*ofst += strbuf_impl->heap.size;
		} else {
			assert(strbuf_impl->heap.growth_rate > 0);
			
			const RESULT_PTR(void) strbuf_page = heapalloc(strbuf_impl->heap.size);
			TRY(void, strbuf_page);

			g_strbuf.growable    = true;
			g_strbuf.ptr         = strbuf_page.val;
			g_strbuf.capacity    = strbuf_impl->heap.size;
			g_strbuf.growth_rate = 1.0f + strbuf_impl->heap.growth_rate;
		}
		break;
	}

	return RESNOERR(void);
}

/** Initializes the fields of the given grid and offsets arena_cursor */
static void grid_init(struct grid* g, void* page, size_t* ofst,
	struct esc_termsize size,
	struct esc_clr bgclr,
	struct esc_clr fgclr,
	size_t chars_tags_bytesize, size_t clrvals_bytesize
) {
	g->size = size;
	g->def_bgclr = bgclr;
	g->def_fgclr = fgclr;
	// We assign each buffer pointer to the right location, based on the given address and offset in the arena
	g->chars_tags = (char_and_tag*)    ((uintptr_t)page + *ofst);
	g->bg_clrs    = (union esc_clrval*)((uintptr_t)g->chars_tags + chars_tags_bytesize + PADDING_BETWEEN(chars_tags_bytesize, union esc_clrval));
	g->fg_clrs    = (union esc_clrval*)((uintptr_t)g->bg_clrs    + clrvals_bytesize);

	*ofst += ((uintptr_t)g->fg_clrs + clrvals_bytesize) - ((uintptr_t)page + *ofst); // last - first
}

RESULT(void) esc_initscr(const struct esc_strbuf_impl* strbuf_impl, bool virtual_grid, struct esc_clr bgclr, struct esc_clr fgclr)
{
	g_use_vgrid = virtual_grid;

	/* --- Calculate sizes --- */

	size_t arena_heapsize = 0;

	const struct esc_termsize size = esc_gettermsize();
	const size_t cell_count        = size.cols * size.rows;

	const size_t cts_bytesize     = cell_count * (sizeof(char_and_tag));
	const size_t clrvals_bytesize = cell_count * (sizeof(union esc_clrval));
	const size_t grid_bytesize    = cts_bytesize + PADDING_BETWEEN(cts_bytesize, union esc_clrval) + clrvals_bytesize * 2;
	
	arena_heapsize += grid_bytesize * (1 + virtual_grid)
		+ (virtual_grid * PADDING_BETWEEN(clrvals_bytesize, char_and_tag)); // padding between the two grids (if virtual_grid)
	
	// We add the strbuf in the arena if it's circular and heap allocated as it won't jump around while growing
	if (strbuf_impl->buf_tag == ESC_STRBUF_HEAP && strbuf_impl->heap.growth_rate == 0) {
		arena_heapsize += strbuf_impl->heap.size;
	}

	/* --- Allocate and set --- */

	const RESULT_PTR(void) page = heapalloc(arena_heapsize);
	TRY(void, page);
	g_rndr_arena = (struct rndr_arena) {
		.bytes    = page.val,
		.heapsize = arena_heapsize,
	};
	
	size_t arena_ofst = 0;

	grid_init(&g_pgrid, page.val, &arena_ofst, size, bgclr, fgclr, cts_bytesize, clrvals_bytesize);
	assert(arena_ofst == grid_bytesize);
	if (virtual_grid) {
		grid_init(&g_vgrid, page.val, &arena_ofst, size, bgclr, fgclr, cts_bytesize, clrvals_bytesize);
		assert(arena_ofst == 2 * grid_bytesize + PADDING_BETWEEN(grid_bytesize, union esc_clrval));
	}

	TRY(void, strbuf_init(strbuf_impl, page.val, &arena_ofst));
	assert(arena_ofst == arena_heapsize);

	return RESNOERR(void);
}

void esc_deinitscr()
{
	heapfree(g_rndr_arena.bytes, g_rndr_arena.heapsize);
	if (g_strbuf.growable) {
		heapfree(g_strbuf.ptr, g_strbuf.capacity);
	}
}

/* ------------------------ *
 * --- LIBRARY HOT SPOT --- *
 * ------------------------ */
static RESULT(void) strbuf_grow()
{
	assert(g_strbuf.growable);

	const size_t new_capacity = g_strbuf.capacity * g_strbuf.growth_rate;

	const RESULT_PTR(void) new_page = heapalloc(new_capacity);
	TRY(void, new_page);

	memcpy(new_page.val, g_strbuf.ptr, g_strbuf.len);
	heapfree(g_strbuf.ptr, g_strbuf.capacity);

	g_strbuf.ptr      = new_page.val;
	g_strbuf.capacity = new_capacity;

	return RESNOERR(void);
}


static RESULT(void) strbuf_flush()
{
	if (g_strbuf.len > 0) {
		TRY(void, esc_termwrite(ESC_STDOUT, g_strbuf.ptr, g_strbuf.len));
		g_strbuf.len = 0;
	}
	return RESNOERR(void);
}

/**
 * Adds `str` of size `len` to the string buffer while growing it or flushing if necessary.
 * `len` may be bigger than `g_strbuf.capacity`.
 * Asserts `len` is non-null
 */
static RESULT(void) strbuf_add(const char8_t* str, size_t len)
{
	assert(len > 0);

	bool will_overflow = g_strbuf.len + len > g_strbuf.capacity;

	if (g_strbuf.growable && will_overflow) {
		TRY(void, strbuf_grow());
	} else if (!g_strbuf.growable) {
		const size_t capacity_left = g_strbuf.capacity - g_strbuf.len;

		if (capacity_left == 0) {
			TRY(void, strbuf_flush());
			will_overflow = g_strbuf.len + len > g_strbuf.capacity; // update since len is now 0
		}
		if (will_overflow) {
			memcpy(g_strbuf.ptr + g_strbuf.len, str, capacity_left);
			g_strbuf.len += capacity_left;
			return strbuf_add(str + capacity_left, len - capacity_left);
		}
	}

	memcpy(g_strbuf.ptr + g_strbuf.len, str, len);
	g_strbuf.len += len;
	return RESNOERR(void);
}

// Code alignment final boss
static RESULT(void) strbuf_addclr(size_t idx, bool isbg)
{
	size_t len;
	char8_t seq[4 + U8_WORST_SEQLEN(3)]; // biggest possible bufsize of all the 3 color formats (rgb)
	const union esc_clrval clr = isbg ? g_pgrid.bg_clrs[idx]              : g_pgrid.fg_clrs[idx];
	const enum esc_clrtag tag  = isbg ? g_pgrid.chars_tags[idx].bgclr_tag : g_pgrid.chars_tags[idx].fgclr_tag;
	switch (tag) {
	case ESC_CLRTAG_CODE: len = esc_u8seq(seq, ARRARG(uint8_t, {clr.code}), 'm'); break;
	case ESC_CLRTAG_RGB:  len = esc_u8seq(seq, ARRARG(uint8_t, { isbg ? 48 : 38, 2, clr.rgb.r, clr.rgb.g, clr.rgb.b }), 'm'); break;
	case ESC_CLRTAG_ID:   len = esc_u8seq(seq, ARRARG(uint8_t, { isbg ? 48 : 38, 5, clr.id }), 'm'); break;
	}

	return strbuf_add(seq, len);
}

// TODO : Allow for saving strbuf to any file
/* ------------------------ *
 * --- LIBRARY HOT SPOT --- *
 * --------------------- -- */
RESULT(void) esc_refresh(bool clear)
{
	const size_t pgrid_cell_cnt = g_pgrid.size.rows * g_pgrid.size.cols;

	if (clear && g_refreshed && !g_use_vgrid) { // Won't clear if the screen has never been refreshed
		for (size_t i = 0; i < pgrid_cell_cnt; ++i) {
			TRY(void, strbuf_add(STRLARG(u8" ")));
		}
		TRY(void, strbuf_add(STRLARG(CSI u8"H")));
	}

	struct esc_coord last_coord = {};
	for (size_t i = 0; i < pgrid_cell_cnt; i++) {
		if (!g_pgrid.chars_tags[i].visible) {
			continue;
		}

		TRY(void, strbuf_addclr(i, true));
		TRY(void, strbuf_addclr(i, false));

		// No need to bounds check since we use the grid's own size, not user data
		const struct esc_coord coord = esc_idxtocoord(i).val;

		if (last_coord.x == g_pgrid.size.cols - 1 && coord.x == 0) {
			TRY(void, strbuf_add(STRLARG(u8"\n")));
		} else if (coord.x != last_coord.x + 1 || coord.y != last_coord.y) {
			char8_t mvseq[U16_WORST_SEQLEN(2)];
			const size_t mvseq_len = esc_u16seq(mvseq, ARRARG(uint16_t, {coord.y, coord.x}), 'H');
			TRY(void, strbuf_add(mvseq, mvseq_len));
		}

		const bool has_clr = (g_pgrid.chars_tags[i].bgclr_tag || g_pgrid.chars_tags[i].fgclr_tag);
		if (!g_pgrid.chars_tags[i].c && has_clr) {
			TRY(void, strbuf_add(STRLARG(u8" ")));
		} else {
			char8_t mb[MAX_UTF8_CU];
			size_t mb_len = esc_cptomb(mb, g_pgrid.chars_tags[i].c).val;
			TRY(void, strbuf_add(mb, mb_len));
		}
		if (has_clr) {
			TRY(void, strbuf_add(STRLARG(CSI u8"m")));
		}
		last_coord = coord;
	}

	// TODO : vgrid
	
	g_refreshed = true;

	return strbuf_flush();
}


static RESULT(void) grid_boundscheck(uint16_t x, uint16_t y)
{
	if (x > g_pgrid.size.cols) {
		if (y > g_pgrid.size.rows) goto xy_oob;
		return RESERR(void, ESC_ERR_CELL_X_OOB);
	} else if (y > g_pgrid.size.rows) {
		if (x > g_pgrid.size.cols) goto xy_oob;
		return RESERR(void, ESC_ERR_CELL_Y_OOB);
	}
	return RESNOERR(void);
xy_oob: // goto ftw! (-- https://godbolt.org/z/s8j71cn45 -- , smaller code)
	return RESERR(void, ESC_ERR_CELL_XY_OOB);
}

RESULT(struct esc_coord) esc_idxtocoord(size_t i)
{
	if (i > g_pgrid.size.cols * g_pgrid.size.rows) RESERR(struct esc_coord, ESC_ERR_CELL_INDEX_OOB);

	const uint16_t y = i / g_pgrid.size.cols;
	const uint16_t x = i - y * g_pgrid.size.cols;
	return RESVAL(struct esc_coord, (struct esc_coord) {
		.y = y,
		.x = x,
	});
}

RESULT(size_t) esc_coordtoidx(uint16_t x, uint16_t y)
{
	TRY(size_t, grid_boundscheck(x, y));
	return RESVAL(size_t, y * g_pgrid.size.cols + x);
}

RESULT(void) esc_setcp(char32_t c, uint16_t x, uint16_t y)
{
	TRY(void, grid_boundscheck(x, y));
	const size_t idx = esc_coordtoidx(x, y).val;

	g_pgrid.chars_tags[idx].visible = true;
	g_pgrid.chars_tags[idx].c       = c;

	return RESNOERR(void);
}

RESULT(void) esc_setbgclr(struct esc_clr clr, uint16_t x, uint16_t y)
{
	TRY(void, grid_boundscheck(x, y));
	const size_t idx = esc_coordtoidx(x, y).val;

	g_pgrid.chars_tags[idx].visible   = true;
	g_pgrid.chars_tags[idx].bgclr_tag = clr.tag;

	g_pgrid.bg_clrs[idx] = clr.val;

	return RESNOERR(void);
}
RESULT(void) esc_setfgclr(struct esc_clr clr, uint16_t x, uint16_t y)
{
	TRY(void, grid_boundscheck(x, y));
	const size_t idx = esc_coordtoidx(x, y).val;

	g_pgrid.chars_tags[idx].visible   = true;
	g_pgrid.chars_tags[idx].fgclr_tag = clr.tag;

	g_pgrid.fg_clrs[idx] = clr.val;

	return RESNOERR(void);
}
RESULT(void) esc_setclrpair(struct esc_clr bgclr, struct esc_clr fgclr, uint16_t x, uint16_t y)
{
	TRY(void, grid_boundscheck(x, y));
	const size_t idx = esc_coordtoidx(x, y).val;

	g_pgrid.chars_tags[idx].visible   = true;
	g_pgrid.chars_tags[idx].bgclr_tag = bgclr.tag;
	g_pgrid.chars_tags[idx].fgclr_tag = fgclr.tag;

	g_pgrid.bg_clrs[idx] = bgclr.val;
	g_pgrid.fg_clrs[idx] = fgclr.val;

	return RESNOERR(void);
}

RESULT(void) esc_setvis(bool visible, uint16_t x, uint16_t y)
{
	TRY(void, grid_boundscheck(x, y));
	const size_t idx = esc_coordtoidx(x, y).val;
	g_pgrid.chars_tags[idx].visible = visible;
	return RESNOERR(void);
}

