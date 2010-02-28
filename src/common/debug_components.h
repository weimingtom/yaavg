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
	FILE_STREAM,
	DICT,
	YCONF,
	VIDEO,
	OPENGL,
	EVENT,
	TIMER,
	CACHE,
	BITMAP,
	IO,
	RESOURCE,
	OTHER,
	NR_DEBUG_COMPONENTS,
};

#ifdef __DEBUG_C
#ifdef YAAVG_DEBUG
/* name of component */
static const char *
__debug_component_names[NR_DEBUG_COMPONENTS] = {
	[SYSTEM]	= "SYS",
	[MEMORY]	= "MEM",
	[FILE_STREAM]	= "FIL",
	[DICT]		= "DIC",
	[YCONF]		= "COF",
	[VIDEO]		= "VID",
	[OPENGL]	= "GL ",
	[EVENT]		= "EVT",
	[TIMER]		= "TMR",
	[CACHE]		= "CHE",
	[BITMAP]	= "BTM",
	[IO]		= "I/O",
	[RESOURCE]	= "RES",
	[OTHER]		= "OTH",
};

static const enum __debug_level
__debug_component_levels[NR_DEBUG_COMPONENTS] = {
	[SYSTEM]	= DBG_LV_TRACE,
	[MEMORY]	= DBG_LV_VERBOSE,
	[FILE_STREAM]	= DBG_LV_VERBOSE,
	[DICT]		= DBG_LV_VERBOSE,
	[YCONF]		= DBG_LV_VERBOSE,
	[VIDEO]		= DBG_LV_DEBUG,
	[OPENGL]	= DBG_LV_TRACE,
	[EVENT]		= DBG_LV_TRACE,
	[TIMER]		= DBG_LV_DEBUG,
	[CACHE]		= DBG_LV_VERBOSE,
	[BITMAP]	= DBG_LV_VERBOSE,
	[IO]		= DBG_LV_VERBOSE,
	[RESOURCE]	= DBG_LV_VERBOSE,
	[OTHER]		= DBG_LV_VERBOSE,
};
#endif
#endif

#endif

