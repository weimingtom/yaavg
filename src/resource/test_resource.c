#include <common/init_cleanup_list.h>
#include <resource/resource_proc.h>

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
	__caches_cleanup,
	__dbg_cleanup,
	NULL,
};

int main()
{
	/* test start and stop */
	do_init();
	launch_resource_process();
	shutdown_resource_process();
	do_cleanup();

	int pid;

	struct exception_t exp;
	TRY(exp) {
		do_init();
		pid = launch_resource_process();

		kill(pid, SIGSEGV);

		sleep(1);
		shutdown_resource_process();

		char * test_str = strdupa("0*PACL:aaa.dat|FILE:/tmp/aaa.dat");
		char * type, * proto, * name, * left;
		type = test_str;
		proto = split_resource_type(type);
		name = split_protocol(proto);
		left = split_name(name);

		VERBOSE(SYSTEM, "type=%s\n", type);
		VERBOSE(SYSTEM, "proto=%s\n", proto);
		VERBOSE(SYSTEM, "name=%s\n", name);
		VERBOSE(SYSTEM, "left=%s\n", left);
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

