
#include <config.h>
#include <common/debug.h>
#include <common/exception.h>
#include <common/init_cleanup_list.h>
#include <common/mm.h>
#include <common/functionor.h>
#include <io/io.h>

init_func_t init_funcs[] = {
	__dbg_init,
	__yconf_init,
	NULL,
};

cleanup_func_t cleanup_funcs[] = {
	__yconf_cleanup,
	__io_cleanup,
	__caches_cleanup,
	__dbg_cleanup,
	NULL,
};


int main()
{
	do_init();
	struct exception_t exp;
	TRY(exp) {
		struct io_functionor_t * io_f = NULL;

		io_f = get_io_handler("XP3");
		assert(io_f != NULL);
		char ** table = iof_command(io_f, "readdir:/home/wn/Windows/Fate/data.xp3", NULL);
		assert(table != NULL);
		char ** ptr = table;
		while (*ptr != NULL) {
			VERBOSE(SYSTEM, "%p, %s\n", *ptr, *ptr);
			ptr ++;
		}
		xfree(table);

		struct io_t * io = io_open("XP3", "/home/wn/Windows/Fate/data.xp3:startup.tjs");
		assert(io != NULL);
		io_close(io);

	} FINALLY {}
	CATCH(exp) {
		RETHROW(exp);
	}
	do_cleanup();
	return 0;
}

// vim:ts=4:sw=4

