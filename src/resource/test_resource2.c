#include <common/init_cleanup_list.h>
#include <resource/resource_proc.h>
#include <resource/resource.h>

#include <common/debug.h>
#include <common/exception.h>
#include <bitmap/bitmap.h>

#include <unistd.h>
#include <signal.h>


init_func_t init_funcs[] = {
	__dbg_init,
	__yconf_init,
	NULL,
};

cleanup_func_t cleanup_funcs[] = {
	__yconf_cleanup,
	__caches_cleanup,
	__dbg_cleanup,
	NULL,
};

int main()
{
	/* test start and stop */

	struct exception_t exp;
	TRY(exp) {
		do_init();
		launch_resource_process();

		struct bitmap_t * b = get_resource("0:file:/tmp/xxx.png",
				(deserializer_t)bitmap_deserialize);
		VERBOSE(SYSTEM, "get bitmap! id=%s\n", b->id);
		free_bitmap(b);

		b = get_resource("0:file:./no_alpha.png",
				(deserializer_t)bitmap_deserialize);
		VERBOSE(SYSTEM, "get bitmap! id=%s\n", b->id);
		free_bitmap(b);

		b = get_resource("0:file:./have_alpha.png",
				(deserializer_t)bitmap_deserialize);
		VERBOSE(SYSTEM, "get bitmap! id=%s\n", b->id);
		free_bitmap(b);

		b = get_resource("0:file:./largepng.png",
				(deserializer_t)bitmap_deserialize);
		VERBOSE(SYSTEM, "get bitmap! id=%s\n", b->id);
		free_bitmap(b);

		b = get_resource("0:file:./jpegtest.jpeg",
				(deserializer_t)bitmap_deserialize);
		VERBOSE(SYSTEM, "get bitmap! id=%s\n", b->id);
		free_bitmap(b);

	} FINALLY {
		if ((exp.type != EXP_RESOURCE_PEER_SHUTDOWN)
				&& (exp.type != EXP_RESOURCE_PROCESS_FAILURE)) {
			shutdown_resource_process();
			do_cleanup();
		}
	} CATCH (exp) {
		switch (exp.type) {
			case EXP_RESOURCE_PEER_SHUTDOWN:
			case EXP_RESOURCE_PROCESS_FAILURE:
				WARNING(SYSTEM, "resource process exit before main process\n");
				do_cleanup();
				break;
			default:
				RETHROW(exp);
		}
	}

	return 0;
}

// vim:ts=4:sw=4

