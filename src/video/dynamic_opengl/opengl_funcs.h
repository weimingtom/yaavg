/* 
 * opengl_funcs.h
 * by WN @ Nov. 30, 2009
 */
#ifndef __OPENGL_H
#define __OPENGL_H


#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <assert.h>

#include "dynamic_opengl.h"
extern struct opengl_functions {
#define DEF(x)	typeof(gl##x) * _gl##x;
/* DEF2 defines ARB or EXT functions after the 
 * spec version is defined. don't add anything to the
 * function structure. */
#define DEF2(f, n)
#include "opengl_funcs_list.h"
#undef DEF
#undef DEF2
} _gl_funcs;

#define gl_name(x)	_gl_funcs._gl##x
#define gl(x, ...)	({assert(gl_name(x) != NULL); gl_name(x)(__VA_ARGS__);})

extern struct func_entry __gl_func_map[];

#endif

// vim:ts=4:sw=4

