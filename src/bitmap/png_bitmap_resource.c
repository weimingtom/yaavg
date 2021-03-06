/* 
 * png_bitmap_resource.c
 * by WN @ Dec. 13, 2009
 */

#include <config.h>

#ifndef HAVE_LIBPNG
# error Doesn't have libpng support, shouldn't compile this file. This is an error of CMake system.
#endif

#include <common/debug.h>
#include <common/exception.h>
#include <common/mm.h>
#include <bitmap/bitmap.h>
#include <bitmap/bitmap_resource.h>
#include <io/io.h>

#include <string.h>
#include <assert.h>

/* see pngconf.h */
#define PNG_SKIP_SETJMP_CHECK
#define PNG_USER_MEM_SUPPORTED
#include <png.h>

struct png_bitmap_resource_t {
	struct bitmap_resource_t bitmap_resource;
	char * id;
	uint8_t ** rows;
	void * pixels;
	uint8_t __data[0];
};

static png_voidp
wrap_malloc PNGARG((png_structp p DEBUG_ARG, png_size_t sz))
{
	TRACE(MEMORY, "png_structp %p malloc %d bytes\n", p, sz);
	return xmalloc(sz);
}

static void
wrap_free PNGARG((png_structp p DEBUG_ARG, png_voidp ptr))
{
	TRACE(MEMORY, "png_structp %p free %p bytes\n", p, ptr);
	xfree(ptr);
}

static void
wrap_read PNGARG((png_structp p, png_bytep byte, png_size_t size))
{
	struct io_t * io = (struct io_t *)(p->io_ptr);
	void * buffer = (void*)byte;
	assert(io != NULL);
	assert(buffer != NULL);
	io_read_force(io, buffer, size);
	return;
}

static void
wrap_png_error PNGARG((png_structp p, png_const_charp str))
{
	WARNING(BITMAP, "libpng report error for %p: %s\n", p, str);
	THROW(EXP_LIBPNG_ERROR, "libpng error %s", str);
}

static void
wrap_png_warn PNGARG((png_structp p, png_const_charp str))
{
	WARNING(BITMAP, "libpng report warning for %p: %s\n", p, str);
}

static bool_t
png_check_usable(const char * param)
{
	assert(param != NULL);
	assert(strnlen(param, 4) >= 3);
	DEBUG(BITMAP, "%s:%s\n", __func__, param);

	if (strncmp(param, "PNG", 3) == 0)
		return TRUE;
	return FALSE;
}

static void
png_bitmap_destroy(struct resource_t * r)
{
	assert(r != NULL);
	struct bitmap_resource_t * br =
		container_of(r, struct bitmap_resource_t,
				resource);
	struct png_bitmap_resource_t * pr = 
		container_of(br, struct png_bitmap_resource_t,
				bitmap_resource);
	assert(r->ptr == pr);
	assert(r->pprivate == pr);
	DEBUG(BITMAP, "destroying png bitmap %s\n", r->id);
	xfree(pr);
}

