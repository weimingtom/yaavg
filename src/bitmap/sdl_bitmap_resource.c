/* 
 * sdl_png_bitmap.c
 * by WN @ Dec. 13, 2009
 */


#include <config.h>

#ifndef HAVE_SDLIMAGE
# error Doesn't have sdlimage support, shouldn't compile this file. This is an error of CMake system.
#endif

#include <common/debug.h>
#include <common/exception.h>
#include <common/defs.h>

#include <io/io.h>
#include <bitmap/bitmap_resource.h>
#include <utils/generic_sdl.h>

#include <string.h>
#include <assert.h>

#include <endian.h>

#if ! __BYTE_ORDER == __LITTLE_ENDIAN
# error Please tell me the value of SDLs RGBA mask in big endian machine.
#endif

#include <SDL/SDL_image.h>

struct bitmap_resource_functionor_t
sdl_bitmap_resource_functionor;

static bool_t
sdl_check_usable(const char * param)
{
	assert(param != NULL);
	assert(strnlen(param, 4) >= 3);
	DEBUG(BITMAP, "%s:%s\n", __func__, param);
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
	return FALSE;
}

static void
sdl_destroy(struct resource_t * r)
{
	assert(r != NULL);
	DEBUG(BITMAP, "destroying sdl image %s\n", r->id);
	/* retrive the bitmap resource structure */
	struct bitmap_resource_t * b = container_of(r,
			struct bitmap_resource_t,
			resource);
	assert(b == r->ptr);
	SDL_Surface * img = r->pprivate;
	assert(img != NULL);
	SDL_UnlockSurface(img);
	SDL_FreeSurface(img);
	DEBUG(BITMAP, "sdl image %s destroyed\n", r->id);
	xfree(b);
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
	TRACE(IO, "ops=%p, seek %d, %d\n", ops, offset, whence);
	struct io_t * io = get_io(ops);
	TRACE(IO, "io=%p\n", io);
	assert(io != NULL);
	io_seek(io, offset, whence);
	return io_tell(io);
}

static int
wrap_read(struct SDL_RWops * ops, void * ptr,
		int size, int maxnum)
{
	TRACE(IO, "ops=%p, read %p, %d, %d\n", ops, ptr, size, maxnum);
	struct io_t * io = get_io(ops);
	assert(io != NULL);
	int err;
	err = io_read(io, ptr, size, maxnum);
#if 0
	TRACE(IO, "io_read return %d\n", err);
	for (int i = 0; i < size * maxnum; i++) {
		if (i % 10 == 0)
			TRACE(IO, "@q\n");
		TRACE(IO, "@q0x%x, ", *(unsigned char*)(ptr + i));
	}
	TRACE(IO, "@q\n");
#endif
	return err;
}

static int
wrap_write(struct SDL_RWops * ops ATTR_UNUSED, const void * ptr ATTR_UNUSED,
		int size ATTR_UNUSED, int maxnum ATTR_UNUSED)
{
	struct io_t * io = get_io(ops);
	assert(io != NULL);
	THROW(EXP_UNCATCHABLE, "shouldn't call this function\n");
	return 0;
}

static int
wrap_close(struct SDL_RWops * ops)
{
	/* don't close io_t! */
	struct io_t * io = get_io(ops);
	assert(io != NULL);
	return 0;
}

static struct bitmap_resource_t *
sdl_load(struct io_t * io, const char * id)
{
	assert(io != NULL);

	DEBUG(BITMAP, "loading image %s use sdl loader, io is %s\n",
			id, io->id);
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
		THROW(EXP_BAD_RESOURCE, "load image %s use io %s failed: %s",
				id, io->id, SDL_GetError());

	/* trace the information of img */
	DEBUG(BITMAP, "format of %s:\n", id);
	DEBUG(BITMAP, "\tsize: %dx%d\n", img->w, img->h);
	struct SDL_PixelFormat * f = img->format;
	DEBUG(BITMAP, "\tbits pre pixel: %d\n", f->BitsPerPixel);
	DEBUG(BITMAP, "\tbytes pre pixel: %d\n", f->BytesPerPixel);
	DEBUG(BITMAP, "\t(R, G, B, A)loss=(%d, %d, %d, %d)\n",
			f->Rloss,
			f->Gloss,
			f->Bloss,
			f->Aloss);
	DEBUG(BITMAP, "\t(R, G, B, A)shift=(%d, %d, %d, %d)\n",
			f->Rshift,
			f->Gshift,
			f->Bshift,
			f->Ashift);
	DEBUG(BITMAP, "\t(R, G, B, A)mask=(0x%x, 0x%x, 0x%x, 0x%x)\n",
			f->Rmask,
			f->Gmask,
			f->Bmask,
			f->Amask);
	DEBUG(BITMAP, "\tcolorkey=0x%x\n",
			f->colorkey);
	DEBUG(BITMAP, "\talpha=0x%x\n",
			f->alpha);
	DEBUG(BITMAP, "\tfirst 4 bytes: (0x%x, 0x%x, 0x%x, 0x%x)\n",
			((uint8_t*)(img->pixels))[0],
			((uint8_t*)(img->pixels))[1],
			((uint8_t*)(img->pixels))[2],
			((uint8_t*)(img->pixels))[3]);

	catch_var(struct bitmap_resource_t *, b, NULL);
	define_exp(exp);
	TRY(exp) {
		int id_sz = strlen(id) + 1;
		set_catched_var(b, xcalloc(1, sizeof(*b) + id_sz));
		assert(b != NULL);

		struct bitmap_t * h = &b->head;
		strcpy((void*)b->__data, id);
		fill_bitmap_by_sdlsurface(h, img, (const char *)(b->__data), id_sz);

		/* create resource structure */
		struct resource_t * r = &b->resource;

		r->id = h->id;
		/* only an estimation, I don't know how to
		 * compute SDL's internal memory storage usage. */
		r->res_sz = sizeof(*img) + bitmap_data_size(h) + id_sz;
		r->serialize = generic_bitmap_serialize;
		r->destroy = sdl_destroy;
		r->ptr = b;

		r->pprivate = img;
		SDL_LockSurface(img);
	} FINALLY {	}
	CATCH(exp) {
		get_catched_var(b);
		xfree_null_catched(b);
		SDL_FreeSurface(img);
		RETHROW(exp);
	}

	if (b == NULL)
		THROW(EXP_UNCATCHABLE, "we shouldn't get here");
	return b;
}


struct bitmap_resource_functionor_t sdl_bitmap_resource_functionor = {
	.name = "SDLImageBitmap",
	.fclass = FC_BITMAP_RESOURCE_HANDLER,
	.check_usable = sdl_check_usable,
	.load = sdl_load,
};

// vim:ts=4:sw=4

