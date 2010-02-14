/* 
 * generic_opengl.h
 * by WN @ Feb. 13, 2010
 */

#include <config.h>
#include <common/defs.h>
#include <common/debug.h>
#include <common/exception.h>

/* the opengl functions have to be set nomatter
 * we are using opengl or opengl3 */
#include <video/dynamic_opengl/opengl_funcs.h>
#include <video/generic_opengl.h>

/* setted when gl or gl3 video initing */
const char * GL_vendor = NULL;
const char * GL_renderer = NULL;
const char * GL_version = NULL;
const char * GL_glsl_version = NULL;
struct dict_t * GL_extensions_dict = NULL;

int GL_major_version = 0;
int GL_minor_version = 0;
int GL_full_version = 0;

int GLSL_major_version = 0;
int GLSL_minor_version = 0;
int GLSL_full_version = 0;

int GL_max_texture_size = 0;
bool_t GL_texture_NPOT = FALSE;
bool_t GL_texture_RECT = FALSE;
bool_t GL_texture_COMPRESSION = FALSE;




static const char *
glerrno_to_desc(GLenum errno)
{
	struct kvp {
		GLenum errno;
		const char * desc;
	};

	static struct kvp tb[] = {
		{GL_INVALID_ENUM, "Enum argument out of range"},
		{GL_INVALID_VALUE, "Numeric argument out of range"},
		{GL_INVALID_OPERATION, "Operation illegal in current state"},
		{GL_INVALID_FRAMEBUFFER_OPERATION, "Framebuffer object is not complete"},
		{GL_STACK_OVERFLOW, "Command would cause a stack overflow"},
		{GL_STACK_UNDERFLOW, "Command would cause a stack underflow"},
		{GL_OUT_OF_MEMORY, "Not enough memory left to execute command"},
		{GL_TABLE_TOO_LARGE, "The specified table is too large"},
		/* 0 is resvred for GL_NO_ERROR at least in nvidia opengl */
		{0, "Unknown error"}
	};

	struct kvp * ptr = tb;
	while(ptr->errno != 0) {
		if (ptr->errno == errno)
			break;
		ptr ++;
	}
	return ptr->desc;
}


GLenum
gl_pop_error_debug(const char * file, const char * func, int line)
{
	GLenum err = gl(GetError);
	if (err == GL_NO_ERROR)
		return GL_NO_ERROR;
	WARNING(OPENGL, "OpenGL error: \"%s\" at %s:%s:%d\n",
			glerrno_to_desc(err), file, func, line);
	return err;
}

GLenum
gl_pop_error_nodebug(void)
{
	return gl(GetError);
}


// vim:ts=4:sw=4

