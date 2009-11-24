/* 
 * cleanup_list.c
 * by WN @ Nov. 23, 2009
 */

#include <common/defs.h>
#include <common/list.h>
#include <common/cleanup_list.h>
#include <assert.h>

struct cleanup_entry {
	cleanup_func_t func;
	uintptr_t arg;
};

static struct cleanup_entry entries[NR_CLEANUP_ENTRIES];
static int nr_entries = 0;

void
register_cleanup(cleanup_func_t func, uintptr_t arg)
{
	nr_entries ++;
	assert(nr_entries <= NR_CLEANUP_ENTRIES);
	entries[nr_entries - 1].func = func;
	entries[nr_entries - 1].arg = arg;
}

void
do_cleanup(void)
{
	for (int i = 0; i < nr_entries; i++) {
		entries[i].func(entries[i].arg);
	}
}

// vim:ts=4:sw=4

