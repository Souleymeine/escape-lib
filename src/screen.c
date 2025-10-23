#include <stddef.h>
#include <uchar.h>
#if __unix__
#include <sys/mman.h>
#elif _WIN32
#include <memoryapi.h>
#endif

#include "escdef.h"
#include "screen.h"
#include "termsize.h"

#include "_escdef.h"

/** Returns a struct with the pointer all pointing to the right addresses based on the given _screenbuf ptr */
static inline void initscrbuf(struct scrbuf* restrict scrbuf_ptr, union termclr bg_clr, union termclr fg_clr, flags termflags,
                              size_t char_bufsize, size_t cellflag_bufsize, size_t clr_bufsize)
{
	scrbuf_ptr->termflags = termflags;
	scrbuf_ptr->bg_clr    = bg_clr;
	scrbuf_ptr->fg_clr    = fg_clr;
	// We assign each buffer pointer to the right location, based on the given address and offset in the arena
	scrbuf_ptr->chars     = (char32_t*)scrbuf_ptr + sizeof(scrbuf_ptr);
	scrbuf_ptr->cellflags = (uchar*)(scrbuf_ptr->chars + char_bufsize);
	scrbuf_ptr->bg_clrs   = (union termclr*)(scrbuf_ptr->cellflags + cellflag_bufsize);
	scrbuf_ptr->fg_clrs   = (union termclr*)(scrbuf_ptr->bg_clrs + clr_bufsize);
}

screen* newscr(union termclr bg_clr, union termclr fg_clr, flags scrflags)
{
	const struct termsize termsize = get_termsize();
	const size_t cell_cnt          = termsize.cols * termsize.rows;
	const size_t char_bufsize      = cell_cnt * sizeof(char32_t);
	const size_t clr_bufsize       = cell_cnt * sizeof(union termclr);
	const size_t cellflag_bufsize  = cell_cnt * sizeof(uchar);
	// size of struct scrbuf + the size of its buffers
	const size_t tot_scr_size = sizeof(struct scrbuf) + (2 * clr_bufsize + char_bufsize + cellflag_bufsize);

	const bool use_vscr = scrflags & USE_VSCR;

	const size_t pagesize = sizeof(struct _scr_arena) + (1 + use_vscr) * tot_scr_size;

#if __unix__

	// MAP_ANONYMOUS, when used with -1 as fd, has the side effect of initializing the page with 0s, which is desired
	const void* arena_ptr = mmap(nullptr, pagesize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (arena_ptr == MAP_FAILED) {
		return nullptr;
	}

#elif _WIN32

	// VirtualAlloc initializes page with 0s, which is desired
	const LPVOID arena_ptr = VirtualAlloc(nullptr, pagesize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!arena_ptr) {
		return nullptr;
	}

#endif

	// We cast arena_ptr to byte* because sizeof returns a size in bytes. If you don't understand review your pointer arithmetics.
	struct _scr_arena* scr_arena = (struct _scr_arena*)arena_ptr;
	const byte* pbufptr          = (byte*)arena_ptr + sizeof(scr_arena);

	scr_arena->_pagesize = pagesize;
	scr_arena->_termsize = termsize;
	scr_arena->_pbuf     = (struct scrbuf*)pbufptr;
	scr_arena->_vbuf     = use_vscr ? (struct scrbuf*)(pbufptr + tot_scr_size) : nullptr;

	const flags termflags = (scrflags & HOLD_TERMFLAGS) ? (scrflags & ~(HOLD_TERMFLAGS | USE_VSCR)) : -1;
	initscrbuf(scr_arena->_pbuf, bg_clr, fg_clr, termflags, char_bufsize, cellflag_bufsize, clr_bufsize);
	if (use_vscr) {
		initscrbuf(scr_arena->_vbuf, bg_clr, fg_clr, termflags, char_bufsize, cellflag_bufsize, clr_bufsize);
	}

	return scr_arena;
}

inline bool freescr(screen* scr)
{
#if __unix__

	if (munmap(scr, scr->_pagesize) == -1) {
		return 1;
	}

#elif _WIN32

	if (VirtualFree(scr, scr->pagesize, MEM_RELEASE) == 0) {
		return 1;
	}

#endif

	scr = nullptr;
	return 0;
}

