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
	catch_var(struct io_t *, io, NULL);
	define_exp(exp);
	TRY(exp) {
		set_catched_var(io, io_open("file", "/tmp/abc"));
		assert(io != NULL);
		char data[5];
		io_read(io, data, 1, 5);
		
		VERBOSE(SYSTEM, "read str=\"%s\"\n", data);

		/* test map_to_mem */
		char * ptr = io_map_to_mem(io, 1, 4096);
		*ptr = 'x';
		VERBOSE(SYSTEM, "%s\n", ptr);
		io_release_map(io, ptr, 1, 4096);

		/* test get_internal_buffer  */
		ptr = io_get_internal_buffer(io);
		ptr[3] = 'x';
		VERBOSE(SYSTEM, "%s\n", ptr);
		io_release_internal_buffer(io, ptr);

	} FINALLY {
		get_catched_var(io);
		if (io != NULL) {
			io_close(io);
			io = NULL;
		}
	} CATCH (exp) {
		RETHROW(exp);
	}
	
	do_cleanup();
	return 0;
}

// vim:ts=4:sw=4
