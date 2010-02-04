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

static uint8_t tlg_text[4096] ATTR(aligned(4));

static void
tlg_destroy(struct resource_t * r)
{
	assert(r != NULL);
	struct tlg_bitmap_resource_t * tr = r->ptr;
	assert(tr != NULL);
	assert(&(tr->bitmap_resource.resource) == r);
	DEBUG(BITMAP, "destroying tlg bitmap %s\n", r->id);
	xfree(tr);
}

static int
decompress_slide(uint8_t * out, const uint8_t * in, int insize, int initialr)
{
	int r = initialr;
	unsigned int flags = 0;
	const uint8_t * inlim = in + insize;
	while(in < inlim) {
		if (((flags >>= 1) & 256) == 0) {
			flags = 0[in++] | 0xff00;
		}

		if (flags & 1) {
			int mpos = in[0] | ((in[1] & 0xf) << 8);
			int mlen = (in[1] & 0xf0) >> 4;
			in += 2;
			mlen += 3;
			if(mlen == 18)
				mlen += 0[in++];

			while(mlen--) {
				0[out++] = tlg_text[r++] = tlg_text[mpos++];
				mpos &= (4096 - 1);
				r &= (4096 - 1);
			}
		} else {
			unsigned char c = 0[in++];
			0[out++] = c;
			tlg_text[r++] = c;
/*			0[out++] = tlg_text[r++] = 0[in++];*/
			r &= (4096 - 1);
		}
	}
	return r;
}


static void
write_rgb_color(uint8_t *outp, const uint8_t *upper, uint8_t * const * buf, int width)
{
	int x;
	uint8_t pc[3];
	uint8_t c[3];
	pc[0] = pc[1] = pc[2] = 0;
	for(x = 0; x < width; x++) {
		c[0] = buf[0][x];
		c[1] = buf[1][x];
		c[2] = buf[2][x];
		c[0] += c[1]; c[2] += c[1];

		outp[0] = (((pc[0] += c[0]) + upper[0]) & 0xff);
		outp[1] = (((pc[1] += c[1]) + upper[1]) & 0xff);
		outp[2] = (((pc[2] += c[2]) + upper[2]) & 0xff);
		upper += 3;
		outp += 3;
	}
}


static void
write_rgba_color(uint8_t *outp, const uint8_t *upper, uint8_t * const * buf, int width)
{
	int x;
	uint8_t pc[4];
	uint8_t c[4];
	pc[0] = pc[1] = pc[2] = pc[3] = 0;
	for(x = 0; x < width; x++)
	{
		c[0] = buf[0][x];
		c[1] = buf[1][x];
		c[2] = buf[2][x];
		c[3] = buf[3][x];
		c[0] += c[1]; c[2] += c[1];

		outp[0] = (((pc[0] += c[0]) + upper[0]) & 0xff);
		outp[1] = (((pc[1] += c[1]) + upper[1]) & 0xff);
		outp[2] = (((pc[2] += c[2]) + upper[2]) & 0xff);
		outp[3] = (((pc[3] += c[3]) + upper[3]) & 0xff);
		outp += 4;
		upper += 4;
	}
}

