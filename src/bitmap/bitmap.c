/* 
 * bitmap.c
 * by WN @ Dec. 13, 2009
 */

#include <config.h>
#include <common/debug.h>
#include <common/mm.h>
#include <bitmap/bitmap.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

struct bitmap_t *
alloc_bitmap(struct bitmap_t * phead, int id_sz)
{
	struct bitmap_t * res = xmalloc(sizeof(*phead) +
			id_sz +
			bitmap_data_size(phead) +
			PIXELS_ALIGN - 1);
	assert(res != NULL);
	*res = *phead;
	res->id = (char *)(res->__data);
	res->pixels = PIXELS_PTR(res->__data + id_sz);
	return res;
}

void
free_bitmap(struct bitmap_t * ptr)
{
	xfree(ptr);
}

struct bitmap_t *
bitmap_deserialize(struct io_t * io)
{
	struct bitmap_t head;
	assert(io && (io->functionor->read));

	TRACE(BITMAP, "bitmap deserializing from io \"%s\"\n",
			io->functionor->name);
	
	/* read head */
	io_read(io, &head, sizeof(head), 1);
	int data_sz = bitmap_data_size(&head); 

	TRACE(BITMAP, "read head, w=%d, h=%d, bpp=%d, id_sz=%d\n",
			head.w, head.h, head.bpp, head.id_sz);

	struct bitmap_t * r = alloc_bitmap(&head, head.id_sz);
	assert(r != NULL);

	/* read id */
	io_read(io, r->id, head.id_sz, 1);
	TRACE(BITMAP, "read id: %s\n", r->id);

	/* read pixels */
	io_read(io, r->pixels, data_sz, 1);
	return r;
}

// vim:ts=4:sw=4