static struct bitmap_resource_t *
png_load(struct io_t * io, const char * id)
{
	assert(io != NULL);
	DEBUG(BITMAP, "loading image %s use png loader, io is %s\n",
			id, io->id);

	catch_var(png_structp, read_ptr, NULL);
	catch_var(png_infop, read_info_ptr, NULL);
	catch_var(struct png_bitmap_resource_t *, png_res, NULL);
	define_exp(exp);
	TRY(exp) {
		set_catched_var(read_ptr, png_create_read_struct_2(
				PNG_LIBPNG_VER_STRING,
				NULL,
				wrap_png_error,
				wrap_png_warn,
				NULL,
				wrap_malloc,
				wrap_free));
		if (read_ptr == NULL)
			THROW(EXP_LIBPNG_ERROR, "unable to alloc png write struct");

		if (setjmp(png_jmpbuf(read_ptr))) {
			WARNING(BITMAP, "libpng internal error, and we shouldn't get here.\n");
			THROW(EXP_LIBPNG_ERROR, "libpng internal error");
		}

		set_catched_var(read_info_ptr, png_create_info_struct(read_ptr));
		if (read_info_ptr == NULL)
			THROW(EXP_LIBPNG_ERROR, "unable to alloc png info struct");
		png_set_read_fn(read_ptr, io, wrap_read);

		/* begin read */

		/* below code comes from SDL_image */
		png_uint_32 width, height;
		int bit_depth, color_type, interlace_type;
		png_read_info(read_ptr, read_info_ptr);

		png_get_IHDR(read_ptr, read_info_ptr, &width, &height, &bit_depth,
				&color_type, &interlace_type, NULL, NULL);
		png_set_strip_16(read_ptr);
		png_set_packing(read_ptr);

		if (color_type == PNG_COLOR_TYPE_GRAY)
			png_set_expand(read_ptr);

		if ((color_type == PNG_COLOR_TYPE_GRAY_ALPHA) || 
			(color_type == PNG_COLOR_TYPE_GRAY))
			png_set_gray_to_rgb(read_ptr);

		if (png_get_valid(read_ptr, read_info_ptr, PNG_INFO_tRNS)) {
			TRACE(BITMAP, "stream has PNG_INFO_tRNS\n");
			png_set_tRNS_to_alpha(read_ptr);
		}

		png_read_update_info(read_ptr, read_info_ptr);
		png_get_IHDR(read_ptr, read_info_ptr, &width, &height, &bit_depth,
				&color_type, &interlace_type, NULL, NULL);

		if (bit_depth != 8) {
			WARNING(BITMAP, "After the transformation, the bit_depth is %d, still not 8. This is %s\n",
					bit_depth, io->id);
			THROW(EXP_LIBPNG_ERROR, "libpng error: bit_depth is %d for %s",
					bit_depth, io->id);
		}

		enum bitmap_format format;
		int bpp;
		switch (color_type) {
			case PNG_COLOR_TYPE_RGB:
				bpp = 3;
				format = BITMAP_RGB;
				break;
			case PNG_COLOR_TYPE_RGB_ALPHA:
				bpp = 4;
				format = BITMAP_RGBA;
				break;
			default:
				WARNING(BITMAP, "After the transformation, the color type is %d, "
						"not one of PNG_COLOR_TYPE_RGB or PNG_COLOR_TYPE_RGB_ALPHA. "
						"This is %s", color_type, io->id);
				THROW(EXP_LIBPNG_ERROR, "libpng error: color_type is %d for %s",
						color_type, io->id);
		}

		/* now let's create the png_bitmap_resource_t */
		int id_sz = strlen(id) + 1;
		int one_row_sz = bpp * width;
		int total_sz = sizeof(*png_res) +
			id_sz +
			sizeof(void*) * height +
			sizeof(uint8_t) * width * height * bpp + 14;
		TRACE(BITMAP, "the size of %s is %lux%lu, bpp is %d. we alloc %d bytes\n",
				io->id, width, height, bpp, total_sz);

		set_catched_var(png_res, xmalloc(total_sz));
		assert(png_res != NULL);

		struct bitmap_resource_t * btm_res = &(png_res->bitmap_resource);
		struct resource_t * r = &(btm_res->resource);
		struct bitmap_t * h = &(btm_res->head);

		png_res->id = (char*)png_res->__data;
		png_res->rows = (uint8_t**)ALIGN_UP_PTR(png_res->id + id_sz, 8);
		/* ** png_res->rows is a ** type, don't mul 4 ** */
		png_res->pixels = ALIGN_UP_PTR(png_res->rows + height, 8);

		strcpy(png_res->id, id);
		for (unsigned int i = 0; i < height; i++) {
			png_res->rows[i] =
				png_res->pixels + one_row_sz * i;
		}

		/* read the entire image */
		png_read_image(read_ptr, png_res->rows);

		/* fill the resource and bitmap */
		h->invert_y_axis = FALSE;
		h->id = png_res->id;
		h->id_sz = id_sz;
		h->format = format;
		h->bpp = bpp;
		h->x = h->y = 0;
		h->w = width;
		h->align = 1;
		h->pitch = width * bpp;
		h->h = height;
		h->pixels = png_res->pixels;

		r->id = h->id;
		r->res_sz = total_sz;
		r->serialize = generic_bitmap_serialize;
		r->destroy = png_bitmap_destroy;
		r->ptr = r->pprivate = png_res;
		/* don't return inside try block */
	} FINALLY {
		get_catched_var(read_ptr);
		get_catched_var(read_info_ptr);
		if (read_ptr != NULL)
			png_destroy_read_struct(&read_ptr, &read_info_ptr, 0);
		assert(read_info_ptr == NULL);
	} CATCH(exp) {
		get_catched_var(png_res);
		xfree_null_catched(png_res);
		switch (exp.type) {
			case EXP_LIBPNG_ERROR:
				print_exception(&exp);
				THROW(EXP_BAD_RESOURCE, "unable to load png image %s use io %s",
						id, io->id);
				break;
			default:
				RETHROW(exp);
		}
	}
	assert(png_res != NULL);
	TRACE(BITMAP, "png_res %p loaded for %s\n",
			png_res, png_res->id);
	return &(png_res->bitmap_resource);
}

struct bitmap_resource_functionor_t png_bitmap_resource_functionor = {
	.name = "LibPNGBitmap",
	.fclass = FC_BITMAP_RESOURCE_HANDLER,
	.check_usable = png_check_usable,
	.load = png_load,
};



#if 0
static struct bitmap_resource_functionor_t
png_bitmap_functionor;

static bool_t
png_check_usable(const char * param)
{
	return FALSE;
}

static struct bitmap_resource_functionor_t png_bitmap_functionor = {
	.name = "libpng pngloader",
	.fclass = FC_BITMAP_RESOURCE_HANDLER,
	.check_usable = png_check_usable,
};

#endif
// vim:ts=4:sw=4

