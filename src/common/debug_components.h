/* 
 * This file is used for control the debugger output.
 * It should be included in debug.h, and never used directly.
 */

#ifndef __DEBUG_INCLUDE_DEBUG_COMPONENTS_H
# error Never include <common/debug_components.h> directly, use <common/debug.h> instead.
#endif

def_dbg_component(SYSTEM, 	"SYS", VERBOSE)
def_dbg_component(MEMORY, 	"MEM", VERBOSE)
def_dbg_component(FILE_STREAM, 	"FIL", VERBOSE)
def_dbg_component(DICT, 	"DIC", VERBOSE)
def_dbg_component(YCONF, 	"COF", VERBOSE)
def_dbg_component(VIDEO, 	"VID", DEBUG)
def_dbg_component(OPENGL, 	"GL ", TRACE)
def_dbg_component(GLX, 		"GLX", TRACE)
def_dbg_component(EVENT,	"EVT", TRACE)
def_dbg_component(TIMER,	"TMR", DEBUG)
def_dbg_component(CACHE,	"CHE", VERBOSE)
def_dbg_component(BITMAP,	"BTM", DEBUG)
def_dbg_component(IO,		"I/O", VERBOSE)
def_dbg_component(RESOURCE,	"RES", VERBOSE)
def_dbg_component(OTHER,	"OTR", VERBOSE)


#ifndef DEBUG_OUTPUT_FILE
/* NULL means stderr */
# define DEBUG_OUTPUT_FILE	(NULL)
#endif

