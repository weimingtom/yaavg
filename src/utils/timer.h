/* 
 * timer.h
 * by WN @ Dec. 05, 2009
 */

#ifndef __TIMER_H
#define __TIMER_H

#include <config.h>
#include <common/functionor.h>
#include <stdint.h>

/* tick_t is the absolute time, dtick_t is the difference between 2 tick_t */
typedef uint32_t tick_t;
typedef int32_t dtick_t;

extern struct function_class_t
timer_function_class;

struct timer_functionor_t {
	BASE_FUNCTIONOR
	void (*delay)(tick_t ms);
	tick_t (*get_ticks)(void);
};

extern void
begin_frame_loop(void);

extern dtick_t
enter_frame(void);

extern void
finish_frame(void);

#endif
// vim:ts=4:sw=4

