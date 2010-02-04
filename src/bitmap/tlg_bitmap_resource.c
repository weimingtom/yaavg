/* 
 * tlg_bitmap_resource.c
 * by WN @ Feb. 04, 2010
 */
#include <config.h>

#include <common/debug.h>
#include <common/exception.h>
#include <common/mm.h>
#include <bitmap/bitmap.h>
#include <bitmap/bitmap_resource.h>
#include <io/io.h>

#include <string.h>
#include <assert.h>

struct tlg_bitmap_resource_t {
	struct bitmap_resource_t bitmap_resource;
	char * id;
	void * pixels;
	uint8_t __data[0];
};


static bool_t
tlg_check_usable(const char * param)
{
	assert(param != NULL);
	assert(strnlen(param, 4) >= 3);
	DEBUG(BITMAP, "%s:%s\n", __func__, param);

	if (strncmp(param, "TLG", 3) == 0)
		return TRUE;
	return FALSE;
}

struct tlg_head {
	uint8_t colors;
	uint32_t width;
	uint32_t height;
	uint32_t blockheight;
} ATTR(packed);

static struct bitmap_resource_t *
tlg_load(struct io_t * io, const char * id)
{
	assert(io != NULL);
	DEBUG(BITMAP, "loading image %s use png loader, io is %s\n",
			id, io->id);

	struct tlg_bitmap_resource_t * r = NULL;
	struct exception_t exp;
	TRY(exp) {

		unsigned char mark[12];
		io_read_force(io, mark, 11);
		if (memcmp(mark, "TLG5.0\x00raw\x1a\x00", 11) == 0) {
			
			/* this is TLG5.0 */
			TRACE(BITMAP, "this is tlg 5.0 row\n");


			struct tlg_head tlg_head;
			io_read_force(io, &tlg_head, sizeof(tlg_head));
			tole32(tlg_head.width);
			tole32(tlg_head.height);
			tole32(tlg_head.blockheight);

			int colors, width, height, blockheight;
			colors = tlg_head.colors;
			width = tlg_head.width;
			height = tlg_head.height;
			blockheight = tlg_head.blockheight;

			int blockcount = (int)((height - 1) / blockheight) + 1;

			TRACE(BITMAP, "colors is %d, width=%d, height=%d, blockcount=%d\n", colors,
					width, height, blockcount);
			if ((colors != 3) && (colors != 4))
				THROW(EXP_UNSUPPORT_FORMAT, "unsupport color type");

			io_seek(io, blockcount * sizeof(uint32_t), SEEK_CUR);

			THROW(EXP_UNSUPPORT_FORMAT, "doesn't support tlg format");
		} else {
			THROW(EXP_UNSUPPORT_FORMAT, "doesn't support tlg format");
		}
	} FINALLY {
	} CATCH(exp) {
		if (r != NULL)
			xfree(r);
		RETHROW(exp);
	}
	return &(r->bitmap_resource);
}

struct bitmap_resource_functionor_t tlg_bitmap_resource_functionor = {
	.name = "TLGBitmap",
	.fclass = FC_BITMAP_RESOURCE_HANDLER,
	.check_usable = tlg_check_usable,
	.load = tlg_load,
};

// vim:ts=4:sw=4


