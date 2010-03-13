/* 
 * bitmap_to_png.c
 * by WN @ Feb. 03, 2010
 */

#include <config.h>
#if HAVE_LIBPNG

#include <common/debug.h>
#include <common/exception.h>
#include <bitmap/bitmap.h>
#include <io/io.h>
#include <bitmap/bitmap_to_png.h>

/* see pngconf.h */
#define PNG_SKIP_SETJMP_CHECK
#define PNG_USER_MEM_SUPPORTED
#include <png.h>

static png_voidp
wrap_malloc PNGARG((png_structp p DEBUG_ARG, png_size_t sz))
{
	TRACE(MEMORY, "png_structp %p malloc %d bytes\n", p, sz);
	return xmalloc(sz);
}

static void
wrap_free PNGARG((png_structp p DEBUG_ARG, png_voidp ptr))
{
	TRACE(MEMORY, "png_structp %p free %p\n", p, ptr);
	xfree(ptr);
}

static void
wrap_write(png_structp p, png_bytep byte, png_size_t size)
{
	struct io_t * io = (struct io_t *)(p->io_ptr);
	void * buffer = (void*)byte;
	assert(io != NULL);
	assert(buffer != NULL);
	io_write_force(io, buffer, size);
	return;
}

static void
wrap_flush(png_structp p)
{
	struct io_t * io = (struct io_t *)(p->io_ptr);
	assert(io != NULL);
	io_flush(io);
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


void
bitmap_to_png(struct bitmap_t * b, struct io_t * io)
{
	assert(b != NULL);
	assert(io != NULL);
	assert(io->rdwr & IO_WRITE);
	assert(b->pitch >= b->w * b->bpp);

	DEBUG(BITMAP, "write bitmap %s to %s as png stream\n",
			b->id, io->id);

	TRACE(BITMAP, "white=%d, height=%d, bpp=%d, format=%d\n",
			b->w, b->h, b->bpp, b->format);
	if ((b->bpp != 3) && (b->bpp != 4))
		THROW(EXP_UNCATCHABLE, "bitmap format error: bpp=%d, type=%d",
				b->bpp, b->format);

	png_structp write_ptr = NULL;
	png_infop write_info_ptr = NULL;

	catch_var(uint8_t **, rows, NULL);
	define_exp(exp);
	TRY(exp) {

		write_ptr = png_create_write_struct_2(
				PNG_LIBPNG_VER_STRING,
				NULL,
				wrap_png_error,
				wrap_png_warn,
				NULL,
				wrap_malloc,
				wrap_free);
		if (write_ptr == NULL)
			THROW(EXP_LIBPNG_ERROR, "unable to alloc png write struct");

		if (setjmp(png_jmpbuf(write_ptr))) {
			WARNING(BITMAP, "libpng internal error, and we shouldn't get here.\n");
			THROW(EXP_LIBPNG_ERROR, "libpng internal error");
		}

		/* move the setting of png_color_type after the setjmp to avoid the
		 * "variable might be clobbered by 'longjmp' warning"
		 * */
		int png_color_type = PNG_COLOR_TYPE_RGBA;
		switch (b->bpp) {
			case 3:
				png_color_type = PNG_COLOR_TYPE_RGB;
				assert((b->format == BITMAP_RGB) || (b->format == BITMAP_BGR));
				break;
			default:
				png_color_type = PNG_COLOR_TYPE_RGBA;
				assert((b->format == BITMAP_RGBA) || (b->format == BITMAP_BGRA));
				break;
		}


		write_info_ptr = png_create_info_struct(write_ptr);
		if (write_info_ptr == NULL)
			THROW(EXP_LIBPNG_ERROR, "unable to alloc png info struct");

		png_set_write_fn(write_ptr, io, wrap_write, wrap_flush);


		/* start write */
		png_set_IHDR(write_ptr, write_info_ptr, b->w, b->h, 8, png_color_type,
				PNG_INTERLACE_NONE,
				PNG_COMPRESSION_TYPE_DEFAULT,
				PNG_FILTER_TYPE_DEFAULT);

		png_text texts = {
			.key = "Software",
			.text = "YAAVG png writer",
			.compression = PNG_TEXT_COMPRESSION_NONE,
		};
		png_set_text(write_ptr, write_info_ptr, &texts, 1);
		png_write_info(write_ptr, write_info_ptr);

		/* begin write data */
		int row_size = b->pitch;
		DEBUG(BITMAP, "begin to write png stream\n");

		set_catched_var(rows, xmalloc(b->h * sizeof(uint8_t*)));
		assert(rows != NULL);
		uint8_t ** prow = rows;
		if (b->invert_y_axis) {
			for (int n = b->h - 1; n >= 0; n--) {
				* prow = b->pixels + n * row_size;
				prow ++;
			}
		} else {
			for (int n = 0; n < b->h; n++) {
				* prow = b->pixels + n * row_size;
				prow ++;
			}
		}

		/* for sdl screenshot */
		if (b->invert_alpha)
			png_set_invert_alpha(write_ptr);

		png_set_rows(write_ptr, write_info_ptr, rows);
		if ((b->format == BITMAP_BGR) || (b->format == BITMAP_BGRA))
			png_write_png(write_ptr, write_info_ptr, PNG_TRANSFORM_BGR, NULL);
		else
			png_write_png(write_ptr, write_info_ptr, 0, NULL);
		png_write_end(write_ptr, write_info_ptr);

	} FINALLY {
		get_catched_var(rows);
		xfree_null_catched(rows);
		if (write_ptr != NULL)
			png_destroy_write_struct(&write_ptr, &write_info_ptr);
		assert(write_info_ptr == NULL);
	} NO_CATCH(exp);
	return;
}

#else

void
bitmap_to_png(struct bitmap_t * b, struct io_t * io)
{
	THROW(EXP_UNSUPPORT_OPERATION, "this compilation doesn't support png output");
	return;
}


#endif

// vim:ts=4:sw=4

