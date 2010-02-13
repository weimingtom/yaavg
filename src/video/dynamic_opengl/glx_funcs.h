/* 
 * opengl_funcs.h
 * by WN @ Nov. 30, 2009
 */
#ifndef __OPENGL_H
#define __OPENGL_H

#include <GL/glx.h>
#include <GL/glxext.h>

extern struct glx_functions {
#define DEF(x)	typeof(glX##x) * _glX##x;
#include "glx_funcs_list.h"
#undef DEF
} _glx_funcs;

#define glX(x)	_glx_funcs._glX##x
extern struct func_entry __glx_func_map[];

#endif

// vim:ts=4:sw=4

