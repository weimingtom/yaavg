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
wrap_malloc PNGARG((png_structp p, png_size_t sz))
{
	return xmalloc(sz);
}

static void
wrap_free PNGARG((png_structp p, png_voidp ptr))
{
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
	WARNING(BITMAP, "libpng report error: %s\n", str);
	THROW(EXP_LIBPNG_ERROR, "libpng error %s", str);
}

static void
wrap_png_warn PNGARG((png_structp p, png_const_charp str))
{
	WARNING(BITMAP, "libpng report warning: %s\n", str);
}


void
bitmap_to_png(struct bitmap_t * b, struct io_t * io)
{
	assert(b != NULL);
	assert(io != NULL);
	assert(io->rdwr & IO_WRITE);

	DEBUG(BITMAP, "write bitmap %s to %s as png stream\n",
			b->id, io->id);

	TRACE(BITMAP, "white=%d, height=%d, bpp=%d, format=%d\n",
			b->w, b->h, b->bpp, b->format);
	int png_color_type;
	switch (b->bpp) {
		case 3:
			png_color_type = PNG_COLOR_TYPE_RGB;
			assert((b->format == BITMAP_RGB) || (b->format == BITMAP_BGR));
			break;
		case 4:
			png_color_type = PNG_COLOR_TYPE_RGBA;
			assert((b->format == BITMAP_RGBA) || (b->format == BITMAP_BGRA));
			break;
		default:
			THROW(EXP_UNCATCHABLE, "bitmap format error: bpp=%d, type=%d",
					b->bpp, b->format);
	}

	png_structp write_ptr = NULL;
	png_infop write_info_ptr = NULL;
	uint8_t * rows = NULL;
	struct exception_t exp;
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
		int row_size = b->bpp * b->w;
		DEBUG(BITMAP, "begin to write png stream\n");

		uint8_t ** rows = xmalloc(b->h * sizeof(uint8_t*));
		uint8_t ** prow = rows;
		assert(rows != NULL);
		if (b->revert) {
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
		png_write_rows(write_ptr, rows, b->h);
		png_write_end(write_ptr, write_info_ptr);

	} FINALLY {
		if (rows != NULL)
			xfree(rows);
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

