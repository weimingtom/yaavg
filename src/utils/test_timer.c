#include <config.h>
#include <common/debug.h>
#include <common/exception.h>
#include <common/dict.h>
#include <common/init_cleanup_list.h>
#include <common/functionor.h>
#include <utils/timer.h>

#include <signal.h>

init_func_t init_funcs[] = {
	__dbg_init,
	__dict_init,
	__yconf_init,
	NULL,
};


cleanup_func_t cleanup_funcs[] = {
	__yconf_cleanup,
	__dict_cleanup,
	__dbg_cleanup,
	NULL,
};

int main()
{
	do_init();

	struct timer_functionor_t * timer = 
		(struct timer_functionor_t *)find_functionor(&timer_function_class, NULL);

	VERBOSE(SYSTEM, "timer %s report current ticket: %d\n",
			timer->name,
			timer->get_ticks());
	timer->delay(1000);
	VERBOSE(SYSTEM, "timer %s report current ticket: %d\n",
			timer->name,
			timer->get_ticks());

	do_cleanup();
	raise(SIGUSR1);
	return 0;
}

// vim:ts=4:sw=4

