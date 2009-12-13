#include <config.h>
#include <common/debug.h>
#include <common/exception.h>
#include <common/dict.h>
#include <common/init_cleanup_list.h>
#include <common/functionor.h>
#include <common/mm.h>
#include <utils/timer.h>
#include <video/video.h>

#include <assert.h>
#include <signal.h>

struct mem_cache_t * static_mem_caches[] = {
	&__dict_t_cache,
	NULL,
};

init_func_t init_funcs[] = {
	__dbg_init,
	__yconf_init,
	__timer_init,
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
	struct video_functionor_t * vid = NULL;
	struct timer_functionor_t * timer = NULL;
	do_init();

	struct exception_t exp;
	TRY(exp) {
		SET_STATIC_FUNCTIONOR(vid, video_function_class, NULL);
		assert(vid != NULL);
		VERBOSE(SYSTEM, "found video engine: %s\n", vid->name);

		SET_STATIC_FUNCTIONOR(timer, timer_function_class, NULL);
		assert(timer != NULL);
		VERBOSE(SYSTEM, "found timer engine: %s\n", timer->name);

		dtick_t delta_time = 0;
		begin_frame_loop();
		for (;;) {
			delta_time = enter_frame();
			finish_frame();
		}
	} FINALLY {
		if ((vid) && (vid->destroy))
			vid->destroy();
		vid = NULL;
	}
	CATCH(exp) {
		switch(exp.type) {
			case EXP_FUNCTIONOR_NOT_FOUND:
				ERROR(SYSTEM, "unable to find suitable video functionor\n");
				break;
			case EXP_CORRUPTED_CONF:
				ERROR(SYSTEM, "configuraion \"%s\" currupted: %s\n",
						(const char *)exp.u.ptr, exp.msg);
				break;
			default:
				RETHROW(exp);
		}
	}

	do_cleanup();
	raise(SIGUSR1);
	return 0;
}

// vim:ts=4:sw=4

