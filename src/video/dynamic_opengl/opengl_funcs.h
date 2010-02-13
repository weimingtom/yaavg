/* 
 * opengl_funcs.h
 * by WN @ Nov. 30, 2009
 */
#ifndef __OPENGL_H
#define __OPENGL_H

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <GL/glext.h>

#include "dynamic_opengl.h"
extern struct opengl_functions {
#define DEF(x)	typeof(gl##x) * _gl##x;
#include "opengl_funcs_list.h"
#undef DEF
} _gl_funcs;

#define gl(x)	_gl_funcs._gl##x

extern struct func_entry __gl_func_map[];

#endif

// vim:ts=4:sw=4

