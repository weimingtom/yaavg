/* 
 * opengl3_funcs.h
 * by WN @ Nov. 30, 2009
 */
#ifndef __OPENGL3_H
#define __OPENGL3_H

#define GL3_PROTOTYPES
#include <video/gl3.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <GL/glext.h>

extern struct opengl3_functions {
#define DEF(x)	typeof(gl##x) * _gl3##x;
#define DEF2(f, n)
#include "opengl3_funcs_list.h"
#undef DEF
#undef DEF2
} _gl3_funcs;

#define gl3_name(x)	_gl3_funcs._gl3##x
#define gl3(x, ...)	({assert(gl3_name(x) != NULL); gl3_name(x)(__VA_ARGS__);})
extern struct func_entry __gl3_func_map[];

#endif

// vim:ts=4:sw=4

