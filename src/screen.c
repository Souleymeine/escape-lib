#include <stddef.h>
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
static size_t S_SCRSTR_INIT_BUFSIZE = 262144; // 2^18
static float S_SCRSTR_GROWTH_RATE   = 1.5f;

void scrmemparams(size_t next_scrstr_bufsize, float new_scrstr_growth_rate)
{
	S_SCRSTR_INIT_BUFSIZE = next_scrstr_bufsize;
	S_SCRSTR_GROWTH_RATE  = new_scrstr_growth_rate;
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

/** Returns a struct with the pointer all pointing to the right addresses based on the given _screenbuf ptr */
static inline void initscrbuf(struct scrbuf* scrbuf_ptr, union termclr bg_clr, union termclr fg_clr, size_t char_bufsize,
                              size_t cellflag_bufsize, size_t clr_bufsize)
{
	scrbuf_ptr->bg_clr = bg_clr;
	scrbuf_ptr->fg_clr = fg_clr;
	// We assign each buffer pointer to the right location, based on the given address and offset in the arena
	scrbuf_ptr->chars     = (char32_t*)scrbuf_ptr + sizeof(scrbuf_ptr);
	scrbuf_ptr->cellflags = (uchar*)(scrbuf_ptr->chars + char_bufsize);
	scrbuf_ptr->bg_clrs   = (union termclr*)(scrbuf_ptr->cellflags + cellflag_bufsize);
	scrbuf_ptr->fg_clrs   = (union termclr*)(scrbuf_ptr->bg_clrs + clr_bufsize);
}

screen* newscr(union termclr bg_clr, union termclr fg_clr, termstatefl scrflags)
{
	const struct termsize termsize = get_termsize();
	const size_t cell_cnt          = termsize.cols * termsize.rows;
	const size_t char_bufsize      = cell_cnt * sizeof(char32_t);
	const size_t clr_bufsize       = cell_cnt * sizeof(union termclr);
	const size_t cellflag_bufsize  = cell_cnt * sizeof(uchar);
	// size of struct scrbuf + the size of its buffers
	const size_t scr_byte_size = sizeof(struct scrbuf) + (2 * clr_bufsize + char_bufsize + cellflag_bufsize);

	const bool use_vscr = scrflags & USE_VSCR;

	const size_t pagesize = sizeof(struct _scr_arena) + (1 + use_vscr) * scr_byte_size;
	const void* arena_ptr = heapalloc(pagesize);
	if (!arena_ptr) {
		return nullptr;
	}
	const void* strbuf_ptr = heapalloc(S_SCRSTR_INIT_BUFSIZE);
	if (!strbuf_ptr) {
		return nullptr;
	}

	// We cast arena_ptr to byte* because sizeof returns a size in bytes. If you don't understand review your pointer
	// arithmetics.
	struct _scr_arena* scr_arena = (struct _scr_arena*)arena_ptr;
	const byte* pbufptr          = (byte*)arena_ptr + sizeof(scr_arena);

	scr_arena->_termflags = (scrflags & HOLD_TERMFLAGS) ? (scrflags & ~(HOLD_TERMFLAGS | USE_VSCR)) : -1;
	scr_arena->_pagesize  = pagesize;
	scr_arena->_size      = termsize;
	scr_arena->_strbuf    = (struct _scrstrbuf){.buf = (char*)strbuf_ptr, .curridx = 0, .currsize = S_SCRSTR_INIT_BUFSIZE};
	scr_arena->_pbuf      = (struct scrbuf*)pbufptr;
	scr_arena->_vbuf      = use_vscr ? (struct scrbuf*)(pbufptr + scr_byte_size) : nullptr;

	initscrbuf(scr_arena->_pbuf, bg_clr, fg_clr, char_bufsize, cellflag_bufsize, clr_bufsize);
	if (use_vscr) {
		initscrbuf(scr_arena->_vbuf, bg_clr, fg_clr, char_bufsize, cellflag_bufsize, clr_bufsize);
	}

	return scr_arena;
}

bool freescr(screen* scr)
{
	// Don't mess up the order ! string buf THEN scr
	if (heapfree(scr->_strbuf.buf, scr->_strbuf.currsize) || heapfree(scr, scr->_pagesize)) {
		return true;
	}
	scr = nullptr;
	return false;
}

/* ------------------------ *
 * --- LIBRARY HOT SPOT --- *
 * ------------------------ */
bool strbuf_realloc(struct _scrstrbuf* strbuf)
{
	const size_t newsize = strbuf->currsize * S_SCRSTR_GROWTH_RATE;
	char* newbuf         = heapalloc(newsize);
	if (!newbuf) {
		return true;
	}

	// -- CRITICAL : move memory (MUST be faster than or equal to memcpy) -- //
	for (size_t i = 0; i < strbuf->currsize; ++i) {
		newbuf[i] = strbuf->buf[i];
	}

	char* oldbuf   = strbuf->buf;
	size_t oldsize = strbuf->currsize;

	strbuf->buf      = (char*)newbuf;
	strbuf->currsize = newsize;

	// Return error AFTER assigning newbuf/size to strbuf, that would be even more desastrous otherwise
	if (heapfree(oldbuf, oldsize)) {
		return true;
	}

	return false;
}

static inline void strbufadd(struct _scrstrbuf* restrict strbuf, const char* str, size_t strlen)
{
	if (strbuf->curridx + strlen > strbuf->currsize) {
		strbuf_realloc(strbuf);
	}
	for (size_t i = 0; i < strlen; ++i) {
		strbuf->buf[strbuf->curridx + i] = str[i];
	}
	strbuf->curridx += strlen;
}

// TODO : Allow for saving strbuf to any file
/* ------------------------ *
 * --- LIBRARY HOT SPOT --- *
 * --------------------- -- */
bool srefresh(screen* scr)
{
	const size_t cell_cnt = scr->_size.cols * scr->_size.rows;
	for (size_t i = 0; i < cell_cnt; ++i) {
		strbufadd(&scr->_strbuf, "FAAAHHH", 7);
	}

	long bytes_written;
	const ulong bytes_to_write = scr->_strbuf.curridx + 1;

#if __unix__
	bytes_written = write(STDOUT_FILENO, scr->_strbuf.buf, bytes_to_write);
	if (bytes_written == -1 || bytes_written != (long)bytes_to_write) {
		return true;
	}
#elif _WIN32
	if (WriteConsole(*get_g_stdout_hndl(), scr._strbuf.buf, &bytes_written, nullptr) || bytes_written != (long)bytes_to_write) {
		return true;
	}
#endif

	scr->_strbuf.curridx = 0;

	return false;
}


inline uchar scorderr(const screen* scr, u16 x, u16 y)
{
	unsigned char flags = 0;
	if (x > scr->_size.cols) flags |= COORD_X_OUT;
	else flags |= COORD_X_IN;
	if (y > scr->_size.rows) flags |= COORD_Y_OUT;
	else flags |= COORD_Y_IN;

	return flags;
}

long scordtoidx(const screen* scr, u16 x, u16 y)
{
	if (!scorderr(scr, x, y)) {
		return -1;
	}
	return (y - 1) * scr->_size.cols + (x - 1); // -1 since the first cell is (1, 1), not (0, 0)
}

errflcord ssetc32(screen* restrict scr, char32_t c32, u16 x, u16 y)
{
	const uchar err = scorderr(scr, x, y);
	if (err) {
		return err;
	}
	scr->_pbuf->chars[scordtoidx(scr, x, y)] = c32;
	return 0;
}

errflcord ssetclr(screen* restrict scr, union termclr clr, uchar clrflags, u16 x, u16 y)
{
	const uchar idx = scordtoidx(scr, x, y);
	if (idx) {
		return idx;
	}

	scr->_pbuf->cellflags[idx] |= clrflags | CELL_VISIBLE;
	// points to the correct array element depending on the given flags
	union termclr* cell
		= clrflags & (CELL_BG_CLRCODE | CELL_BG_CLRID | CELL_BG_CLRRGB) ? &scr->_pbuf->bg_clrs[idx] : &scr->_pbuf->fg_clrs[idx];
	*cell = clr;

	return 0;
}

