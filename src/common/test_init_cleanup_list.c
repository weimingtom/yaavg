#include <common/debug.h>
#include <common/exception.h>
#include <common/init_cleanup_list.h>
#include <common/dict.h>
#include <common/mm.h>
#include <signal.h>
#include <stdio.h>

init_func_t init_funcs[] = {
	__dbg_init,
	NULL,
};


cleanup_func_t cleanup_funcs[] = {
	__dbg_cleanup,
	NULL,
};


int main()
{
	dbg_init(NULL);
	do_init();
	catch_var(struct dict_t *, dict, NULL);
	define_exp(exp);
	TRY(exp) {
		set_catched_var(dict, dict_create(10, DICT_FL_STRKEY, NULL, 0));
	} FINALLY {
		get_catched_var(dict);
		dict_destroy(dict, NULL, 0);
	} CATCH(exp) {
		switch (exp.type) {
			default:
				RETHROW(exp);
		}
	}

	do_cleanup();
	return 0;
}

// vim:ts=4:sw=4

