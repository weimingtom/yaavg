/* 
 * opengl_funcs.c
 * by WN @ Feb. 11, 2010
 */

#include "dynamic_opengl.h"
#include "opengl_funcs.h"

struct opengl_functions _gl_funcs;

struct func_entry __gl_func_map[] = {
#define DEF(x) {.name = "gl" #x, .ptr = (void**)&(gl(x))},
#include  "opengl_funcs_list.h"
#undef DEF
	{.name = NULL, .ptr = NULL},
};

// vim:ts=4:sw=4

