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
#define PIXELS_PTR(ptr)	((uint8_t*)(ALIGN_UP((ptr), PIXELS_ALIGN)))


enum bitmap_format {
	BITMAP_RGB,
	BITMAP_RGBA,
};

/* data should be fixed when deserializing */
/* when deserializing, id is actually the length of id */
/* and pixels is actually the length of pixels.
 * in fact, pixels is useless. but we employ it for check
 * */
#define BITMAP_HEAD				\
	char * id;			\
	int id_sz;			\
	enum bitmap_format format;	\
	int bpp;		\
	int w, h;					\
	uint8_t * pixels;

/* 
 * this is the host side bitmap
 */
struct bitmap_t {
	BITMAP_HEAD
	uint8_t __data[0];
};


static inline int
bitmap_data_size(struct bitmap_t * s)
{
	return s->w * s->h * s->bpp;
}

struct bitmap_t *
bitmap_deserialize(struct io_t *);

void
free_bitmap(struct bitmap_t * ptr);

struct bitmap_t *
alloc_bitmap(struct bitmap_t * head, int id_sz);

__END_DECLS
#endif

// vim:ts=4:sw=4

