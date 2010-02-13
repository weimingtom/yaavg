/* 
 * opengl3_video.c
 * by WN @ Feb. 08, 2010
 */

#include <config.h>
#include <common/debug.h>
#include <common/exception.h>
#include <common/cache.h>
#include <common/mm.h>
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


struct sdl_bitmap_t {
	struct cache_entry_t ce;
	/* crossponding bitmap */
	/* one bitmap matches */
	struct bitmap_t * b;
	SDL_Surface * s;
};

static struct cache_t sdl_bitmap_cache;

static void
destroy_sdl_bitmap(struct sdl_bitmap_t * sb)
{
	assert(sb != NULL);
	TRACE(VIDEO, "destroy sdl_bitmap %s\n",
			sb->ce.id);
	SDL_Surface * s = sb->s;
	struct bitmap_t * b = sb->b;
	assert(b->ref_count > 0);
	SDL_FreeSurface(s);
	b->ref_count --;
	if (b->ref_count == 0)
		free_bitmap(b);
	xfree(sb);
}

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
	assert(CUR_VID == &sdl_video_functionor);
	main_screen = generic_init_sdl_video(FALSE);
	assert(main_screen != NULL);

	/* init the bitmap cache */
	cache_init(&sdl_bitmap_cache, "sdl bitmap cache",
			conf_get_int("video.sdl.bitmapcachesz", 0xa00000));
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

static SDL_Surface *
get_surface(const char * name)
{

	struct cache_entry_t * ce = cache_get_entry(
			&sdl_bitmap_cache, name);
	if (ce != NULL)
		return ((struct sdl_bitmap_t * )(ce->data))->s;

	struct bitmap_t * b = get_resource(name,
			(deserializer_t)bitmap_deserialize, NULL);
	assert(b != NULL);
	assert(b->pixels != NULL);

	/* create the surface from pixels */
	uint32_t rmask, gmask, bmask, amask;
	SET_COLOR_MASKS(b->format, rmask, gmask, bmask, amask);
	SDL_Surface * s = SDL_CreateRGBSurfaceFrom(
			b->pixels,
			b->w,
			b->h,
			b->bpp * 8,
			b->pitch,
			rmask,
			gmask,
			bmask,
			amask);
	assert(s != NULL);
	TRACE(VIDEO, "%s is a %dx%dx%d bitmap, pitch=%d\n",
			name, b->w, b->h, b->bpp, b->pitch);
	/* create the sdl_bitmap */
	struct sdl_bitmap_t * sb = xmalloc(sizeof(*sb));
	assert(sb != NULL);
	sb->b = b;
	sb->s = s;

	b->ref_count ++;

	/* build the cache entry */
	ce = &sb->ce;
	ce->id = b->id;
	ce->data = sb;
	ce->sz = sizeof(*sb) + sizeof(*s);
	/* estimate */
	if (b->ref_count == 1)
		ce->sz += sizeof(*b) + b->id_sz +
			b->pitch * b->h + 15;
	ce->destroy_arg = sb;
	ce->destroy = (cache_destroy_t)(destroy_sdl_bitmap);
	ce->cache = NULL;
	ce->pprivate = NULL;

	/* insert */
	cache_insert(&sdl_bitmap_cache, ce);
	return s;
}

static void
sdlv_test_screen(const char * bitmap_name)
{
	assert(bitmap_name != NULL);
	TRACE(VIDEO, "test show bitmap %s\n", bitmap_name);
	SDL_Surface * bitmap = get_surface(bitmap_name);
	assert(bitmap != NULL);
	SDL_BlitSurface(bitmap, NULL, main_screen, NULL);
	return;
}

static void
sdlv_swapbuffer(void)
{
	SDL_UpdateRect(main_screen, 0, 0, 0, 0);
}

struct video_functionor_t sdl_video_functionor = {
	.name = "SDLVideo",
	.fclass = FC_VIDEO,
	.check_usable = sdlv_check_usable,
	.init = sdlv_init,
	.cleanup = sdlv_cleanup,
	.swapbuffer = sdlv_swapbuffer,
	.poll_events = generic_sdl_poll_events,
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

