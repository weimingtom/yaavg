/* 
 * bitmap.c
 * by WN @ Dec. 13, 2009
 */

#include <config.h>
#include <common/debug.h>
#include <common/mm.h>
#include <bitmap/bitmap.h>
#include <common/bithacks.h>
/* END_DESERIALIZE_SYNC and BEGIN_SERIALIZE_SYNC */
#include <bitmap/bitmap_resource.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <aio.h>


/* align = 0 means keep old align settings */
static struct bitmap_t *
alloc_bitmap(struct bitmap_t * phead, int id_sz, int align)
{
	int new_pitch;
	assert(align <= PIXELS_ALIGN);
	if (align != 0) {
		assert(is_power_of_2(align));
		new_pitch = ALIGN_UP(phead->w * phead->bpp, align);
	} else {
		new_pitch = phead->pitch;
	}

	assert(new_pitch >= phead->w * phead->bpp);

	int total_sz = sizeof(*phead) +
		id_sz +
		new_pitch * phead->h +
		PIXELS_ALIGN - 1;
	/* for the deserializer */
	if (phead->pitch > new_pitch)
		total_sz += phead->pitch - new_pitch;

	struct bitmap_t * res = xmalloc(total_sz);
	assert(res != NULL);
	*res = *phead;
	res->id = (char *)(res->__data);
	res->pitch = new_pitch;
	/* ref_count is maintained by caller,
	 * bitmap.c doesn't care about it */
	res->ref_count = 0;
	res->pixels = PIXELS_PTR(res->__data + id_sz);
	res->destroy_bitmap = NULL;
	return res;
}

void
free_bitmap(struct bitmap_t * ptr)
{
	TRACE(BITMAP, "freeing bitmap %p\n", ptr);
	if (ptr->destroy_bitmap == NULL) {
		xfree(ptr);
		return;
	}

	ptr->destroy_bitmap(ptr);
	return;
}

struct bitmap_t *
bitmap_deserialize(struct io_t * io, struct bitmap_deserlize_param * p)
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

	DEBUG(BITMAP, "read head, w=%d, h=%d, bpp=%d, id_sz=%d, pitch=%d\n",
			head.w, head.h, head.bpp, head.id_sz, head.pitch);
	assert(head.pitch >= head.w * head.bpp);

	assert(data_sz > 0);


	struct bitmap_t * r = NULL;
	if (p == NULL)
		r = alloc_bitmap(&head, head.id_sz, 0);
	else
		r = alloc_bitmap(&head, head.id_sz, p->align);

	assert(r != NULL);

	/* read id */
	io_read(io, r->id, head.id_sz, 1);
	DEBUG(BITMAP, "read id: %s\n", r->id);
	DEBUG(BITMAP, "old_pitch: %d, new_pitch: %d\n",
			head.pitch, r->pitch);

	/* read pixels */
	if (head.pitch == r->pitch) {
		if (io->functionor->vmsplice_read) {
			struct iovec vec;
			vec.iov_base = r->pixels;
			vec.iov_len = data_sz;
			io_vmsplice_read(io, &vec, 1);
		} else {
			io_read(io, r->pixels, data_sz, 1);
		}
	} else {
		WARNING(BITMAP, "FIXME: I haven't tested this code\n");
		for (int i = 0; i < r->h; i++) {
			/* each time we increace new pitch, but load old pitch.
			 * if new pitch is larger, some byte is never written.
			 * if old pitch is larger, the padding data is lost.
			 * the alloc_bitmap will alloc some more bytes if old pitch
			 * is larger, so below code never cause memory corruption
			 * */
			void * ptr = r->pixels + r->pitch * i;
			if (io->functionor->vmsplice_read) {
				struct iovec vec;
				vec.iov_base = ptr;
				/* load all original line */
				vec.iov_len = head.pitch;
				io_vmsplice_read(io, &vec, 1);
			} else {
				io_read(io, ptr, head.pitch, 1);
			}
		}
	}
	/* write the sync */
	sync = END_DESERIALIZE_SYNC;
	io_write(io, &sync, sizeof(sync), 1);
	DEBUG(BITMAP, "send sync mark\n");
	return r;
}

// vim:ts=4:sw=4

