#include <config.h>
#include <common/debug.h>
#include <common/exception.h>
#include <common/init_cleanup_list.h>
#include <common/mm.h>
#include <common/functionor.h>
#include <io/io.h>

#include <signal.h>

init_func_t init_funcs[] = {
	__dbg_init,
	__yconf_init,
	NULL,
};

cleanup_func_t cleanup_funcs[] = {
	__yconf_cleanup,
	__dbg_cleanup,
	NULL,
};


int
main()
{
	do_init();
	struct io_t * res = NULL;
	struct exception_t exp;
	TRY(exp) {
		res = res_open("file", "/tmp/abc");
		assert(res != NULL);
		char data[5];
		res_read(res, data, 1, 5);
		VERBOSE(SYSTEM, "read str=\"%s\"\n", data);
	} FINALLY {
		if (res != NULL) {
			res_close(res);
			res = NULL;
		}
	} CATCH (exp) {
		RETHROW(exp);
	}
	
	do_cleanup();
	raise(SIGUSR1);
	return 0;
}

// vim:ts=4:sw=4
