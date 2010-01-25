/* 
 * bitmap_resource.h
 * by WN @ Jan. 24, 2010
 */

#ifndef __BITMAP_RESOURCE_H
#define __BITMAP_RESOURCE_H

#include <bitmap/bitmap.h>
#include <resource/resource.h>
#include <io/io.h>

/* 
 * this is the resource side bitmap
 */

struct bitmap_resource_t {
	struct resource_t resource;
	struct bitmap_t head;
	void * ptr;
	void * pprivate;
	uint8_t __data[0];
};

struct bitmap_resource_functionor_t {
	BASE_FUNCTIONOR
	struct bitmap_resource_t * (*load)(struct io_t * io, const char * id);
};

static void inline
destroy_bitmap_resource(struct bitmap_resource_t * r)
{
	r->resource.destroy(&r->resource);
}

struct resource_t *
load_bitmap_resource(struct io_t * io, const char * id);

struct resource_t *
load_dummy_bitmap_resource(struct io_t * io, const char * id);

#define BEGIN_SERIALIZE_SYNC	(0x01020304)
#define END_DESERIALIZE_SYNC	(0x10203040)

#endif

// vim:ts=4:sw=4

