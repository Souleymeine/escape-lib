#include <assert.h>
#include <string.h>
#if __unix__
#include <sys/mman.h>
#include <unistd.h>
#elif _WIN32
#include <memoryapi.h>
#endif

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
	char* bytes;
	size_t cursor;
	size_t capacity;
	bool growable;
	float growth_rate;
};

struct rndr_arena {
	void* bytes;
	size_t heapsize;
};

static struct grid g_physical_grid;
static struct grid g_virtual_grid;
static struct strbuf g_string_buffer;
static struct rndr_arena g_rndr_arena;
// indicates if the screen has been refreshed at least once
static bool g_refreshed = false;
static bool g_use_virtual_grid = false;

/** --- Windows/POSIX basic heap de/allocator --- */

[[nodiscard("What are you doing?")]]
static ESC_RESULT_PTR(void) heapalloc(size_t pagesize)
{
	// TODO : handle all relevant values of errno
#if __unix__
	// MAP_ANONYMOUS, when used with -1 as fd, has the side effect of initializing the page with 0s, which is desired
	void* page = mmap(nullptr, pagesize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (page == MAP_FAILED) {
#elif _WIN32
	LPVOID page = VirtualAlloc(nullptr, pagesize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!page) {
#endif
		return ESC_RESPTR_ERR(void, ESC_ERR_OUT_OF_MEMORY);
	}
	return ESC_RESPTR_VAL(void, page);
}

static ESC_RESULT(void) heapfree(void* ptr, size_t len)
{
#if __unix__
	if (munmap(ptr, len) == -1) {
#elif _WIN32
	if (VirtualFree(ptr, len, MEM_RELEASE) == 0) {
#endif
		return ESC_RES_ERR(void, ESC_ERR_CANNOT_FREE_MEMORY);
	}
	return ESC_RES_NOERR(void);
}

#define PADDING_BETWEEN(prev_byte_count, next_T) (prev_byte_count % sizeof(next_T))

static ESC_RESULT(void) strbuf_init(const struct esc_strbuf_impl* strbuf_impl, void* page, size_t* ofst)
{
	g_string_buffer.cursor = 0;
	g_string_buffer.growable = false;

	switch (strbuf_impl->buf_tag) {
	case ESC_STRBUF_CIRCULAR_STACK:
		g_string_buffer.bytes    = strbuf_impl->circular_stack.buf;
		g_string_buffer.capacity = strbuf_impl->circular_stack.size;
		break;
	case ESC_STRBUF_HEAP:
		if (strbuf_impl->heap.growth_rate == 0.0f) { // heap circular buffer
			g_string_buffer.bytes    = (char*)((uintptr_t)page + *ofst);
			g_string_buffer.capacity = strbuf_impl->heap.size;
			*ofst += strbuf_impl->heap.size;
		} else {
			assert(strbuf_impl->heap.growth_rate > 0);
			
			const ESC_RESULT_PTR(void) strbuf_page = heapalloc(strbuf_impl->heap.size);
			ESC_TRY(void, strbuf_page);

			g_string_buffer.growable    = true;
			g_string_buffer.bytes       = strbuf_page.val;
			g_string_buffer.capacity    = strbuf_impl->heap.size;
			g_string_buffer.growth_rate = 1.0f + strbuf_impl->heap.growth_rate;
		}
		break;
	}

	return ESC_RES_NOERR(void);
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

ESC_RESULT(void) esc_initscr(const struct esc_strbuf_impl* strbuf_impl, bool virtual_grid, struct esc_clr bgclr, struct esc_clr fgclr)
{
	g_use_virtual_grid = virtual_grid;

	/* --- Calculate sizes --- */

	size_t arena_heapsize = 0;

	const struct esc_termsize size = esc_getsize();
	const size_t cell_count        = size.cols * size.rows;

	const size_t cts_bytesize     = cell_count * (sizeof(char_and_tag));
	const size_t clrvals_bytesize = cell_count * (sizeof(union esc_clrval));
	const size_t grid_bytesize    = cts_bytesize + PADDING_BETWEEN(cts_bytesize, union esc_clrval) + clrvals_bytesize * 2;
	
	arena_heapsize += grid_bytesize * (1 + virtual_grid)
		+ (virtual_grid * PADDING_BETWEEN(clrvals_bytesize, char_and_tag)); // padding between the two grids (if virtual_grid)
	
	// We add the strbuf in the arena if it's circular and heap allocated as it won't jump around while growing
	if (strbuf_impl->buf_tag == ESC_STRBUF_CIRCULAR_STACK) {
		arena_heapsize += strbuf_impl->circular_stack.size;
	} else if (strbuf_impl->buf_tag == ESC_STRBUF_HEAP && strbuf_impl->heap.growth_rate == 0) {
		arena_heapsize += strbuf_impl->heap.size;
	}

	/* --- Allocate and set --- */

	const ESC_RESULT_PTR(void) page = heapalloc(arena_heapsize);
	ESC_TRY(void, page);
	g_rndr_arena = (struct rndr_arena) {
		.bytes    = page.val,
		.heapsize = arena_heapsize,
	};
	
	size_t arena_ofst = 0;

	grid_init(&g_physical_grid, page.val, &arena_ofst, size, bgclr, fgclr, cts_bytesize, clrvals_bytesize);
	assert(arena_ofst == grid_bytesize);
	if (virtual_grid) {
		grid_init(&g_virtual_grid, page.val, &arena_ofst, size, bgclr, fgclr, cts_bytesize, clrvals_bytesize);
		assert(arena_ofst == 2 * grid_bytesize + PADDING_BETWEEN(grid_bytesize, union esc_clrval));
	}

	ESC_TRY(void, strbuf_init(strbuf_impl, page.val, &arena_ofst));
	assert(arena_ofst == arena_heapsize);

	return ESC_RES_NOERR(void);
}

ESC_RESULT(void) esc_deinitscr()
{
	ESC_TRY(void, heapfree(g_rndr_arena.bytes, g_rndr_arena.heapsize));
	if (g_string_buffer.growable) {
		ESC_TRY(void, heapfree(g_string_buffer.bytes, g_string_buffer.capacity));
	}
	return ESC_RES_NOERR(void);
}

/* ------------------------ *
 * --- LIBRARY HOT SPOT --- *
 * ------------------------ */
static ESC_RESULT(void) strbuf_grow()
{
	assert(g_string_buffer.growable);

	const size_t new_capacity = g_string_buffer.capacity * g_string_buffer.growth_rate;
	const ESC_RESULT_PTR(void) new_page = heapalloc(new_capacity);
	ESC_TRY(void, new_page);

	memcpy(new_page.val, g_string_buffer.bytes, g_string_buffer.cursor);

	ESC_TRY(void, heapfree(g_string_buffer.bytes, g_string_buffer.capacity));

	g_string_buffer.bytes    = new_page.val;
	g_string_buffer.capacity = new_capacity;

	return ESC_RES_NOERR(void);
}

static bool strbuf_isfull()
{
	/* Example, size of 3 :
	 * [ | | ] <- bytes (3)
	 *  0 1 2  <- indices (cursor, max 2 (3 - 1)) */
	return g_string_buffer.cursor == g_string_buffer.capacity - 1;
}

static bool strbuf_willoverflow(size_t ofst)
{
	/* Example, size of 3 :
	 * [ | | ] <- bytes (3)
	 *  0 1 2  <- indices (cursor, max 2 (3 - 1)) */
	return g_string_buffer.cursor + ofst > g_string_buffer.capacity - 1;
}

static size_t strbuf_capacityleft()
{
	return g_string_buffer.capacity - g_string_buffer.cursor - 1;
}

static ESC_RESULT(void) strbuf_flush()
{
	if (g_string_buffer.cursor > 0) {
		ESC_TRY(void, esc_termwrite(ESC_STDOUT, g_string_buffer.bytes, g_string_buffer.cursor)
		);
		g_string_buffer.cursor = 0;
	}
	return ESC_RES_NOERR(void);
}

// Copies str of size len asserting it can fit at the string buffer's current cursor
static void strbuf_copy(const char8_t* str, size_t len)
{
	memcpy(g_string_buffer.bytes + g_string_buffer.cursor, str, len);
	g_string_buffer.cursor += len;
}

// Adds str of size len to the string buffer while growing it or flushing if necessary
static ESC_RESULT(void) strbuf_add(const char8_t* str, size_t len)
{
	assert(len > 0);

	if (strbuf_isfull()) {
		ESC_TRY(void, strbuf_flush());
	} else if (strbuf_willoverflow(len)) {
		if (g_string_buffer.growable) {
			ESC_TRY(void, strbuf_grow());
		} else {
			const size_t capacity_left = strbuf_capacityleft();
			strbuf_copy(str, capacity_left);
			ESC_TRY(void, strbuf_flush());

			const size_t bytes_left = len - capacity_left;
			ESC_TRY(void, strbuf_add(str + capacity_left, bytes_left));
		}
	}

	strbuf_copy(str, len);
	return ESC_RES_NOERR(void);
}

static ESC_RESULT(void) strbuf_addclr(const struct grid* g, size_t cell_idx, bool isbg)
{
	char8_t seq[4 + ESC_U8_WORST_PARAMSEQ_LEN(3)]; // biggest possible bufsize of all the 3 color formats (rgb)
	size_t len;
	switch (isbg ? g->chars_tags[cell_idx].bgclr_tag : g->chars_tags[cell_idx].fgclr_tag) {
	case ESC_CLRTAG_CODE:
		const uint8_t code = isbg ? g->bg_clrs[cell_idx].code + 10 : g->fg_clrs[cell_idx].code;
		len = esc_u8paramseq(seq, (uint8_t[]){code}, 1, 'm');
		break;
	case ESC_CLRTAG_RGB:
		const struct esc_rgb rgb = isbg ? g->bg_clrs[cell_idx].rgb : g->fg_clrs[cell_idx].rgb;
		len = esc_u8paramseq(seq, (uint8_t[]){ isbg ? 48 : 38, 2, rgb.r, rgb.g, rgb.b }, 5, 'm');
		break;
	case ESC_CLRTAG_ID:
		const uint8_t id = isbg ? g->bg_clrs[cell_idx].id : g->fg_clrs[cell_idx].id;
		len = esc_u8paramseq(seq, (uint8_t[]){ isbg ? 48 : 38, 5, id }, 3, 'm');
		break;
	}

	return strbuf_add(seq, len);
}

// TODO : Allow for saving strbuf to any file
/* ------------------------ *
 * --- LIBRARY HOT SPOT --- *
 * --------------------- -- */
ESC_RESULT(void) esc_refresh(bool clear)
{
	const size_t pgrid_cell_cnt = g_physical_grid.size.rows * g_physical_grid.size.cols;

	if (clear && g_refreshed && !g_use_virtual_grid) { // Won't clear if the screen has never been refreshed
		for (size_t i = 0; i < pgrid_cell_cnt; ++i) {
			ESC_TRY(void, strbuf_add(u8" ", 1));
		}
		ESC_TRY(void, strbuf_add(CSI u8"H", 2));
	}

	struct esc_coord last_coord = {};
	for (size_t i = 0; i < pgrid_cell_cnt; i++) {
		if (!g_physical_grid.chars_tags[i].visible) {
			continue;
		}

		ESC_TRY(void, strbuf_addclr(&g_physical_grid, i, true));
		ESC_TRY(void, strbuf_addclr(&g_physical_grid, i, false));

		// No need to bounds check since we use the grid's own size, not user data
		const struct esc_coord coord = esc_idxtocoord(i).val;

		if (last_coord.x == g_physical_grid.size.cols - 1 && coord.x == 0) {
			ESC_TRY(void, strbuf_add(u8"\n", 1));
		} else if (coord.x != last_coord.x + 1 || coord.y != last_coord.y) {
			char8_t mvseq[ESC_U16_WORST_PARAMSEQ_LEN(2)];
			const size_t mvseq_len = esc_u16paramseq(mvseq, (uint16_t[]){coord.y, coord.x}, 2, 'H');
			ESC_TRY(void, strbuf_add(mvseq, mvseq_len));
		}

		const bool has_clr = (g_physical_grid.chars_tags[i].bgclr_tag || g_physical_grid.chars_tags[i].fgclr_tag);
		if (!g_physical_grid.chars_tags[i].c && has_clr) {
			ESC_TRY(void, strbuf_add(u8" ", 1));
		} else {
			char8_t mb[ESC_MAX_UTF8_CU];
			size_t mb_len = esc_cptomb(g_physical_grid.chars_tags[i].c, mb).val;
			ESC_TRY(void, strbuf_add(mb, mb_len));
		}
		if (has_clr) {
			ESC_TRY(void, strbuf_add(CSI u8"m", 3));
		}
		last_coord = coord;
	}

	// TODO : vgrid
	
	g_refreshed = true;

	return strbuf_flush();
}


static ESC_RESULT(void) grid_coordboundscheck(uint16_t x, uint16_t y)
{
	if (x > g_physical_grid.size.cols) {
		if (y > g_physical_grid.size.rows) goto xy_oob;
		return ESC_RES_ERR(void, ESC_ERR_CELL_X_OOB);
	} else if (y > g_physical_grid.size.rows) {
		if (x > g_physical_grid.size.cols) goto xy_oob;
		return ESC_RES_ERR(void, ESC_ERR_CELL_Y_OOB);
	} else {
		return ESC_RES_NOERR(void);
	}
xy_oob: // goto generates smaller code ftw
	return ESC_RES_ERR(void, ESC_ERR_CELL_XY_OOB);
}

static ESC_RESULT(void) grid_idxboundscheck(size_t i)
{
	return (i > g_physical_grid.size.cols * g_physical_grid.size.rows)
		? ESC_RES_ERR(void, ESC_ERR_CELL_INDEX_OOB)
		: ESC_RES_NOERR(void);
}

ESC_RESULT(struct esc_coord) esc_idxtocoord(size_t i)
{
	ESC_TRY(struct esc_coord, grid_idxboundscheck(i));
	uint16_t y = i / g_physical_grid.size.cols;
	uint16_t x = i - y * g_physical_grid.size.cols;
	return ESC_RES_VAL(struct esc_coord, (struct esc_coord) {
		.y = y,
		.x = x,
	});
}

ESC_RESULT(size_t) esc_coordtoidx(uint16_t x, uint16_t y)
{
	ESC_TRY(size_t, grid_coordboundscheck(x, y));
	return ESC_RES_VAL(size_t, y * g_physical_grid.size.cols + x);
}

ESC_RESULT(void) esc_setcp(char32_t c, uint16_t x, uint16_t y)
{
	const ESC_RESULT(size_t) idx = esc_coordtoidx(x, y);
	ESC_TRY(void, idx);

	g_physical_grid.chars_tags[idx.val].visible = true;
	g_physical_grid.chars_tags[idx.val].c       = c;

	return ESC_RES_NOERR(void);
}

ESC_RESULT(void) esc_setbgclr(struct esc_clr clr, uint16_t x, uint16_t y)
{
	const ESC_RESULT(size_t) idx = esc_coordtoidx(x, y);
	ESC_TRY(void, idx);

	g_physical_grid.chars_tags[idx.val].visible   = true;
	g_physical_grid.chars_tags[idx.val].bgclr_tag = clr.tag;

	g_physical_grid.bg_clrs[idx.val] = clr.val;

	return ESC_RES_NOERR(void);
}
ESC_RESULT(void) esc_setfgclr(struct esc_clr clr, uint16_t x, uint16_t y)
{
	const ESC_RESULT(size_t) idx = esc_coordtoidx(x, y);
	ESC_TRY(void, idx);

	g_physical_grid.chars_tags[idx.val].visible   = true;
	g_physical_grid.chars_tags[idx.val].fgclr_tag = clr.tag;

	g_physical_grid.fg_clrs[idx.val] = clr.val;

	return ESC_RES_NOERR(void);
}
ESC_RESULT(void) esc_setclrpair(struct esc_clr bgclr, struct esc_clr fgclr, uint16_t x, uint16_t y)
{
	const ESC_RESULT(size_t) idx = esc_coordtoidx(x, y);
	ESC_TRY(void, idx);

	g_physical_grid.chars_tags[idx.val].visible   = true;
	g_physical_grid.chars_tags[idx.val].bgclr_tag = bgclr.tag;
	g_physical_grid.chars_tags[idx.val].fgclr_tag = fgclr.tag;

	g_physical_grid.bg_clrs[idx.val] = bgclr.val;
	g_physical_grid.fg_clrs[idx.val] = fgclr.val;

	return ESC_RES_NOERR(void);
}

ESC_RESULT(void) esc_setvis(bool visible, uint16_t x, uint16_t y)
{
	const ESC_RESULT(size_t) idx = esc_coordtoidx(x, y);
	ESC_TRY(void, idx);
	g_physical_grid.chars_tags[idx.val].visible = visible;
	return ESC_RES_NOERR(void);
}

