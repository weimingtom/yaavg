/* 
 * opengl_funcs.c
 * by WN @ Feb. 11, 2010
 */

#include "dynamic_opengl.h"
#include "opengl3_funcs.h"

struct opengl3_functions _gl3_funcs;

struct func_entry __gl3_func_map[] = {
#define DEF(x) {.name = "gl" #x, .ptr = (void**)&(gl3_name(x))},
#define DEF2(f, n) {.name = "gl" #n, .ptr = (void**)&(gl3_name(f))},
#include  "opengl3_funcs_list.h"
#undef DEF
#undef DEF2
	{.name = NULL, .ptr = NULL},
};

// vim:ts=4:sw=4

