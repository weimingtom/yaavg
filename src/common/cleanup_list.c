/* 
 * cleanup_list.c
 * by WN @ Nov. 23, 2009
 */

#include <common/defs.h>
#include <common/debug.h>
#include <common/list.h>
#include <common/cleanup_list.h>

static LIST_HEAD(cleanup_list);
static LIST_HEAD(cleanup_list_hard);

void
register_cleanup_entry(struct cleanup_list_entry * e)
{
	list_add_tail(&(e->list), &cleanup_list);
}

void
register_cleanup_entry_hard(struct cleanup_list_entry * e)
{
	list_add_tail(&(e->list), &cleanup_list_hard);
}


void
do_cleanup(void)
{
	struct cleanup_list_entry * pos, *n;
	list_for_each_entry_safe(pos, n, &cleanup_list, list) {
		list_del(&(pos->list));
		pos->func(pos->arg);
	}
}

static void
hard_cleanup(void)
{
	struct cleanup_list_entry * pos, *n;
	list_for_each_entry_safe(pos, n, &cleanup_list_hard, list) {
		list_del(&(pos->list));
		pos->func(pos->arg);
	}
}

static void ATTR(constructor)
cleanup_list_init(void)
{
	atexit(hard_cleanup);
}

// vim:ts=4:sw=4

