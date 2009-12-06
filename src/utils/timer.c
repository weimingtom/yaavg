/* 
 * timer.c
 * by WN @ Dec. 05, 2009
 */

#include <config.h>
#include <utils/timer.h>
#include <assert.h>

extern struct functionor_t base_timer_functionor;
extern struct functionor_t sdl_timer_functionor;

static struct functionor_t * functionors[] = {
	&sdl_timer_functionor,
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

static struct timer_functionor * frame_timer = NULL;

struct time_controller {
	tick_t last_time;
	tick_t mspf;
	tick_t mspf_fallback;
	float fps;
	float real_fps;
	int frames;
};

static struct time_controller controller = {
	.last_time = 0,
	.mspf = 17,		/* 60 fps */
	.mspf_fallback = 100;
};

void
begin_frame_loop(void)
{
	SET_STATIC_FUNCTIONOR(frame_timer, timer_function_class, NULL);
	assert(frame_timer != NULL);
	controller.last_time = frame_timer->get_ticks();
	controller.fps = yconf_get_int("timer.fps", 60);
	controller.real_fps = controller.fps;
	controller.mspf_fallback = yconf_get_int("timer.mspf.fallback", 100);
	controller.mspf = 1000 / controller.fps;
	if (controller.mspf <= controller.mspf_fallback)
		THROW(EXP_CORRUPTED_CONF, "timer.mspf_fallback(%d) should larger than 1000 / timer.fps(%d)\n",
				controller.mspf_fallback, controller.mspf);
}

dtick_t
enter_frame(void)
{
	tick_t current = frame_timer->get_ticks();
	dtick_t delta = current - controller.last_time;
	if (delta < 0) {
		WARNING(TIMER, "delta is %d, timer may roll back\n",
				delta);
		delta = controller.mspf;
	}

	if (delta > controller.mspf_fallback) {
		WARNING(TIMER, "delta is %d, fall back to %d\n",
				delta, controller.mspf_fallback);
		delta = controller.mspf_fallback;
	}

	controller->last_time = current;
	return delta;
}

void
finish_frame(void)
{
	/* count fps */
	tick_t current = frame_timer->get_ticks();
	dtick_t render_ms = current - controller.last_time;
	TRACE(TIMER, "render 1 frame use %d ms\n", render_ms);
	if (render_ms <= 0)
		render_ms = 1;

	if (render_ms < controller.mspf)
		frame_timer->delay(controller.mspf - render_ms);
}

// vim:ts=4:sw=4

