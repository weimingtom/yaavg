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
	if (align == 0)
		align = phead->align;
	if ((align != 0) && (align != 1)) {
		assert(is_power_of_2(align));
		new_pitch = ALIGN_UP(phead->w * phead->bpp, align);
	} else {
		align = 1;
		new_pitch = phead->pitch;
	}

	assert(new_pitch >= phead->w * phead->bpp);

	int total_sz = sizeof(*phead) +
		id_sz +
		new_pitch * phead->h +
		PIXELS_ALIGN - 1;
	/* for the deserializer:
	 * alloc a little more data can simplify the 
	 * data fill procedure */
	if (phead->pitch > new_pitch)
		total_sz += phead->pitch - new_pitch;

	struct bitmap_t * res = xmalloc(total_sz);
	assert(res != NULL);
	*res = *phead;
	res->id = (char *)(res->__data);
	res->pitch = new_pitch;
	res->align = align;
	res->total_sz = total_sz;
	/* ref_count is maintained by caller,
	 * bitmap.c doesn't care about it */
	res->ref_count = 0;
	res->pixels = PIXELS_PTR(res->__data + id_sz);
	res->destroy = NULL;
	return res;
}

void
free_bitmap(struct bitmap_t * ptr)
{
	TRACE(BITMAP, "freeing bitmap %p\n", ptr);
	if (ptr->destroy == NULL) {
		xfree(ptr);
		return;
	}

	ptr->destroy(ptr);
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

	DEBUG(BITMAP, "read head, x=%d, y=%d, w=%d, h=%d, bpp=%d, id_sz=%d, pitch=%d, align=%d\n",
			head.x, head.y, head.w, head.h, head.bpp, head.id_sz, head.pitch, head.align);
	assert(head.align == 1);
	assert(head.pitch == head.w * head.bpp);

	assert(data_sz > 0);

	struct bitmap_t * r = NULL;
	if (p == NULL)
		r = alloc_bitmap(&head, head.id_sz, 1);
	else
		r = alloc_bitmap(&head, head.id_sz, p->align);

	assert(r != NULL);

	/* read id */
	io_read(io, (char*)r->id, head.id_sz, 1);
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

static void
bitmap_in_array_destroy(struct bitmap_t * b)
{
	WARNING(BITMAP, "bitmap %p is part of a bitmap array and shouldn't not be destroied\n",
			b);
	return;
}

static void
bitmap_array_destroy(struct bitmap_array_t * ba)
{
	if (ba->original_bitmap)
		free_bitmap(ba->original_bitmap);
	xfree(ba);
}

static void
copy_bitmaps_pixels(struct bitmap_t * dest, struct bitmap_t * src,
		int x, int y)
{
	assert(dest != NULL);
	assert(src != NULL);
	assert((x >= 0) && (y >= 0));
	int w = dest->w;
	int h = dest->h;
	assert(x + w <= src->w);
	assert(y + h <= src->h);
	assert(src->format == dest->format);
	assert(src->bpp == dest->bpp);

	DEBUG(BITMAP, "copy: (%d, %d, %d, %d)\n",
			x, y, w, h);
	for (int j = 0; j < h; j++) {
		void * src_pix = src->pixels +
			src->pitch * (j + y) + x * src->bpp;
		void * dest_pix = dest->pixels +
			dest->pitch * j;
		int sz = w * src->bpp;
		memcpy(dest_pix, src_pix, sz);
	}
}

struct bitmap_array_t *
split_bitmap(struct bitmap_t * b, int sz_lim_w, int sz_lim_h, int align)
{
	assert(b != NULL);
	assert(sz_lim_w > 0);
	assert(sz_lim_h > 0);
	if (align == 0)
		align = 1;
	assert(is_power_of_2(align));
	DEBUG(BITMAP, "spliting bitmap %s: sz_lim is %dx%x, align=%d\n",
			b->id, sz_lim_w, sz_lim_h, align);

	/* check the original bitmap */
	if ((b->w <= sz_lim_w) && (b->h <= sz_lim_h) && (b->align == align)) {
		/* the simplest situation */
		int total_sz = sizeof(struct bitmap_array_t) +
			strlen(b->id) + 1;
		struct bitmap_array_t * r = xmalloc(total_sz);
		assert(r != NULL);
		r->original_bitmap = b;
		r->head = *b;
		r->tiles = NULL;

		/* init the mesh */

		r->id = (char*)(r->__data);
		strcpy((void*)r->id, b->id);

		r->align = align;
		r->sz_lim_w = sz_lim_w;
		r->sz_lim_h = sz_lim_h;
		r->nr_tiles = 1;
		r->nr_h = 1;
		r->nr_w = 1;
		r->total_sz = total_sz + b->total_sz;
		r->destroy = bitmap_array_destroy;
		DEBUG(BITMAP, "%s satisifies the requirements\n",
				b->id);
		return r;
	}

	/* we need to split the bitmap */
	int nr_w, nr_h, nr_total;
	nr_w = (b->w + sz_lim_w - 1) / sz_lim_w;
	nr_h = (b->h + sz_lim_h - 1) / sz_lim_h;
	nr_total = nr_w * nr_h;
	DEBUG(BITMAP, "nr_w=%d, nr_h=%d, b->align=%d, align=%d\n", nr_w, nr_h,
			b->align, align);
	assert(nr_total >= 1);
	if (nr_total == 1)
		assert(b->align != align);

	/* compute the total_sz */
	assert(b->id != NULL);
	int id_sz = strlen(b->id) + 1;


	int normal_sz =	sz_lim_w * sz_lim_h * b->bpp + align - 1;

	int right_edge_w = b->w % sz_lim_w;
	if (right_edge_w == 0)
		right_edge_w = sz_lim_w;
	int right_edge_sz =	right_edge_w * sz_lim_h * b->bpp + align - 1;

	int bottom_edge_h = b->h % sz_lim_h;
	if (bottom_edge_h == 0)
		bottom_edge_h = sz_lim_h;
	int bottom_edge_sz = sz_lim_w * bottom_edge_h * b->bpp + align - 1;

	int corner_sz = sizeof(struct bitmap_t *) +
		right_edge_w * bottom_edge_h * b->bpp + align - 1;

	/* compute the total_sz */
	int nr_normal_tiles = (nr_w - 1) * (nr_h - 1);
	int nr_bottom_edge_tiles = nr_w - 1;
	int nr_right_edge_tiles = nr_h - 1;
	DEBUG(BITMAP, "the bitmap is splitted into %d+%d+%d+1 tiles\n",
			nr_normal_tiles, nr_right_edge_tiles, nr_bottom_edge_tiles);

	int tiles_total_sz = nr_normal_tiles * normal_sz +
		nr_bottom_edge_tiles * bottom_edge_sz +
		nr_right_edge_tiles * right_edge_sz +
		corner_sz;

	int total_sz = sizeof(struct bitmap_array_t) +
		id_sz +
		nr_total * sizeof(struct bitmap_t) +
		tiles_total_sz;

	DEBUG(BITMAP, "alloc %d bytes for the array\n",
			total_sz);

	struct bitmap_array_t * r = xmalloc(total_sz);
	assert(r != NULL);
	r->destroy = bitmap_array_destroy;
	
	r->original_bitmap = NULL;


	r->id = (void*)(r->__data);
	r->align = align;
	r->sz_lim_w = sz_lim_w;
	r->sz_lim_h = sz_lim_h;
	r->nr_w = nr_w;
	r->nr_h = nr_h;
	r->nr_tiles = nr_total;
	r->total_sz = total_sz;
	r->tiles = (void*)(r->id) + id_sz;
	void * curr_pixel = &(r->tiles[nr_total + 1]);

	r->head = *b;
	r->head.id = r->id;
	r->head.pixels = curr_pixel;
	r->head.destroy = bitmap_in_array_destroy;
	strcpy((void*)r->id, b->id);


	struct bitmap_t * curr = r->tiles;
#define build_curr(_w, _h, x_start, y_start) do {	\
		*curr = r->head;			\
		curr->x = x_start;			\
		curr->y = y_start;			\
		curr->w = (_w);				\
		curr->h = (_h);				\
		curr->pitch = ALIGN_UP((_w) * b->bpp, align);	\
		curr->pixels = ALIGN_UP_PTR(curr_pixel, align); \
		copy_bitmaps_pixels(curr, b, (x_start), (y_start));	\
		curr->total_sz = curr->pitch * curr->h + sizeof(*curr) + align - 1;\
		curr_pixel = curr->pixels + curr->pitch * (_h);	\
		curr ++;		\
	} while(0)

	for (int y = 0; y < nr_h - 1; y++) {
		for (int x = 0; x < nr_w - 1; x++)  {
			build_curr(sz_lim_w, sz_lim_h, x * sz_lim_w, y * sz_lim_h);
		}
		build_curr(right_edge_w, sz_lim_h, (nr_w - 1) * sz_lim_w, y * sz_lim_h);
	}

	DEBUG(BITMAP, "normal tiles OK\n");
	/* last line */
	for (int x = 0; x < nr_w - 1; x++)
		build_curr(sz_lim_w, bottom_edge_h, x * sz_lim_w, (nr_h - 1) * sz_lim_h);

	/* the corner */
	build_curr(right_edge_w, bottom_edge_h, (nr_w - 1) * sz_lim_w, (nr_h - 1) * sz_lim_h);

	DEBUG(BITMAP, "bitmap %s split over\n", r->id);
	return r;
}

