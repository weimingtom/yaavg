/* 
 * generic_opengl.h
 * by WN @ Feb. 13, 2010
 */

#ifndef __GENERIC_OPENGL_H
#define __GENERIC_OPENGL_H
#include <config.h>
#include <common/dict.h>
#include <GL/gl.h>
#include <GL/glext.h>

extern GLenum
gl_pop_error_debug(const char * file, const char * func, int line);

extern GLenum
gl_pop_error_nodebug(void);

#ifdef YAAVG_DEBUG
# define GL_POP_ERROR() gl_pop_error_debug(__FILE__, __FUNCTION__, __LINE__)
#else
# define GL_POP_ERROR() gl_pop_error_nodebug()
#endif

/* strdict, defined in opengl_video.c */
extern struct dict_t * GL_extensions_dict;
extern const char * GL_vendor;
extern const char * GL_renderer;
extern const char * GL_version;
extern const char * GL_glsl_version;
extern struct dict_t * GL_extensions_dict;

#define MKVER(ma, mi)		(((ma)*1000) + ((mi)))
extern int GL_major_version;
extern int GL_minor_version;
extern int GL_full_version;

extern int GLSL_major_version;
extern int GLSL_minor_version;
extern int GLSL_full_version;

extern int GL_max_texture_size;
extern bool_t GL_texture_NPOT;
extern bool_t GL_texture_RECT;
extern bool_t GL_texture_COMPRESSION;

#endif

// vim:ts=4:sw=4

