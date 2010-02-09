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
	struct timer_functionor_t * timer = NULL;
	do_init();

	catch_var(struct video_functionor_t *, vid, NULL);
	define_exp(exp);
	TRY(exp) {
		SET_STATIC_FUNCTIONOR(vid, video_function_class, NULL);
		assert(vid != NULL);
		set_catched_var(vid, vid);
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
		get_catched_var(vid);
		if ((vid) && (vid->cleanup))
			vid->cleanup();
		set_catched_var(vid, NULL);
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
	return 0;
}

// vim:ts=4:sw=4

