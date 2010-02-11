/* 
 * opengl3_video.c
 * by WN @ Feb. 08, 2010
 */

#include <config.h>
#include <common/debug.h>
#include <common/exception.h>
#include <video/video.h>

#if defined HAVE_OPENGL

static bool_t
gl_check_usable(const char * param)
{
	VERBOSE(VIDEO, "%s:%s\n", __func__, param);
	/* different from other's */
	if (strcasecmp(param, "SDL") != 0)
		return FALSE;

	/* find the opengl driver */
}

struct video_functionor_t opengl_video_functionor = {
	.name = "OpenGLVideo",
	.fclass = FC_VIDEO,
	.check_usable = gl_check_usable,
};
#else
struct video_functionor_t opengl_video_functionor = {
	.name = "OpenGLVideo",
	.fclass = FC_VIDEO,
};
#endif

// vim:ts=4:sw=4

