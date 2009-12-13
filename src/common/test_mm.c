/* 
 * test_mm.c
 * by WN @ Nov. 19, 2009
 */

#include <common/mm.h>
#include <common/debug.h>
#include <common/exception.h>
#include <common/init_cleanup_list.h>
#include <signal.h>

struct x {
	int a;
	int b;
	int c;
	int d;
};

static DEFINE_MEM_CACHE(cache, "test_cache_1", sizeof(struct x));

struct mem_cache_t * static_mem_caches[] = {
	&cache,
	&__dict_t_cache,
	NULL,
};

init_func_t init_funcs[] = {
	__dbg_init,
	NULL,
};


cleanup_func_t cleanup_funcs[] = {
	__mem_cache_cleanup,
	__dbg_cleanup,
	NULL,
};
int
main()
{
	do_init();
	dbg_init(NULL);
	struct x * buf[5];
	for (int i = 0; i < 5; i++) {
		buf[i] = mem_cache_alloc(&cache);
	}

	for (int i = 0; i < 5; i++) {
		mem_cache_free(&cache, buf[i]);
	}

	for (int i = 0; i < 5; i++) {
		buf[i] = mem_cache_alloc(&cache);
	}

	for (int i = 0; i < 5; i++) {
		mem_cache_free(&cache, buf[i]);
	}

	do_cleanup();
	raise(SIGUSR1);

	return 0;
}
// vim:ts=4:sw=4
