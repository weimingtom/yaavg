/* 
 * bitmap.h
 * by WN @ Dec. 13, 2009
 */

#ifndef __BITMAP_H
#define __BITMAP_H

#include <common/cache.h>
#include <common/defs.h>
#include <common/functionor.h>
#include <io/io.h>

__BEGIN_DECLS

#define PIXELS_ALIGN	(16)
#define PIXELS_PTR(ptr)	((uint8_t*)(ALIGN_UP_PTR((ptr), PIXELS_ALIGN)))


enum bitmap_format {
	BITMAP_RGB,
	BITMAP_RGBA,
	BITMAP_BGR,
	BITMAP_BGRA,
};

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define SET_COLOR_MASKS(format, rmask, gmask, bmask, amask)	\
	do {	\
		switch (format) {	\
			case BITMAP_RGB:	\
				rmask = 0xff;	\
				gmask = 0xff00;	\
				bmask = 0xff0000;	\
				amask = 0;	\
				break;	\
			case BITMAP_RGBA:	\
				rmask = 0xff;	\
				gmask = 0xff00;	\
				bmask = 0xff0000;	\
				amask = 0xff000000;	\
				break;	\
			case BITMAP_BGR:	\
				rmask = 0xff0000;	\
				gmask = 0xff00;	\
				bmask = 0xff;	\
				amask = 0;	\
				break;	\
			case BITMAP_BGRA:	\
				rmask = 0xff0000;	\
				gmask = 0xff00;	\
				bmask = 0xff;	\
				amask = 0xff000000;	\
				break;	\
		}	\
	} while(0)
#else
# define BITMAP_FORMAT_TO_MASKS(format, rmask, gmask, bmask, amask) do { } while(0)
# error I do not know the rgba masks in big endian machine, please tell me
#endif
/* data should be fixed when deserializing */
/* when deserializing, id is actually the length of id */
/* and pixels is actually the length of pixels.
 * in fact, pixels is useless. but we employ it for check
 * */

/* 
 * this is the host side bitmap
 */
struct bitmap_t {
	/* public */
	bool_t revert;
	/* 
	 * some bitmap, such as AIR's PDT, is reverted. see bitmap_to_png.c
	 * */
	char * id;
	int id_sz;
	enum bitmap_format format;
	/* bpp is Bytes per pixer */
	int bpp;
	int w, h;
	/* pitch is the size of each line **IN BYTES** */
	int pitch;
	int ref_count;
	uint8_t * pixels;
	/* private */
	void (*destroy_bitmap)(struct bitmap_t * b);
	uint8_t __data[0];
};


static inline int
bitmap_data_size(struct bitmap_t * s)
{
	return s->pitch * s->h;
}

struct bitmap_deserlize_param {
	int align;	/* align each line with 1, 4, 8 or 16 */
};

struct bitmap_t *
bitmap_deserialize(struct io_t *, struct bitmap_deserlize_param *);

void
free_bitmap(struct bitmap_t * ptr);

__END_DECLS
#endif

// vim:ts=4:sw=4

