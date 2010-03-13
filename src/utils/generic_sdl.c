/* 
 * utils/generic_sdl.c
 * by WN @ Feb. 08, 2010
 */

#include <config.h>
#ifdef HAVE_SDL
#include <signal.h>

#include <utils/generic_sdl.h>
#include <yconf/yconf.h>
#include <SDL/SDL.h>

void
generic_init_sdl(void)
{
#if 0
	SDL_SetEventFilter(NULL);
#endif
	SDL_EnableUNICODE(SDL_ENABLE);
	int delay, interval;
	delay = conf_get_int("sys.events.keyrepeatdelay", -1);
	interval = conf_get_int("sys.events.keyrepeatinterval", -1);
	if (delay < 0)
		delay = SDL_DEFAULT_REPEAT_DELAY;
	if (interval < 0)
		interval = SDL_DEFAULT_REPEAT_INTERVAL;
	SDL_EnableKeyRepeat(delay, interval);
}

void
generic_sdl_unblock_sigint(void)
{
	signal(SIGINT, SIG_DFL);
}

void
fill_bitmap_by_sdlsurface(struct bitmap_t * h, SDL_Surface * img,
		const char * id, int id_sz)
{
	assert(h != NULL);
	assert(img != NULL);

	struct SDL_PixelFormat * f = img->format;
	h->id = id;
	h->id_sz = id_sz;

	/* check whether the pixel format is correct */
	if ((f->BytesPerPixel != 3) && (f->BytesPerPixel != 4))
		THROW(EXP_UNSUPPORT_FORMAT, "bitmap %s bytes_pre_pixel is %d",
				h->id, f->BytesPerPixel);

	/* select a format */
	enum bitmap_format format;
	if (f->BytesPerPixel == 3) {
		/* RGBA or BGRA */
		/* we assume we are in a little endian machine,
		 * see the head of this file */
		if ((f->Rmask == 0xff) &&
				(f->Gmask == 0xff00) &&
				(f->Bmask == 0xff0000))
			format = BITMAP_RGB;
		else if ((f->Bmask == 0xff) &&
				(f->Gmask == 0xff00) &&
				(f->Rmask == 0xff0000))
			format = BITMAP_BGR;
		else
			THROW(EXP_UNSUPPORT_FORMAT, "bitmap %s rgb mask is strange: (0x%x, 0x%x, 0x%x)",
					h->id, f->Rmask, f->Gmask, f->Bmask);
	} else {
		assert(f->BytesPerPixel == 4);

		if ((f->Rmask == 0xff) &&
				(f->Gmask == 0xff00) &&
				(f->Bmask == 0xff0000) && 
				(f->Amask == 0xff000000))
		{
			format = BITMAP_RGBA;
		}
		else if ((f->Bmask == 0xff) &&
				(f->Gmask == 0xff00) &&
				(f->Rmask == 0xff0000) &&
				(f->Amask == 0xff000000))
		{
			format = BITMAP_BGRA;
		}
		else
			THROW(EXP_UNSUPPORT_FORMAT, "bitmap %s rgba mask is strange: (0x%x, 0x%x, 0x%x, 0x%x)",
					h->id, f->Rmask, f->Gmask, f->Bmask, f->Amask);
	}
	h->invert_y_axis = FALSE;

	h->format = format;
	h->bpp = f->BytesPerPixel;
	h->x = h->y = 0;
	h->w = img->w;
	h->h = img->h;
	h->pitch = img->w * h->bpp;
	h->align = 1;
	h->pixels = (uint8_t*)(img->pixels);
}

#endif

// vim:ts=4:sw=4

