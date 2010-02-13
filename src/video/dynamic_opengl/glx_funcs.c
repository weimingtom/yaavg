/* 
 * opengl_funcs.c
 * by WN @ Feb. 11, 2010
 */

#include "dynamic_opengl.h"
#include "glx_funcs.h"

struct glx_functions _glx_funcs;

struct func_entry __glx_func_map[] = {
#define DEF(x) {.name = "glX" #x, .ptr = (void**)&(glX(x))},
#include  "glx_funcs_list.h"
#undef DEF
	{.name = NULL, .ptr = NULL},
};

// vim:ts=4:sw=4

