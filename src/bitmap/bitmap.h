/* 
 * bitmap.h
 * by WN @ Dec. 13, 2009
 */

#ifndef __BITMAP_H
#define __BITMAP_H

#include <common/defs.h>
#include <common/functionor.h>

enum bitmap_format {
	BITMAP_RGB = 3,
	BITMAP_RGBA = 4,
	BITMAP_LUMINANCE = 1,
	BITMAP_LUMINANCE_ALPHA = 2,
};

typedef enum bitmap_format bytes_pre_pixel;

struct bitmap_functionor_t;
struct bitmap_t {
	enum bitmap_format format;
	int w, h;
	struct bitmap_functionor_t * functionor;
	void * pprivate;
	uint8_t * data;
	uint8_t internal_data[0];
};

struct rgba_pixel {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};

static inline int
bitmap_data_size(struct bitmap_t * s)
{
	return s->w * s->h * s->format;
}

extern struct function_class_t
bitmap_function_class;

struct bitmap_functionor_t {
	BASE_FUNCTIONOR
	struct bitmap_t * (*load_bitmap)(const char * name);
	void (*save_bitmap)(struct bitmap_t *, const char * path);
	void (*destroy_bitmap)(struct bitmap_t *);
};

extern struct bitmap_functionor_t *
get_bitmap_handler(const char * name);

extern struct bitmap_t *
load_bitmap(const char * name);

extern void
destroy_bitmap(struct bitmap_t * bitmap);

#endif

// vim:ts=4:sw=4

