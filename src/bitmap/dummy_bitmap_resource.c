/* 
 * dummy_bitmap.c
 * by WN @ Jan. 16, 2010
 */


#include <config.h>
#include <common/defs.h>
#include <common/debug.h>
#include <common/functionor.h>
#include <common/mm.h>
#include <bitmap/bitmap_resource.h>
#include <assert.h>

struct bitmap_resource_functionor_t
dummy_bitmap_resource_functionor;

static bool_t
dummy_check_usable(const char * param)
{
	DEBUG(BITMAP, "%s:%s\n", __func__, param);
	return TRUE;
}


#define DUMMY_W		(8)
#define DUMMY_H		(8)
#define DUMMY_BPP	(4)
static uint8_t __dummy_pixels[DUMMY_W * DUMMY_H * DUMMY_BPP];

static void
dummy_destroy(struct resource_t * r)
{
	assert(r != NULL);
#ifdef YAAVG_DEBUG
	struct bitmap_resource_t * b = container_of(r,
			struct bitmap_resource_t,
			resource);
	DEBUG(BITMAP, "dummy bitmap %s(%p) destroied\n",
			b->head.id, b->head.id);
#endif
	xfree(r);
}

static struct bitmap_resource_t *
dummy_load(struct io_t * io, const char * id)
{
	struct bitmap_resource_t * b = NULL;
	WARNING(BITMAP, "load bitmap %s using dummy handler\n", id);

	int total_sz = sizeof(*b) + strlen(id) + 1;

	b = xcalloc(1, total_sz);
	assert(b != NULL);

	struct resource_t * r = &b->resource;
	struct bitmap_t * h = &b->head;

	strcpy((void*)(b->__data), id);

	h->id = (void*)(b->__data);
	h->id_sz = strlen(h->id) + 1;
	h->format = BITMAP_RGBA;
	h->bpp = DUMMY_BPP;
	h->w = DUMMY_W;
	h->h = DUMMY_H;
	h->pixels = __dummy_pixels;

	r->id = h->id;
	r->res_sz = total_sz;
	r->serialize = generic_bitmap_serialize;
	r->destroy = dummy_destroy;
	r->ptr = b;
	r->pprivate = NULL;

	return b;
}

struct bitmap_resource_functionor_t dummy_bitmap_resource_functionor = {
	.name = "DummyBitmap",
	.fclass = FC_BITMAP_RESOURCE_HANDLER,
	.check_usable = dummy_check_usable,
	.load = dummy_load,
};

// vim:ts=4:sw=4

