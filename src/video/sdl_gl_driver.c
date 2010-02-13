/* 
 * sdl_gl_driver.c
 * by WN @ Feb. 13, 2010
 */

#include <config.h>
#include <common/defs.h>
#include <common/debug.h>
#include <common/exception.h>
#include <video/gl_driver.h>

#include <video/video.h>
#include <video/generic_sdl_video.h>
#include <utils/generic_sdl.h>
#include <events/sdl_events.h>

#ifdef HAVE_SDL
extern struct video_functionor_t opengl_video_functionor;

static SDL_Surface * main_screen = NULL;

static bool_t
sdlgl_check_usable(const char * param)
{
	VERBOSE(VIDEO, "%s:%s\n", __func__, param);
	if (strcasecmp(param, "SDL3") == 0) {
		VERBOSE(VIDEO, "sdl doesn't support opengl 3\n");
		return FALSE;
	}
	if (strcasecmp(param, "SDL") == 0)
		return TRUE;
	return FALSE;
}

static void
sdlgl_init(void)
{
	assert(CUR_VID == &opengl_video_functionor);
	main_screen = generic_init_sdl_video(TRUE);
	assert(main_screen != NULL);
	return;
}

static void
sdlgl_cleanup(void)
{
	DEBUG(VIDEO, "closing sdl-opengl driver\n");
	if (main_screen == NULL) {
		FATAL(VIDEO, "call sdlgl_cleanup before sdlgl_init\n");
		THROW_FATAL(VIDEO, "$@!#^");
	}

	SDL_FreeSurface(main_screen);
	main_screen = NULL;
	generic_destroy_sdl_video();
}

struct gl_driver_functionor_t sdl_gl_driver_functionor = {
	.name = "SDLOpenGLDriver",
	.fclass = FC_OPENGL_DRIVER,
	.check_usable = sdlgl_check_usable,
	.init = sdlgl_init,
	.poll_events = generic_sdl_poll_events,
	.cleanup = sdlgl_cleanup,
};

#else

static bool_t
sdlgl_check_usable(const char * param)
{
	return FALSE;
}

struct gl_driver_functionor_t sdl_gl_driver_functionor = {
	.name = "SDLOpenGLDriver",
	.fclass = FC_OPENGL_DRIVER,
	.check_usable = sdlgl_check_usable,
};

#endif
// vim:ts=4:sw=4

