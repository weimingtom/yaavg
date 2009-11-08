/* 
 * debug.h
 * by WN @ Nov. 08, 2009
 */

#ifndef __DEBUG_H
#define __DEBUG_H
#include <common/defs.h>

__BEGIN_DECLS

enum __debug_level {
	DBG_LV_SILENT	= 0,
	DBG_LV_TRACE,
	DBG_LV_DEBUG,
	DBG_LV_VERBOSE,
	DBG_LV_WARNING,
	DBG_LV_ERROR,
	DBG_LV_FATAL,
	DBG_LV_FORCE,
	NR_DEBUG_LEVELS,
};

#define __DEBUG_H_INCLUDE_DEBUG_COMPONENTS_H
#include <common/debug_components.h>
#undef __DEBUG_H_INCLUDE_DEBUG_COMPONENTS_H


#define __NOP	do {}while(0)

extern void
dbg_init(const char * file);

extern void
dbg_exit(void);

/*
 * we should introduce new directive.
 * if '@' is the 1st char of fmt, the following char is a controller:
 * `q' for suppress prefix;
 *
 * after the controller, user can insert a ' ' to split directives and real
 * format.
 */
extern void
dbg_output(enum __debug_level level, enum __debug_component comp,
		const char * file, const char * func, int line,
		char * fmt, ...);

__END_DECLS
#endif

