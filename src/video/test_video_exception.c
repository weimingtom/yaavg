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



static void
render(void)
{
	define_exp(exp);
restart:
	TRY(exp) {
		dtick_t delta_time = 0;
		timer_begin_frame_loop();
		bool_t is_exit = FALSE;
		while (!is_exit) {
			delta_time = timer_enter_frame();

//			vid_test_screen("0*XP3:ss_保有背景真アサシン.tlg|FILE:/home/wn/Windows/Fate/image.xp3");

			timer_finish_frame();
			THROW_VIDEO_EXP(VIDEXP_RERENDER, timer_get_current(),
					"test exception");

			vid_swapbuffer();

			struct phy_event_t evt;
			while (vid_poll_events(&evt)) {
				print_event(&evt);
				if (evt.type == EVENT_PHY_QUIT)
					is_exit = TRUE;
				if (evt.type == EVENT_PHY_KEY_DOWN) {
					if (evt.u.key.scancode == 0x18)
						is_exit = TRUE;
					if (evt.u.key.scancode == 0x09)
						is_exit = TRUE;
					if (evt.u.key.scancode == 0x1c)
						vid_toggle_fullscreen();
				}
			}
		}
	} FINALLY { }
	CATCH(exp) {
		switch (exp.type) {
			case EXP_VIDEO_EXCEPTION:
				print_exception(&exp);
				switch (exp.u.video_exception) {
					case VIDEXP_NOEXP:
					case VIDEXP_RERENDER:
						WARNING(VIDEO, "try rerender\n");
						goto restart;
					case VIDEXP_SKIPFRAME:
						WARNING(VIDEO, "skip 2 frames and rerender\n");
						timer_enter_frame();
						timer_finish_frame();
						timer_enter_frame();
						timer_finish_frame();
						goto restart;
						break;
					default:
						RETHROW(exp);
				}
				break;
			default:	
				RETHROW(exp);
		}
	}
}

int main()
{
restart:
	do_init();
	do {} while(0);
	define_exp(exp);
	catch_var(bool_t, video_inited, FALSE);
	TRY(exp) {
		launch_resource_process();
		video_init();
		set_catched_var(video_inited, TRUE);
		render();
	} FINALLY {
		get_catched_var(video_inited);
		if (video_inited)
			video_cleanup();
		shutdown_resource_process();
	} CATCH(exp) {
		switch (exp.type) {
			case EXP_VIDEO_EXCEPTION:
				switch (exp.u.video_exception) {
					case VIDEXP_REINIT:
						WARNING(VIDEO, "reinit full system and rerender\n");
						goto restart;
						break;
					case VIDEXP_FATAL:
					default:
						ERROR(VIDEO, "render problem, unable to continue\n");
						RETHROW(exp);
				}
				break;
			default:	
				RETHROW(exp);
		}
	}

	do_cleanup();
	return 0;
}
// vim:ts=4:sw=4

