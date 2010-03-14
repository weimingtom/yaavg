/* 
 * glx_gl_driver.c
 * by WN @ Mar. 14, 2010
 */

#include <config.h>
#include <common/defs.h>
#include <common/debug.h>
#include <common/exception.h>
#include <yconf/yconf.h>
#include <video/gl_driver.h>

#include <video/video.h>
#include <events/sdl_events.h>

#include <dlfcn.h>

#ifdef HAVE_X11
extern struct video_functionor_t opengl_video_functionor;

static void (*pglXGetProcAddress)(const char * name) = NULL;

/* never unload any so file */
static void
load_opengl_library(void)
{
	const char * path = conf_get_string("video.opengl.gllibrary", NULL);
	if (path == NULL)
		path = "/usr/lib/libGL.so";
	void *handle = dlopen(path, RTLD_NOW);
	const char *loaderror = (char *)dlerror();
	if (handle == NULL)
		THROW_FATAL(EXP_LOAD_SHARED_OBJECT,
				"Failed loading %s: %s", path, loaderror);
	pglXGetProcAddress = dlsym(handle, "glXGetProcAddress");
	loaderror = (char *)dlerror();
	if (pglXGetProcAddress == NULL)
		THROW_FATAL(EXP_LOAD_SHARED_OBJECT,
				"Failed loading glXGetProcAddress: %s", loaderror);
	VERBOSE(GLX, "library %s has been loaded\n", path);
}

static bool_t
glx_check_usable(const char * param)
{
	VERBOSE(OPENGL, "%s:%s\n", __func__, param);
	if (strcasecmp(param, "GLX3") == 0) {
		VERBOSE(VIDEO, "GLX doesn't support opengl 3\n");
		return FALSE;
	}
	if (strcasecmp(param, "GLX") == 0)
		return TRUE;
	return FALSE;
}

static void
glx_init(void)
{
	assert(CUR_VID == &opengl_video_functionor);
	DEBUG(GLX, "glx opengl driver init\n");
	load_opengl_library();
}

struct gl_driver_functionor_t glx_gl_driver_functionor = {
	.name = "GLXOpenGLDriver",
	.fclass = FC_OPENGL_DRIVER,
	.check_usable = glx_check_usable,
	.init = glx_init,
};

#else
static bool_t
glx_check_usable(const char * param)
{
	return FALSE;
}

struct gl_driver_functionor_t glx_gl_driver_functionor = {
	.name = "GLXOpenGLDriver",
	.fclass = FC_OPENGL_DRIVER,
	.check_usable = glx_check_usable,
};

#endif

// vim:ts=4:sw=4

