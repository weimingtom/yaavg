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
dummy_serialize(struct resource_t * r, struct io_t * io)
{
	assert(r != NULL);
	assert(io != NULL);
	struct bitmap_resource_t * b = container_of(r,
			struct bitmap_resource_t,
			resource);
	DEBUG(BITMAP, "serializing dummy bitmap %s\n",
			b->head.id);

	struct bitmap_t head = b->head;
	head.id = NULL;
	head.pixels = NULL;

	int sync = BEGIN_SERIALIZE_SYNC;
	io_write(io, &sync, sizeof(sync), 1);

	struct iovec vecs[3];

	vecs[0].iov_base = &head;
	vecs[0].iov_len = sizeof(head);

	vecs[1].iov_base = b->head.id;
	vecs[1].iov_len = b->head.id_sz;

	vecs[2].iov_base = b->head.pixels;
	vecs[2].iov_len = bitmap_data_size(&head);


	if (io->functionor->vmsplice) {
		io_vmsplice(io, vecs, 3);
	} else if (io->functionor->writev) {
		io_writev(io, vecs, 3);
	} else {
		io_write(io, vecs[0].iov_base, vecs[0].iov_len, 1);
		io_write(io, vecs[1].iov_base, vecs[1].iov_len, 1);
		io_write(io, vecs[2].iov_base, vecs[2].iov_len, 1);
	}

	/* wait for sync */
	/* end sync have to reside here because of the lifetime of head:
	 * it will dead right after we return from this function */
	io_read(io, &sync, sizeof(sync), 1);
	assert(sync == END_DESERIALIZE_SYNC);
	DEBUG(BITMAP, "got sync mark\n");

	DEBUG(BITMAP, "dummy bitmap %s send over\n",
			b->head.id);
}

static void
dummy_destroy(struct resource_t * r)
{
	assert(r != NULL);
	struct bitmap_resource_t * b = container_of(r,
			struct bitmap_resource_t,
			resource);
	DEBUG(BITMAP, "dummy bitmap %s(%p) destroied\n",
			b->head.id, b->head.id);
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
	r->serialize = dummy_serialize;
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