void
fill_mesh_by_array(struct bitmap_array_t * array, struct rect_mesh_t * mesh)
{
	assert(array != NULL);
	assert(mesh != NULL);
	assert(mesh->nr_w == array->nr_w);
	assert(mesh->nr_h == array->nr_h);

	trival_init_mesh_rect(&mesh->big_rect,
			array->head.w, array->head.h);
	struct bitmap_t * curr_bitmap = array->tiles;
	struct rect_mesh_tile_t * curr_tile = mesh->tiles;
	for (int y = 0; y < mesh->nr_h; y++) {
		for (int x = 0; x < mesh->nr_w; x++) {
			trival_init_mesh_rect(&(curr_tile->rect),
					curr_bitmap->w,
					curr_bitmap->h);
			curr_bitmap ++;
			curr_tile ++;
		}
	}
}

void
free_bitmap_array(struct bitmap_array_t * ba)
{
	if (ba->destroy)
		ba->destroy(ba);
	else
		xfree(ba);
}

struct bitmap_t *
clip_bitmap(struct bitmap_t * ori, struct rect_t rect, int align)
{
	assert(ori != NULL);
	assert((rect.x >= 0) && (rect.y >= 0));
	assert((rect_xbound(rect) < ori->w) && (rect_ybound(rect) < ori->h));

	/* create the bitmap */
#define clipped_id_fmt "%s-clipped-" RECT_FMT, ori->id, RECT_ARG(rect)
	int id_sz = snprintf(NULL, 0, clipped_id_fmt) + 1;
	struct bitmap_t head = *ori;
	head.x = rect.x;
	head.y = rect.y;
	head.w = rect.w;
	head.h = rect.h;
	struct bitmap_t * r = alloc_bitmap(&head, id_sz, align);
	assert(r != NULL);
	snprintf((char*)r->id, id_sz + 1, clipped_id_fmt);
#undef clipped_id_fmt
	/* an xfree is enough */
	r->destroy = NULL;

	/* copy data! */
	copy_bitmaps_pixels(r, ori, rect.x, rect.y);
	return r;
}

// vim:ts=4:sw=4

