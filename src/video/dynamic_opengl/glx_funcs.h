/* 
 * opengl_funcs.h
 * by WN @ Nov. 30, 2009
 */
#ifndef __OPENGL_H
#define __OPENGL_H

#include <GL/glx.h>
#include <GL/glxext.h>
#include <assert.h>

extern struct glx_functions {
#define DEF(x)	typeof(glX##x) * _glX##x;
#include "glx_funcs_list.h"
#undef DEF
} _glx_funcs;

#define glX_name(x)	_glx_funcs._glX##x
#define glX(x, ...) ({assert(glX_name(x) != NULL); glX_name(x)(__VA_ARGS__);})
extern struct func_entry __glx_func_map[];

#endif

// vim:ts=4:sw=4

