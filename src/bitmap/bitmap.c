/* 
 * bitmap.c
 * by WN @ Dec. 13, 2009
 */

#include <config.h>
#include <common/debug.h>
#include <common/mm.h>
#include <bitmap/bitmap.h>
/* END_DESERIALIZE_SYNC and BEGIN_SERIALIZE_SYNC */
#include <bitmap/bitmap_resource.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <aio.h>


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

	DEBUG(BITMAP, "bitmap deserializing from io \"%s\"\n",
			io->functionor->name);

	/* read sync */
	int sync = 0;
	io_read(io, &sync, sizeof(sync), 1);
	assert(sync == BEGIN_SERIALIZE_SYNC);
	
	/* read head */
	io_read(io, &head, sizeof(head), 1);
	int data_sz = bitmap_data_size(&head); 

	DEBUG(BITMAP, "read head, w=%d, h=%d, bpp=%d, id_sz=%d\n",
			head.w, head.h, head.bpp, head.id_sz);

	assert(data_sz > 0);

	struct bitmap_t * r = alloc_bitmap(&head, head.id_sz);
	assert(r != NULL);

	/* read id */
	io_read(io, r->id, head.id_sz, 1);
	DEBUG(BITMAP, "read id: %s\n", r->id);

	/* read pixels */
	if (io->functionor->vmsplice_read) {
		struct iovec vec;
		vec.iov_base = r->pixels;
		vec.iov_len = data_sz;
		io_vmsplice_read(io, &vec, 1);
	} else {
		io_read(io, r->pixels, data_sz, 1);
	}
	/* write the sync */
	sync = END_DESERIALIZE_SYNC;
	io_write(io, &sync, sizeof(sync), 1);
	DEBUG(BITMAP, "send sync mark\n");
	return r;
}

// vim:ts=4:sw=4

