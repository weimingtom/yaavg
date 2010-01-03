#include <common/init_cleanup_list.h>
#include <resource/resource.h>

#include <common/debug.h>
#include <common/exception.h>

#include <unistd.h>
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

int main()
{
	/* test start and stop */
	launch_resource_process();
	do_init();
	shutdown_resource_process();
	do_cleanup();

	int pid;

	struct exception_t exp;
	TRY(exp) {
		pid = launch_resource_process();
		do_init();

		kill(pid, SIGSEGV);

		sleep(1);
		shutdown_resource_process();
	} FINALLY {
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

