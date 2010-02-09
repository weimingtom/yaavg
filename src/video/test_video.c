/* 
 * test_video.c
 * by WN @ Jan. 30, 2010
 */

#include <config.h>
#include <common/debug.h>
#include <common/exception.h>
#include <common/init_cleanup_list.h>
#include <resource/resource_proc.h>
#include <resource/resource.h>

#include <utils/timer.h>
#include <video/video.h>

#include <assert.h>
#if 0
init_func_t init_funcs[] = {
	__dbg_init,
	__yconf_init,
	__timer_init,
	NULL,
};


cleanup_func_t cleanup_funcs[] = {
	__yconf_cleanup,
	__caches_cleanup,
	__dbg_cleanup,
	NULL,
};

static void
render(void)
{
	struct exception_t exp;
	TRY(exp) {
		dtick_t delta_time = 0;
		begin_frame_loop();
		bool_t is_exit = FALSE;
		while (!is_exit) {
			delta_time = enter_frame();
			video_begin_frame();
			video_render_frame();
			video_end_frame();
			finish_frame();
			video_swapbuffer();
			struct phy_event ent;
			while (video_poll_events(&ent)) {
				print_event(&ent);
				if (ent.u.type == PHY_QUIT) {
					VERBOSE(VIDEO, "system exit\n");
					is_exit = TRUE;
					break;
				}
			}
		}
	} FINALLY {}
	CATCH(exp) {
		RETHROW(exp);
	}
}

int
main()
{

	struct exception_t exp;
	TRY(exp) {
		do_init();
		launch_resource_process();
		prepare_video();
		video_init();
		/* begin frame loop */
		render();
	} FINALLY {
		video_cleanup();
		if ((exp.type != EXP_RESOURCE_PEER_SHUTDOWN)
				&& (exp.type != EXP_RESOURCE_PROCESS_FAILURE)) {
			shutdown_resource_process();
		}
	} CATCH(exp) {
		switch (exp.type) {
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

#endif

init_func_t init_funcs[] = {
	__dbg_init,
	__yconf_init,
	__timer_init,
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
	define_exp(exp);
	TRY(exp) {
		video_init();
	} FINALLY {
		video_cleanup();
	} CATCH(exp) {
		RETHROW(exp);
	}
	do_cleanup();
	return 0;
}
// vim:ts=4:sw=4

