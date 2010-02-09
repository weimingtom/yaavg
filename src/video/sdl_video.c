/* 
 * opengl3_video.c
 * by WN @ Feb. 08, 2010
 */

#include <config.h>
#include <common/debug.h>
#include <common/exception.h>
#include <video/video.h>
#include <video/generic_sdl_video.h>

#if defined HAVE_SDL
#include <SDL/SDL.h>

static SDL_Surface * main_screen = NULL;
struct video_functionor_t sdl_video_functionor;

static bool_t
sdlv_check_usable(const char * param)
{
	VERBOSE(VIDEO, "%s:%s\n", __func__, param);
	if (strcasecmp(param, "SDL") == 0)
		return TRUE;
	VERBOSE(VIDEO, "doesn't support\n");
	return FALSE;
}

static void
sdlv_init(void)
{
	DEBUG(VIDEO, "initing sdl video\n");
	assert(CUR_VID == &sdl_video_functionor);

	generic_init_sdl_video(FALSE,
			CUR_VID->bpp, CUR_VID->grabinput);

	catch_var(SDL_Surface *, screen, NULL);
	define_exp(exp);
	TRY(exp) {
		int flags = 0;
		if (CUR_VID->fullscreen)
			flags |= SDL_FULLSCREEN;
		if (CUR_VID->resizable)
			flags |= SDL_RESIZABLE;
		set_catched_var(screen,
				SDL_SetVideoMode(
					CUR_VID->width,
					CUR_VID->height,
					CUR_VID->bpp,
					flags));
		if (screen == NULL) {
			FATAL(VIDEO, "SDL_SetVideoMode failed: %s\n",
					SDL_GetError());
			THROW_FATAL(EXP_UNCATCHABLE, "SDL_SetVideoMode failed");
		}
	} FINALLY { }
	CATCH(exp) {
		get_catched_var(screen);
		if (screen != NULL) {
			SDL_FreeSurface(screen);
			set_catched_var(screen, NULL);
		}
		generic_destroy_sdl_video();
	}

	main_screen = screen;
}

static void
sdlv_cleanup(void)
{
	DEBUG(VIDEO, "closing sdl video\n");
	if (main_screen == NULL) {
		FATAL(VIDEO, "call sdlv_cleanup before sdlv_init\n");
		THROW_FATAL(VIDEO, "$@!#^");
	}

	SDL_FreeSurface(main_screen);
	main_screen = NULL;
	generic_destroy_sdl_video();
}

struct video_functionor_t sdl_video_functionor = {
	.name = "SDLVideo",
	.fclass = FC_VIDEO,
	.check_usable = sdlv_check_usable,
	.init = sdlv_init,
	.cleanup = sdlv_cleanup,
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

