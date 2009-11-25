#include <common/debug.h>
#include <common/exception.h>
#include <common/init_cleanup_list.h>
#include <common/dict.h>
#include <signal.h>
#include <stdio.h>

init_func_t init_funcs[] = {
	__dbg_init,
	__dict_init,
	NULL,
};


cleanup_func_t cleanup_funcs[] = {
	__dict_cleanup,
	__dbg_cleanup,
	NULL,
};


int main()
{
	dbg_init(NULL);
	do_init();
	struct exception_t exp;
	struct dict_t * dict;
	TRY(exp) {
		dict = dict_create(10, DICT_FL_STRKEY, NULL, 0);
	} FINALLY {
		dict_destroy(dict, NULL, 0);
	} CATCH(exp) {
		switch (exp.type) {
			default:
				RETHROW(exp);
		}
	}

	do_cleanup();
	raise(SIGUSR1);
	return 0;
}

// vim:ts=4:sw=4

