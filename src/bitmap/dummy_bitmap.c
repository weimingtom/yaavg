/* 
 * dummy_bitmap.c
 * by WN @ Jan. 16, 2010
 */


#include <config.h>
#include <common/defs.h>
#include <common/debug.h>
#include <common/functionor.h>
#include <common/mm.h>
#include <bitmap/bitmap.h>
#include <assert.h>

struct bitmap_resource_functionor_t
dummy_bitmap_resource_functionor;

static bool_t
dummy_check_usable(const char * param)
{
	TRACE(BITMAP, "%s:%s\n", __func__, param);
	return TRUE;
}

static void
dummy_destroy(struct bitmap_resource_t * b);

static struct bitmap_resource_t *
dummy_load(struct io_t * io, const char * name)
{
	struct bitmap_resource_t * retval = NULL;

	WARNING(BITMAP, "load bitmap %s using dummy handler\n", name);

	retval = xcalloc(1, sizeof(*retval) + (8 * 8 * 4));
	assert(retval != NULL);

	retval->id = strdup(name);
	retval->format = BITMAP_RGBA;
	retval->bpp = 4;
	retval->w = retval->h = 8;
	retval->functionor = &dummy_bitmap_resource_functionor;
	retval->pprivate = retval + 1;

	struct cache_entry_t * ce = &retval->cache_entry;

	ce->id = retval->id;
	ce->data = retval;
	ce->sz = sizeof(retval + 8 * 8 * 4);
	ce->destroy_arg = retval;
	ce->destroy = (cache_destroy_t)dummy_destroy;
	ce->cache = NULL;
	INIT_LIST_HEAD(&ce->lru_list);
	ce->type = CACHE_ENTRY_BITMAP;
	ce->pprivate = NULL;

	return retval;
}

static void
dummy_store(struct bitmap_resource_t * name, const char * path)
{
	TRACE(BITMAP, "dummy: store bitmap to %s\n", path);
	return;
}

static void
dummy_destroy(struct bitmap_resource_t * b)
{
	assert(b->functionor->destroy ==
			dummy_destroy);
	TRACE(BITMAP, "dummy: destroy bitmap %s\n", b->id);
	struct cache_entry_t * ce = &b->cache_entry;
	xfree(ce->id);
	xfree(b);
	return;
}

static void
dummy_serialize(struct bitmap_resource_t * b, struct io_t * io)
{
	assert(b != NULL);
	assert(io != NULL);

	int bitmap_size = bitmap_data_size((struct bitmap_t*)b);
	if (b->id == NULL)
		b->id = "";
	int id_sz = strlen(b->id) + 1;

	/* dup a bitmap head */
	struct bitmap_t head;
	head = *((struct bitmap_t *)(b));
	/* see bitmap.h, the definition of bitmap_t */
	head.id = (void*)id_sz;
	head.pixels = (void*)bitmap_size;

	if (io->functionor->writev) {
		struct iovec * vecs;
		vecs = alloca(sizeof(struct iovec) * 3);
		assert(vecs != NULL);
		vecs[0].iov_base = &head;
		vecs[0].iov_len = sizeof(head);
		vecs[1].iov_base = (void*)(b->id);
		vecs[1].iov_len = id_sz;
		vecs[2].iov_base = b->pprivate;
		vecs[2].iov_len = bitmap_size;
		io_writev(io, vecs, 3);
	} else {
		io_write(io, &head, sizeof(struct bitmap_t), 1);
		io_write(io, (void*)b->id, id_sz, 1);
		io_write(io, b->pprivate, bitmap_size, 1);
	}
}


struct bitmap_resource_functionor_t dummy_bitmap_resource_functionor = {
	.name = "DummyBitmap",
	.fclass = FC_BITMAP_RESOURCE_HANDLER,
	.check_usable = dummy_check_usable,
	.destroy = dummy_destroy,
	.load = dummy_load,
	.store = dummy_store,
	.serialize = dummy_serialize,
};

// vim:ts=4:sw=4

