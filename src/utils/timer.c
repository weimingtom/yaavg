/* 
 * timer.c
 * by WN @ Dec. 05, 2009
 */

#include <config.h>
#include <common/debug.h>
#include <common/exception.h>
#include <utils/timer.h>
#include <yconf/yconf.h>
#include <assert.h>

extern struct functionor_t base_timer_functionor;
#ifdef HAVE_SDL
extern struct functionor_t sdl_timer_functionor;
#endif

static struct functionor_t * functionors[] = {
#ifdef HAVE_SDL
	&sdl_timer_functionor,
#endif
	&base_timer_functionor,
	NULL,
};

struct function_class_t timer_function_class = {
	.fclass = FC_TIMER,
	.current_functionor = NULL,
	.functionors = &functionors,
};

void
__timer_init(void)
{
	find_functionor(&timer_function_class, NULL);
}

static struct timer_functionor_t * frame_timer = NULL;

struct time_controller {
	tick_t last_frame_tick;
	tick_t mspf;
	tick_t mspf_fallback;
	float fps;
	float real_fps;

	/* fps counter */
	int frames;
	tick_t last_fps_counter;
};

static struct time_controller controller = {
	.last_frame_tick = 0,
	.mspf = 17,
	.mspf_fallback = 100,
	.fps = 60.0,
	.real_fps = 60.0,
	.frames = 0,
};

void
begin_frame_loop(void)
{
	SET_STATIC_FUNCTIONOR(frame_timer, timer_function_class, NULL);
	assert(frame_timer != NULL);
	
	controller.last_frame_tick = frame_timer->get_ticks();
	controller.fps = conf_get_int("timer.fps", 60);
	if ((controller.fps <= 0) || (controller.fps > 1000)) {
		WARNING(TIMER, "user select ultimate fps, cpu overload will become 100%%!.\n");
		controller.fps = 2000;
	}
	controller.real_fps = controller.fps;
	controller.mspf = 1000 / controller.fps;
	controller.mspf_fallback = conf_get_int("timer.mspf.fallback", 100);

	controller.frames = 0;
	controller.last_fps_counter = controller.last_frame_tick;
	if (controller.mspf > controller.mspf_fallback)
		THROW_VAL(EXP_CORRUPTED_CONF,
				"timer.mspf_fallback",
				"timer.mspf_fallback(%d) should larger than 1000 / timer.fps(%d)\n",
				controller.mspf_fallback, controller.mspf);
}

dtick_t
enter_frame(void)
{
	tick_t current = frame_timer->get_ticks();
	dtick_t delta = current - controller.last_frame_tick;
	if (delta < 0) {
		WARNING(TIMER, "delta is %d, timer may rolled back\n",
				delta);
		delta = controller.mspf;
	}


	/* adjust fps */
	/* compute last frame's fps */
	float last_fps;
	if (delta != 0)
		last_fps = 1.0 / (float)delta * 1000;
	else
		last_fps = 1000.0;
	controller.real_fps = controller.real_fps * 0.2 +
		last_fps * 0.8;

	float fps_diff = controller.real_fps - controller.fps;
	if ((-0.0001 < fps_diff) && (fps_diff < 0.0001))
		fps_diff = 0;
	/* faster than desired fps */
	if (fps_diff > 0)
		controller.mspf ++;
	/* slower */
	if (fps_diff < 0) {
		if (controller.mspf > 1)
			controller.mspf --;
	}

	/* fps debug */
	controller.frames ++;
	dtick_t fps_ticks = current - controller.last_fps_counter;
	if (fps_ticks >= 1000) {
		DEBUG(TIMER, "%d frames in %d ms, fps=%f\n",
				controller.frames,
				fps_ticks,
				(float)(controller.frames)/(float)(fps_ticks) * 1000);
		controller.frames = 0;
		controller.last_fps_counter = current;
	}

	/* fall back */
	if ((tick_t)delta > controller.mspf_fallback) {
		WARNING(TIMER, "delta is %d, fall back to %d\n",
				delta, controller.mspf_fallback);
		delta = controller.mspf_fallback;
	}

	controller.last_frame_tick = current;
	return delta;
}

void
finish_frame(void)
{
	/* count fps */
	/* untimate fps, don't delay */
	if (controller.fps > 1000)
		return;
	tick_t current = frame_timer->get_ticks();
	dtick_t render_ms = current - controller.last_frame_tick;
	TRACE(TIMER, "render 1 frame use %d ms\n", render_ms);
	TRACE(TIMER, "real_fps is %f\n", controller.real_fps);
	if (render_ms < 1)
		render_ms = 1;

	if ((tick_t)render_ms < controller.mspf) {
		TRACE(TIMER, "delay %d ms\n", controller.mspf - render_ms);
		frame_timer->delay(controller.mspf - render_ms);
	}
}

tick_t
get_current(void)
{
	return controller.last_frame_tick;
}

void
force_delay(tick_t ms)
{
	if (frame_timer == NULL)
		SET_STATIC_FUNCTIONOR(frame_timer, timer_function_class, NULL);
	assert(frame_timer != NULL);
	frame_timer->delay(ms);
}

// vim:ts=4:sw=4

