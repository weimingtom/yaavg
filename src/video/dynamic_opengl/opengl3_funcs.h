/* 
 * opengl3_funcs.h
 * by WN @ Nov. 30, 2009
 */
#ifndef __OPENGL3_H
#define __OPENGL3_H

#include <video/gl3.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <GL/glext.h>

extern struct opengl3_functions {
#define DEF(x)	typeof(gl##x) * _gl##x;
#include "opengl3_funcs_list.h"
#undef DEF
} _gl3_funcs;

#define gl3(x)	_gl3_funcs._gl##x

extern struct func_entry __gl3_func_map[];

#endif

// vim:ts=4:sw=4

