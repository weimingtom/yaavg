/* 
 * bitmap.h
 * by WN @ Dec. 13, 2009
 */

#ifndef __BITMAP_H
#define __BITMAP_H

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
/* and pixels is actually the length of pixels */
#define BITMAP_HEAD				\
	const char * id;			\
	enum bitmap_format format;	\
	int bpp;		\
	int w, h;					\
	uint8_t * pixels;
struct bitmap_t {
	BITMAP_HEAD
	uint8_t __data[0];
};

struct bitmap_resource_functionor_t;
struct bitmap_resource_t {
	BITMAP_HEAD
	void * pprivate;
	struct bitmap_resource_functionor_t * functionor;
};

static inline int
bitmap_data_size(struct bitmap_t * s)
{
	return s->w * s->h * s->bpp;
}

struct bitmap_resource_functionor_t {
	BASE_FUNCTIONOR
	struct bitmap_resource_t * (*load)(const char * name);
	void (*destroy)(struct bitmap_resource_t *);
	void (*store)(struct bitmap_t *, const char * path);
	void (*serialize)(struct bitmap_resource_t *, struct io_t *);
};

extern struct bitmap_resource_functionor_t *
get_bitmap_resource_handler(const char * name);

extern
struct function_class_t bitmap_resource_function_class;

struct bitmap_t *
bitmap_deserialize(struct io_t *);

void
free_bitmap(struct bitmap_t * ptr);

struct bitmap_t *
alloc_bitmap(struct bitmap_t * head);

__END_DECLS
#endif

// vim:ts=4:sw=4

