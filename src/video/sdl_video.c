/* 
 * opengl3_video.c
 * by WN @ Feb. 08, 2010
 */

#include <config.h>
#include <common/debug.h>
#include <common/exception.h>
#include <common/cache.h>
#include <yconf/yconf.h>

#include <video/video.h>
#include <utils/generic_sdl.h>
#include <video/generic_sdl_video.h>
#include <events/phy_events.h>
#include <events/sdl_events.h>
#include <resource/resource_proc.h>
#include <bitmap/bitmap.h>

#if defined HAVE_SDL
#include <SDL/SDL.h>

/* needn't use volatile shadow because we set it after everything
 * is okay. */
static SDL_Surface * main_screen = NULL;
struct video_functionor_t sdl_video_functionor;


struct sdl_bitmap {
	struct cache_entry_t ce;
	SDL_Surface * surface;
};

static struct cache_t sdl_bitmap_cache;

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
	assert(main_screen == NULL);

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

		/* notice the '!' */
		/* unblock sigint should be done after the init of screen */
		if (!conf_get_bool("video.sdl.blocksigint", TRUE))
			generic_sdl_unblock_sigint();

		/* init the bitmap cache */
		cache_init(&sdl_bitmap_cache, "sdl bitmap cache",
				conf_get_int("video.sdl.bitmapcachesz", 0xa00000));

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

static int
sdlv_poll_events(struct phy_event_t * e)
{
	return generic_sdl_poll_events(e);
}

static SDL_Surface *
get_surface(const char * name)
{
	struct bitmap_t * b = get_resource(name,
			(deserializer_t)bitmap_deserialize, NULL);
	assert(b != NULL);

	/* create the surface from pixels */
	uint32_t rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	switch (b->format) {
		case BITMAP_RGB:
			rmask = 0xff;
			gmask = 0xff00;
			bmask = 0xff0000;
			amask = 0;
			break;
		case BITMAP_RGBA:
			rmask = 0xff;
			gmask = 0xff00;
			bmask = 0xff0000;
			amask = 0xff000000;
			break;
		case BITMAP_BGR:
			rmask = 0xff0000;
			gmask = 0xff00;
			bmask = 0xff;
			amask = 0;
			break;
		case BITMAP_BGRA:
			rmask = 0xff0000;
			gmask = 0xff00;
			bmask = 0xff;
			amask = 0xff000000;
			break;
	}
#else
	switch (b->format) {
		case BITMAP_RGB:
			rmask = 0xff0000;
			gmask = 0xff00;
			bmask = 0xff;
			amask = 0;
			break;
		case BITMAP_RGBA:
			rmask = 0xff;
			gmask = 0xff00;
			bmask = 0xff0000;
			amask = 0xff000000;
			break;
		case BITMAP_BGR:
			rmask = 0xff0000;
			gmask = 0xff00;
			bmask = 0xff;
			amask = 0;
			break;
		case BITMAP_BGRA:
			rmask = 0xff0000;
			gmask = 0xff00;
			bmask = 0xff;
			amask = 0xff000000;
			break;
	}
#endif

	free_bitmap(b);
}

void
sdlv_test_screen(const char * bitmap_name)
{
	assert(bitmap_name != NULL);
	TRACE(VIDEO, "test show bitmap %s\n", bitmap_name);
	SDL_Surface * bitmap = NULL;
	struct cache_entry_t * ce = cache_get_entry(
			&sdl_bitmap_cache, bitmap_name);
	if (ce == NULL) {
		
	} else {
		bitmap = ce->data;
	}
	return;
}

struct video_functionor_t sdl_video_functionor = {
	.name = "SDLVideo",
	.fclass = FC_VIDEO,
	.check_usable = sdlv_check_usable,
	.init = sdlv_init,
	.cleanup = sdlv_cleanup,
	.poll_events = sdlv_poll_events,
	.test_screen = sdlv_test_screen,
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

