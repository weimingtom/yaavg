/* 
 * cleanup_list.c
 * by WN @ Nov. 23, 2009
 */

#include <common/defs.h>
#include <common/list.h>
#include <common/init_cleanup_list.h>
#include <assert.h>

extern init_func_t init_funcs[];
extern cleanup_func_t cleanup_funcs[];

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

