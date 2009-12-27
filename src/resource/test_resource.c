#include <common/init_cleanup_list.h>
#include <resource/resource.h>

#include <common/debug.h>
#include <common/exception.h>

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
	do_init();
	launch_resource_process();
	do_cleanup();
	return 0;
}

// vim:ts=4:sw=4

