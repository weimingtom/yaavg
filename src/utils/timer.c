/* 
 * timer.c
 * by WN @ Dec. 05, 2009
 */

#include <config.h>
#include <utils/timer.h>

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


// vim:ts=4:sw=4

