/* 
 * sdl_png_bitmap.c
 * by WN @ Dec. 13, 2009
 */


#include <config.h>
#include <common/debug.h>
#include <common/exception.h>
#include <bitmap/bitmap.h>

#include <string.h>
#include <assert.h>

#ifdef HAVE_SDL
# ifdef HAVE_SDLIMAGE
#  define SDLIMAGE_ENABLE
# endif
#endif

#ifdef SDLIMAGE_ENABLE
# include <SDL/SDL_image.h>
#endif

#if 0

static bool_t
sdl_check_usable(const char * param)
{
	assert(param != NULL);
	assert(strnlen(param, 4) >= 3);
#ifdef SDLIMAGE_ENABLE
	if (strncmp(param, "BMP", 3) == 0)
		return TRUE;
	if (strncmp(param, "GIF", 3) == 0)
		return TRUE;
	if (strncmp(param, "JPG", 3) == 0)
		return TRUE;
	if (strncmp(param, "LBM", 3) == 0)
		return TRUE;
	if (strncmp(param, "PCX", 3) == 0)
		return TRUE;
	if (strncmp(param, "PNG", 3) == 0)
		return TRUE;
	if (strncmp(param, "PNM", 3) == 0)
		return TRUE;
	if (strncmp(param, "TIF", 3) == 0)
		return TRUE;
	if (strncmp(param, "XCF", 3) == 0)
		return TRUE;
	if (strncmp(param, "XPM", 3) == 0)
		return TRUE;
	if (strncmp(param, "XV", 3) == 0)
		return TRUE;
#endif
	return FALSE;
}

static struct bitmap_t *
sdl_load_bitmap(const char * name)
{
#ifdef SDLIMAGE_ENABLE
	return NULL;
#else
	THROW(EXP_UNCATCHABLE, "shouldn't be here: SDLIMAGE is disabled");
	return NULL;
#endif
}

static void
sdl_save_bitmap(struct bitmap_t * bitmap,
		const char * path)
{
#ifdef SDLIMAGE_ENABLE
	return;
#else
	THROW(EXP_UNCATCHABLE, "shouldn't be here: SDLIMAGE is disabled");
	return;
#endif
	
}

static void
sdl_destroy_bitmap(struct bitmap_t * bitmap)
{
#ifdef SDLIMAGE_ENABLE
	return;
#else
	THROW(EXP_UNCATCHABLE, "shouldn't be here: SDLIMAGE is disabled");
	return;
#endif
	
}

struct bitmap_functionor_t sdl_bitmap_functionor = {
	.name = "SDLImageBitmap",
	.fclass = FC_BITMAP_HANDLER,
	.check_usable = sdl_check_usable,
	.destroy_bitmap = sdl_destroy_bitmap,
	.load_bitmap = sdl_load_bitmap,
	.save_bitmap = sdl_save_bitmap,
};
#endif

// vim:ts=4:sw=4