static struct bitmap_resource_t *
tlg_load(struct io_t * io, const char * id)
{
	assert(io != NULL);
	DEBUG(BITMAP, "loading image %s use png loader, io is %s\n",
			id, io->id);

	struct tlg_bitmap_resource_t * retval = NULL;
	uint8_t * inbuf = NULL;
	uint8_t * outbuf[4] = {NULL, NULL, NULL, NULL};
	struct exception_t exp;
	TRY(exp) {
		unsigned char mark[12];
		io_read_force(io, mark, 11);
		if (memcmp(mark, "TLG5.0\x00raw\x1a\x00", 11) == 0) {

			/* this is TLG5.0 */
			TRACE(BITMAP, "this is tlg 5.0 row\n");

			struct tlg_head {
				uint8_t nr_colors;
				uint32_t width;
				uint32_t height;
				uint32_t blockheight;
			} ATTR(packed);


			struct tlg_head tlg_head;
			io_read_force(io, &tlg_head, sizeof(tlg_head));
			tole32(tlg_head.width);
			tole32(tlg_head.height);
			tole32(tlg_head.blockheight);

			int nr_colors, width, height, blockheight;
			nr_colors = tlg_head.nr_colors;
			width = tlg_head.width;
			height = tlg_head.height;
			blockheight = tlg_head.blockheight;

			int blockcount = (int)((height - 1) / blockheight) + 1;

			TRACE(BITMAP, "nr_colors is %d, width=%d, height=%d, blockcount=%d\n", nr_colors,
					width, height, blockcount);
			if ((nr_colors != 3) && (nr_colors != 4))
				THROW(EXP_UNSUPPORT_FORMAT, "unsupport color type");

			io_seek(io, blockcount * sizeof(uint32_t), SEEK_CUR);

			/* begin decompress */
			memset(tlg_text, '\0', sizeof(tlg_text));

			inbuf = xmemalign(4, blockheight * width + 10);
			assert(inbuf != NULL);
			for (int i = 0; i < nr_colors; i++) {
				outbuf[i] = xmemalign(4, blockheight * width + 10);
				assert(outbuf[i] != NULL);
			}

			int rxx = 0;

			/* alloc pixels */
			int id_sz = strlen(id) + 1;
			int total_sz = sizeof(*retval) +
				id_sz +
				width * height * nr_colors + 7;
			retval = xmalloc(total_sz);
			assert(retval != NULL);
			TRACE(BITMAP, "alloced %d data for tlg bitmap %s\n",
					total_sz, id);
			retval->id = (char*)retval->__data;
			retval->pixels = ALIGN_UP_PTR(retval->id + id_sz, 8);
			int row_sz = width * nr_colors;

			uint8_t * prevline = alloca(width * nr_colors);
			assert(prevline != NULL);
			memset(prevline, 0, width * nr_colors);

			for (int y_blk = 0; y_blk < height; y_blk += blockheight) {
				for (int c = 0; c < nr_colors; c++) {
					uint8_t flag = io_read_byte(io);
					int sz = io_read_le32(io);
					if (flag == 0) {
						io_read_force(io, inbuf, sz);
						TRACE(BITMAP, "decompress, sz=%d, rxx=%d\n", sz, rxx);
						rxx = decompress_slide(outbuf[c], inbuf, sz, rxx);
					} else {
						TRACE(BITMAP, "rawdata, sz=%d\n", sz);
						io_read_force(io, outbuf[c], sz);
					}
				}

				int y_lim = y_blk + blockheight;
				if(y_lim > height)
					y_lim = height;
				uint8_t * outbufp[4];
				for (int i = 0; i < nr_colors; i++)
					outbufp[i] = outbuf[i];

				uint8_t * current_line = prevline;
				for (int y = y_blk; y < y_lim; y++) {
					current_line = retval->pixels + y * row_sz;
					switch (nr_colors) {
						case 3:
							write_rgb_color(current_line, prevline, outbufp, width);
							outbufp[0] += width;
							outbufp[1] += width;
							outbufp[2] += width;
							break;
						case 4:
							write_rgba_color(current_line, prevline, outbufp, width);
							outbufp[0] += width;
							outbufp[1] += width;
							outbufp[2] += width;
							outbufp[3] += width;
							break;
					}
				}
				prevline = current_line;
			}

			/* fill resource */
			struct bitmap_resource_t * b = &retval->bitmap_resource;
			struct bitmap_t * h = &b->head;
			struct resource_t * r = &b->resource;

			h->revert = FALSE;
			strcpy(retval->id, id);
			h->id = retval->id;
			h->id_sz = id_sz;
			h->format = (nr_colors == 3) ? BITMAP_RGB : BITMAP_RGBA;
			h->bpp = nr_colors;
			h->w = width;
			h->h = height;
			h->pixels = retval->pixels;

			r->id = h->id;
			r->res_sz = total_sz;
			r->serialize = generic_bitmap_serialize;
			r->destroy = tlg_destroy;
			r->ptr = retval;
			r->pprivate = NULL;
		} else {
			THROW(EXP_UNSUPPORT_FORMAT, "doesn't support tlg format");
		}
	} FINALLY {
		if (inbuf != NULL)
			xfree(inbuf);
		for (int i = 0; i < 4; i++) {
			if (outbuf[i] != NULL)
				xfree(outbuf[i]);
		}
	} CATCH(exp) {
		if (retval != NULL)
			xfree(retval);
		RETHROW(exp);
	}
	return &(retval->bitmap_resource);
}

struct bitmap_resource_functionor_t tlg_bitmap_resource_functionor = {
	.name = "TLGBitmap",
	.fclass = FC_BITMAP_RESOURCE_HANDLER,
	.check_usable = tlg_check_usable,
	.load = tlg_load,
};

// vim:ts=4:sw=4


