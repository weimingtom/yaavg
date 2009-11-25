/* 
 * cleanup_list.c
 * by WN @ Nov. 23, 2009
 */

#include <common/defs.h>
#include <common/list.h>
#include <common/init_cleanup_list.h>
#include <assert.h>

static init_func_t init_funcs[] = {
	__dbg_init,
	__dict_init,
	__yconf_init,
	NULL,
};


static cleanup_func_t cleanup_funcs[] = {
	__yconf_cleanup,
	__dict_cleanup,
	__dbg_cleanup,
	NULL,
};


void
do_init(void)
{
	init_func_t * p = &init_funcs[0];
	while (*p != NULL) {
		(*p)();
		p ++;
	}
	return;
}

void
do_cleanup(void)
{
	cleanup_func_t * p = &cleanup_funcs[0];
	while (*p != NULL) {
		(*p)();
		p ++;
	}
	return;
}


// vim:ts=4:sw=4

