
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
		struct io_t * io = NULL;
#if 0
		io = io_open("XP3",
				"./archive.xp3:bbb.png");
		io_close(io);
		io = io_open("XP3",
				"./test.xp3:bbb.png");
		io_close(io);
#endif
		io = io_open("XP3",
				"/home/wn/Windows/Fate/data.xp3:bbb.png");
		io_close(io);
	} FINALLY {}
	CATCH(exp) {
		RETHROW(exp);
	}
	do_cleanup();
	return 0;
}

// vim:ts=4:sw=4

