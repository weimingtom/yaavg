/* 
 * This file is used for control the debugger output.
 * It should be included in debug.h, and never used directly.
 */

#ifndef __DEBUG_H_INCLUDE_DEBUG_COMPONENTS_H
# error Never include <common/debug_components.h> directly, use <common/debug.h> instead.
#endif

#ifndef __DEBUG_COMPONENTS_H
#define __DEBUG_COMPONENTS_H
enum __debug_component {
	SYSTEM	= 0,
	MEMORY,
	DICT,
	YCONF,
	VIDEO,
	TIMER,
	NR_DEBUG_COMPONENTS,
};

#ifdef __DEBUG_C
#ifdef YAAVG_DEBUG
/* name of component */
static const char *
__debug_component_names[NR_DEBUG_COMPONENTS] = {
	[SYSTEM]	= "SYS",
	[MEMORY]	= "MEM",
	[DICT]		= "DIC",
	[YCONF]		= "COF",
	[VIDEO]		= "VID",
	[TIMER]		= "TMR"
};

static const enum __debug_level
__debug_component_levels[NR_DEBUG_COMPONENTS] = {
	[SYSTEM]	= DBG_LV_VERBOSE,
	[MEMORY]	= DBG_LV_VERBOSE,
	[DICT]		= DBG_LV_VERBOSE,
	[YCONF]		= DBG_LV_VERBOSE,
	[VIDEO]		= DBG_LV_TRACE,
	[TIMER]		= DBG_LV_DEBUG,
};
#endif
#endif

#endif
