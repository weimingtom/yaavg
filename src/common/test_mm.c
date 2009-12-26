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


init_func_t init_funcs[] = {
	__dbg_init,
	NULL,
};


cleanup_func_t cleanup_funcs[] = {
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
		buf[i] = xmalloc(sizeof(struct x));
	}

	for (int i = 0; i < 5; i++) {
		xfree(buf[i]);
	}

	for (int i = 0; i < 5; i++) {
		buf[i] = xmalloc(sizeof(struct x));
	}

	for (int i = 0; i < 5; i++) {
		xfree(buf[i]);
	}

	do_cleanup();
	raise(SIGUSR1);

	return 0;
}
// vim:ts=4:sw=4
