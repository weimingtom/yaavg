/* 
 * opengl3_video.c
 * by WN @ Feb. 08, 2010
 */

#include <config.h>
#include <common/debug.h>
#include <common/exception.h>
#include <yconf/yconf.h>
#include <video/video.h>
#include <video/gl_driver.h>

#if defined HAVE_OPENGL

static bool_t
gl_check_usable(const char * param)
{
	VERBOSE(VIDEO, "%s:%s\n", __func__, param);
	if (strcasecmp(param, "opengl") == 0)
		return TRUE;
	return FALSE;
}

static void
gl_init(void)
{
	DEBUG(VIDEO, "init opengl video\n");

	/* find the opengl driver */
	const char * drv_str = conf_get_string("video.opengl.driver", "sdl");
	find_functionor(&gl_driver_function_class, drv_str);
	assert(CUR_DRV != NULL);
	CUR_DRV->major_version = 0;
	drv_init();

	CUR_VID->poll_events = CUR_DRV->poll_events;
	return;
}

static void
gl_cleanup(void)
{
	DEBUG(VIDEO, "closing opengl video\n");
	drv_cleanup();
	return;
}

struct video_functionor_t opengl_video_functionor = {
	.name = "OpenGLVideo",
	.fclass = FC_VIDEO,
	.check_usable = gl_check_usable,
	.init = gl_init,
	/* redirect to driver's poll_events */
	.poll_events = NULL,
	.cleanup = gl_cleanup,
};

#else

static bool_t
gl_check_usable(const char * param)
{
	VERBOSE(VIDEO, "%s:%s\n", __func__, param);
	return FALSE;
}

struct video_functionor_t opengl_video_functionor = {
	.name = "OpenGLVideo",
	.fclass = FC_VIDEO,
	.check_usable = gl_check_usable,
};
#endif

// vim:ts=4:sw=4

