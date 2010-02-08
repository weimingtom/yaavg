/* 
 * opengl3_video.c
 * by WN @ Feb. 08, 2010
 */

#include <config.h>
#include <common/debug.h>
#include <common/exception.h>
#include <video/video.h>

#if defined HAVE_SDL
#include <SDL/SDL.h>

static bool_t
sdlv_check_usable(const char * param)
{
	VERBOSE(VIDEO, "%s:%s\n", __func__, param);
	if (strcasecmp(param, "SDL") == 0)
		return TRUE;
	VERBOSE(VIDEO, "doesn't support\n");
	return FALSE;
}


struct video_functionor_t sdl_video_functionor = {
	.name = "SDLVideo",
	.fclass = FC_VIDEO,
	.check_usable = sdlv_check_usable,
};

#else

static bool_t
sdlv_check_usable(const char * param)
{
	VERBOSE(VIDEO, "%s:%s\n", __func__, param);
	return FALSE;
}

struct video_functionor_t sdl_video_functionor = {
	.name = "SDLVideo",
	.fclass = FC_VIDEO,
	.check_usable = sdlv_check_usable,
};
#endif

// vim:ts=4:sw=4

