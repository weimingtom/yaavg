/* 
 * bitmap.h
 * by WN @ Dec. 13, 2009
 */

#ifndef __BITMAP_H
#define __BITMAP_H

#include <common/cache.h>
#include <common/defs.h>
#include <common/functionor.h>
#include <utils/rect.h>
#include <utils/rect_mesh.h>
#include <io/io.h>

__BEGIN_DECLS

/* PIXELS_ALIGN should be power of 2, and no less than 8 */
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
	/* 
	 * some bitmap, such as AIR's PDT, has inverted yaxis. see bitmap_to_png.c
	 * */
	bool_t invert_y_axis;
	/* this is only used in sdl screenshot,
	 * for bitmap_to_png to invert alpha channel,
	 * don't use it in other place!! */
	bool_t invert_alpha;
	/* some sdl internal image us 32 bits pixel, but no real alpha. */
	const char * id;
	/* this is important for deserializing */
	int id_sz;
	enum bitmap_format format;
	/* bpp is Bytes per pixer */
	int bpp;
	int x, y, w, h;
	/* pitch is the size of each line **IN BYTES** */
	int pitch;
	int align;
	int total_sz;
	/* ref_count is maintained by caller,
	 * bitmap.c doesn't care about it */
	int ref_count;
	uint8_t * pixels;
	/* private */
	void (*destroy)(struct bitmap_t * b);
	uint8_t __data[0];
};


static inline int
bitmap_data_size(struct bitmap_t * s)
{
	return s->pitch * s->h;
}

struct bitmap_deserlize_param {
	/* align each line with 1, 4, 8 or 16 */
	int align;
	/* force to build a non-revert bitmap */
	bool_t revert_y_axis;
};

struct bitmap_t *
bitmap_deserialize(struct io_t *, struct bitmap_deserlize_param *);

void
free_bitmap(struct bitmap_t * ptr);

struct bitmap_t *
clip_bitmap(struct bitmap_t * ori, struct rect_t rect, int align);

struct bitmap_array_t {
	/* not null original_bitmap indicates that it
	 * satisfies the size limitation and won't be freed
	 * when this array is alloced. in this situation,
	 * tiles is also pointed to original_bitmap, id field
	 * is original_bitmap->id, **NOTE** additional string is alloced.
	 * nr_w, nr_h and nr_tiles are all 1; total_sz
	 * is original_bitmap->total_sz plus the size of
	 * the array structure.
	 * **NOTE** when destroying bitmap array, the original bitmap will also
	 * be freed.
	 * **NOTE** no matter whetner the original bitmap satisfies the size limitation,
	 * the ref_count field of bitmap is not affected.
	 * */
	/* null original_bitmap indicates that the original bitmap
	 * doesn't satisify the size limitation and been splitted.
	 * the original bitmap can be safely freed then. in this situation,
	 * the head field is copied from original_bitmap, tiles are
	 * array of struct bitmap_t, all data stored in __data field */
	struct bitmap_t * original_bitmap;
	/* tiles is an array of bitmap head */
	struct bitmap_t head;
	struct bitmap_t * tiles;

	const char * id;

	int align;
	int sz_lim_w;
	int sz_lim_h;
	int nr_w;
	int nr_h;
	int nr_tiles;
	int total_sz;
	void (*destroy)(struct bitmap_array_t * b);

	uint8_t __data[0];
};

/* 
 * split a big bitmap into small pieces.
 * sz_w and sz_h are the size limitation of small tiles.
 *
 * return value: return an array of bitmaps.
 *
 * if b satisifies the size limitation, return NULL.
 */
struct bitmap_array_t *
split_bitmap(struct bitmap_t * b, int sz_lim_w, int sz_lim_h, int align);

void
fill_mesh_by_array(struct bitmap_array_t * array, struct rect_mesh_t * mesh);

static inline struct rect_mesh_t *
build_mesh_by_array(struct bitmap_array_t * array)
{
	assert(array != NULL);
	struct rect_mesh_t * m = alloc_rect_mesh(array->nr_w, array->nr_h);
	fill_mesh_by_array(array, m);
	return m;
}

void
free_bitmap_array(struct bitmap_array_t * ptr);

__END_DECLS
#endif

// vim:ts=4:sw=4

