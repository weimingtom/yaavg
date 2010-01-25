/* 
 * sdl_png_bitmap.c
 * by WN @ Dec. 13, 2009
 */


#include <config.h>
#include <common/debug.h>
#include <common/exception.h>
#include <common/defs.h>

#include <io/io.h>
#include <bitmap/bitmap_resource.h>

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

static void inline
check_sdl_image(void)
{
#ifdef SDLIMAGE_ENABLE
	return;
#else
	THROW(EXP_UNCATCHABLE, "shouldn't be here: SDLIMAGE is disabled");
#endif

}


struct bitmap_resource_functionor_t
sdl_bitmap_resource_functionor;

static bool_t
sdl_check_usable(const char * param)
{
	assert(param != NULL);
	assert(strnlen(param, 4) >= 3);
	TRACE(BITMAP, "%s:%s\n", __func__, param);
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
	if (strncmp(param, "XV", 2) == 0)
		return TRUE;
#endif
	return FALSE;
}

static void
sdl_serialize(struct resource_t * r, struct io_t * io)
{
	check_sdl_image();
	assert(r != NULL);
	assert(io != NULL);
}

static void
sdl_destroy(struct resource_t * r)
{
	check_sdl_image();
}


struct wrap_rwops {
	struct SDL_RWops rwops;
	struct io_t * io;
};

static inline struct io_t *
get_io(struct SDL_RWops * rwops)
{
	assert(rwops != NULL);
	struct wrap_rwops * w = container_of(rwops,
			struct wrap_rwops,
			rwops);
	return w->io;
}

static int
wrap_seek(struct SDL_RWops * ops, int offset, int whence)
{
	TRACE(BITMAP, "ops=%p, seek %d, %d\n", ops, offset, whence);
	struct io_t * io = get_io(ops);
	TRACE(BITMAP, "io=%p\n", io);
	assert(io != NULL);
	return io_seek(io, offset, whence);
}

static int
wrap_read(struct SDL_RWops * ops, void * ptr,
		int size, int maxnum)
{
	TRACE(BITMAP, "read %p, %d, %d\n", ptr, size, maxnum);
	struct io_t * io = get_io(ops);
	assert(io != NULL);
	return io_read(io, ptr, size, maxnum);
}

static int
wrap_write(struct SDL_RWops * ops, const void * ptr,
		int size, int maxnum)
{
	struct io_t * io = get_io(ops);
	assert(io != NULL);
	THROW(EXP_UNCATCHABLE, "shouldn't call this function\n");
	return 0;
}

static int
wrap_close(struct SDL_RWops * ops, const void * ptr,
		int size, int maxnum)
{
	/* don't close io_t! */
	struct io_t * io = get_io(ops);
	assert(io != NULL);
	return 0;
}

static struct bitmap_resource_t *
sdl_load(struct io_t * io, const char * id)
{
	check_sdl_image();
	assert(io != NULL);

	TRACE(BITMAP, "loading image %s use sdl loader, io is %s\n",
			id, io->functionor->name);
	/* build SDL_rwops */
	struct wrap_rwops rwops;

#define SET_FUNC(x)	\
	do {	\
		rwops.rwops.x = wrap_##x;		\
	} while(0)

	SET_FUNC(seek);
	SET_FUNC(read);
	SET_FUNC(write);
	SET_FUNC(close);

#undef SET_FUNC
	rwops.io = io;

	SDL_Surface * img = IMG_Load_RW(&rwops.rwops, 0);
	if (img == NULL)
		THROW(EXP_BAD_RESOURCE, "load image %s use io %s failed: %s\n",
				id, io->functionor->name, SDL_GetError());

	/* trace the information of img */
	TRACE(BITMAP, "format of %s:\n", id);
	TRACE(BITMAP, "\tsize: %dx%d\n", img->w, img->h);
	struct SDL_PixelFormat * f = img->format;
	TRACE(BITMAP, "\tbits pre pixel: %d\n", f->BitsPerPixel);
	TRACE(BITMAP, "\tbytes pre pixel: %d\n", f->BytesPerPixel);
	TRACE(BITMAP, "\t(R, G, B, A)loss=(%d, %d, %d, %d)\n",
			f->Rloss,
			f->Gloss,
			f->Bloss,
			f->Aloss);
	TRACE(BITMAP, "\t(R, G, B, A)shift=(%d, %d, %d, %d)\n",
			f->Rshift,
			f->Gshift,
			f->Bshift,
			f->Ashift);
	TRACE(BITMAP, "\t(R, G, B, A)mask=(0x%x, 0x%x, 0x%x, 0x%x)\n",
			f->Rmask,
			f->Gmask,
			f->Bmask,
			f->Amask);
	TRACE(BITMAP, "\tcolorkey=0x%x\n",
			f->colorkey);
	TRACE(BITMAP, "\talpha=0x%x\n",
			f->alpha);

	SDL_FreeSurface(img);
	THROW(EXP_BAD_RESOURCE, "load image %s use io %s failed: %s\n",
			id, io->functionor->name, SDL_GetError());
	/* lock before free */
	SDL_LockSurface(img);
	return NULL;
}


struct bitmap_resource_functionor_t sdl_bitmap_resource_functionor = {
	.name = "SDLImageBitmap",
	.fclass = FC_BITMAP_RESOURCE_HANDLER,
	.check_usable = sdl_check_usable,
	.load = sdl_load,
};

// vim:ts=4:sw=4

