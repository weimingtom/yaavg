/* 
 * xp3_io.c
 * by WN @ Jan. 31, 2009
 *
 * for kirikiri's xp3 compress format
 */

#include <config.h>
#include <common/mm.h>
#include <common/debug.h>
#include <common/exception.h>
#include <common/dict.h>
#include <common/cache.h>
#include <yconf/yconf.h>

#include <io/io.h>
#include <assert.h>

#ifdef HAVE_ZLIB
# include <zlib.h>
# include <stdio.h>

struct xp3_package {
	struct cache_entry_t ce;
	/* this is the physic file io */
	struct io_t * io;
	struct dict_t * index_dict;
	int sz;
	char phy_fn[0];
};

struct xp3_file {
	struct cache_entry_t ce;
	size_t sz;
	uint8_t __data[0];
};

struct xp3_io_t {
	struct io_t io;
	int pos;
	char * phy_fn;
	char * fn;
	char __id[0];
};

struct xp3_index_item_segment {
	bool_t is_compressed;
	uint32_t flags;
	uint64_t start;
	uint64_t offset;
	uint64_t ori_sz;
	uint64_t arch_sz;
};

struct xp3_index_item {
	uint8_t * utf8_name;
	/* name_sz contain the last '\0' */
	int name_sz;
	uint32_t file_hash;
	int nr_segments;
	struct xp3_index_item_segment * segments;
	uint64_t ori_sz;	/* uncompressed size */
	uint64_t arch_sz;	/* size in archive */
	uint8_t __data[0];
};

/* if src == NULL, only compute the utf8 len */
static int
ucs2le_to_utf8(uint8_t * dest, int dest_len,
		const uint16_t * src, int nr_chars)
{
	int len = 0;
	const uint16_t * pos = src;
	while (pos < src + nr_chars) {
		uint16_t c = le32toh(*pos);
		pos ++;
		if (c < 0x80) {
			/* one byte utf-8 code */
			if (len + 1 <= dest_len) {
				*dest = (uint8_t)(c & 0xff);
				dest += 1;
			}
			len += 1;
			continue;
		} else if (c < 0x800) {
			/* two bytes utf-8 code */
			if (len + 2 <= dest_len) {
				dest[0] = (c >> 6) | 0xc0;
				dest[1] = (c & 0x3f) | 0x80;
				dest += 2;
			}
			len += 2;
			continue;
		} else {
			/* three bytes utf-8 code */
			if (len + 3 <= dest_len) {
				dest[0] = (c >> 12) | 0xe0;
				dest[1] = ((c & 0x0fc0) >> 6) | 0x80;
				dest[2] = (c & 0x3f) | 0x80;
				dest += 3;
			}
			len += 3;
			continue;
		}
	}
	return len;
}



#define TVP_XP3_INDEX_ENCODE_METHOD_MASK 0x07
#define TVP_XP3_INDEX_ENCODE_RAW      0
#define TVP_XP3_INDEX_ENCODE_ZLIB     1
#define TVP_XP3_INDEX_CONTINUE   0x80
#define TVP_XP3_FILE_PROTECTED (1<<31)
#define TVP_XP3_SEGM_ENCODE_METHOD_MASK  0x07
#define TVP_XP3_SEGM_ENCODE_RAW       0
#define TVP_XP3_SEGM_ENCODE_ZLIB      1

struct chunk_head {
	uint8_t name[4];
	uint64_t chunk_sz;
} ATTR(packed);

static const uint8_t *
find_chunk(const uint8_t * data, const uint8_t * name,
		struct chunk_head * ph, const uint8_t * end, bool_t if_throw)
{
	assert(data != NULL);
	assert(name != NULL);
	assert(ph != NULL);

	const uint8_t * pos = data;

	do {
		*ph = *((struct chunk_head *)(pos));
		ph->chunk_sz = le64toh(ph->chunk_sz);

		if (ph->chunk_sz > 0x7fffffff)
			THROW(EXP_BAD_RESOURCE, "chunk too large: chunk_sz=%Ld", ph->chunk_sz);
		pos += sizeof(*ph);

		if (memcmp(ph->name, name, 4) == 0)
			return pos;
		pos += ph->chunk_sz;
	} while (pos < end);
	if (if_throw)
		THROW(EXP_BAD_RESOURCE, "unable to find %s chunk",
				name);
	return NULL;
}

#define XP3_HEAD_SZ	(11)
/* return the flags */
static void
check_xp3_head(struct io_t * io)
{
	static uint8_t XP3Mark[] =
		{ 0x58/*'X'*/, 0x50/*'P'*/, 0x33/*'3'*/, 0x0d/*'\r'*/,
		  0x0a/*'\n'*/, 0x20/*' '*/, 0x0a/*'\n'*/, 0x1a/*EOF*/,
		  /* mark2 */
		  0x8b, 0x67, 0x01};
	uint8_t mark[XP3_HEAD_SZ+1];
	int err;
	err = io_read(io, mark, XP3_HEAD_SZ, 1);
	assert(err == 1);
	if (memcmp(XP3Mark, mark, XP3_HEAD_SZ) != 0)
		THROW(EXP_BAD_RESOURCE, "xp3 file %s magic mismatch",
				io->id);
	DEBUG(IO, "xp3 file %s magic matches\n", io->id);
}

static void
destroy_xp3_index_item(struct dict_entry_t * e, uintptr_t unused)
{
	if (e == NULL) {
		WARNING(IO, "received NULL dict entry here\n");
		return;
	}
	struct xp3_index_item * item = e->data.ptr;
	if (item == NULL) {
		WARNING(IO, "why item is NULL?\n");
		return;
	}

	DEBUG(IO, "freeing a %d bytes item %s\n",
			GET_DICT_DATA_REAL_SZ(e->data),
			item->utf8_name);
	xfree(item);
	return;
}

static void
destroy_xp3_package(struct xp3_package * pkg)
{
	if (pkg == NULL) {
		WARNING(IO, "shouldn't receive NULL here\n");
		return;
	}
	io_close(pkg->io);
	if (pkg->index_dict != NULL)
		dict_destroy(pkg->index_dict,
				destroy_xp3_index_item, 0);
	xfree(pkg);
}

static struct cache_t xp3_package_cache;
static struct cache_t xp3_file_cache;

static struct xp3_package *
init_xp3_package(const char * phy_fn)
{
	struct io_t * phy_io = NULL;
	uint8_t * index_data = NULL;
	struct xp3_package * p_xp3_package;
	struct dict_t * index_dict = NULL;
	int index_size;
	uint8_t index_flag = 0;
	struct exception_t exp;
	TRY(exp) {
		phy_io = io_open("FILE", phy_fn);
		assert(phy_io != NULL);
		check_xp3_head(phy_io);

		do {
			uint64_t index_offset = io_read_le64(phy_io);
			DEBUG(IO, "index offset=0x%Lx\n", index_offset);
			io_seek(phy_io, index_offset, SEEK_SET);

			index_flag = io_read_byte(phy_io);
			DEBUG(IO, "index_flag = 0x%x\n", index_flag);

			uint8_t cm = index_flag & TVP_XP3_INDEX_ENCODE_METHOD_MASK;

			switch (cm) {
				case TVP_XP3_INDEX_ENCODE_ZLIB: {
					uint64_t r_compressed_size = io_read_le64(phy_io);
					uint64_t r_index_size = io_read_le64(phy_io);

					DEBUG(IO, "r_compressed_size = %Ld\n", r_compressed_size);
					DEBUG(IO, "r_index_size = %Ld\n", r_index_size);
					if ((r_compressed_size > 0x7fffffff) || (r_index_size > 0x7fffffff))
						THROW(EXP_BAD_RESOURCE, "r_compressed_size or r_index_size too large(%Lu, %Lu)",
								r_compressed_size, r_index_size);

					index_size = r_index_size;
					int compressed_size = r_compressed_size;

					index_data = xrealloc(index_data, index_size);
					assert(index_data != NULL);
					uint8_t * compressed_data = alloca(compressed_size);
					assert(compressed_data != NULL);

					io_read_force(phy_io, compressed_data, compressed_size);

					unsigned long dest_len = index_size;
					int result = uncompress(
							index_data,
							&dest_len,
							compressed_data,
							compressed_size);
					DEBUG(IO, "unzip, result=%d, dest_len=%lu\n",
							result, dest_len);
					if (result != Z_OK)
						THROW(EXP_BAD_RESOURCE, "uncompress failed: result=%d",
								result);
					if ((int)dest_len != index_size)
						THROW(EXP_BAD_RESOURCE, "uncompress failed: dest_len=%lu, index_size=%d",
								dest_len, index_size);
				}
					break;
				case TVP_XP3_INDEX_ENCODE_RAW: {
					uint64_t r_index_size = io_read_le64(phy_io);
					if (r_index_size > 0x7fffffff)
						THROW(EXP_BAD_RESOURCE, "too large index %Lu",
								r_index_size);

					index_size = r_index_size;
					DEBUG(IO, "index_size=%d\n", index_size);
					index_data = xmalloc(index_size);
					assert(index_data != NULL);
					io_read_force(phy_io, index_data, index_size);
				}
					break;
				default:
					THROW(EXP_BAD_RESOURCE, "compress method unknown");
			}

			/* read index information from memory */
			const uint8_t * pindex = index_data;
			uint8_t * index_end = index_data + index_size;
			while(pindex < index_end) {
				/* file trunk */
				struct chunk_head file_h;
				const uint8_t * fc_start = find_chunk(pindex, (uint8_t*)"File",
						&file_h, index_end, FALSE);
				DEBUG(IO, "File chunk start at %p, size=%Ld\n",
						fc_start, file_h.chunk_sz);
				if (fc_start == NULL)
					break;

				/* info trunk */
				struct chunk_head info_h;
				const uint8_t * info_start = find_chunk(fc_start, (uint8_t*)"info",
						&info_h, fc_start + file_h.chunk_sz, TRUE);
				DEBUG(IO, "info chunk start at %p, size=%Ld\n",
						info_start, info_h.chunk_sz);

				/* segments trunk */
				struct chunk_head segments_h;
				const uint8_t * segments_start = find_chunk(fc_start, (uint8_t*)"segm",
						&segments_h, fc_start + file_h.chunk_sz, TRUE);
				DEBUG(IO, "segments chunk found at %p, size=%Ld\n",
						segments_start, segments_h.chunk_sz);

				/* adlr chunk */
				struct chunk_head adlr_h;
				const uint8_t * adlr_start = find_chunk(fc_start, (uint8_t*)"adlr",
						&adlr_h, fc_start + file_h.chunk_sz, TRUE);
				DEBUG(IO, "adlr chunk start at %p, size=%Ld\n",
						adlr_start, adlr_h.chunk_sz);

				/* read info */
				struct info_head {
					uint32_t flags;
					uint64_t ori_sz;
					uint64_t arch_sz;
					uint16_t name_len;
					uint16_t ucs2le_name[0];
				} ATTR(packed);

				const struct info_head * p_ih = (const struct info_head*)(info_start);
				struct info_head ih = *p_ih;

				ih.flags	= le32toh(ih.flags);
				ih.ori_sz	= le32toh(ih.ori_sz);
				ih.arch_sz	= le32toh(ih.arch_sz);
				ih.name_len	= le32toh(ih.name_len);

				DEBUG(IO, "flags=0x%x, ori_sz=%Ld, arch_sz=%Ld, name_len=%d\n",
						ih.flags, ih.ori_sz, ih.arch_sz, ih.name_len);

				/* +1 for the last '\0' */
				int name_sz = ucs2le_to_utf8(
						NULL,
						0,
						p_ih->ucs2le_name,
						ih.name_len) + 1;

				/* process segments */
				struct segment {
					uint32_t flags;
					uint64_t start;
					uint64_t ori_sz;
					uint64_t arch_sz;
				} ATTR(packed);

				if (segments_h.chunk_sz % sizeof(struct segment) != 0)
					THROW(EXP_BAD_RESOURCE, "corrupted segments chunk: size=%Ld",
							segments_h.chunk_sz);

				int nr_segments = segments_h.chunk_sz / sizeof(struct segment);

				/* now we can alloc dict entry */
				int item_sz = sizeof(struct xp3_index_item) + name_sz +
					sizeof(struct xp3_index_item_segment) * nr_segments + 7;
				struct xp3_index_item * pitem = xcalloc(1, item_sz);
				assert(pitem != NULL);

				pitem->utf8_name = pitem->__data;
				pitem->name_sz = name_sz;
				/* name_sz contain the last '\0' */
				pitem->segments = ALIGN_UP((void*)(&(pitem->__data[name_sz])), 8);
				pitem->ori_sz = ih.ori_sz;
				pitem->arch_sz = ih.arch_sz;
				pitem->nr_segments = nr_segments;
				pitem->file_hash = read_le32(adlr_start);

				ucs2le_to_utf8(pitem->utf8_name, name_sz,
						p_ih->ucs2le_name, ih.name_len);
				pitem->utf8_name[name_sz - 1] = '\0';

				DEBUG(IO, "xp3 item name = %s\n", pitem->utf8_name);

				/* fill segments */
				uint64_t offset_in_archive = 0;
				for (int i = 0; i < nr_segments; i++) {
					struct segment s = 
						((const struct segment *)(segments_start))[i];
					s.flags = le32toh(s.flags);
					s.start = le64toh(s.start);
					s.ori_sz = le64toh(s.ori_sz);
					s.arch_sz = le64toh(s.arch_sz);
					DEBUG(IO, "segment %d: flags=0x%x, start=0x%Lx, ori_sz=0x%Lx, arch_sz=0x%Lx\n",
							i, s.flags, s.start, s.ori_sz, s.arch_sz);

					struct xp3_index_item_segment * pos = &(pitem->segments[i]);
					pos->flags = s.flags;
					pos->start = s.start;
					pos->ori_sz = s.ori_sz;
					pos->arch_sz = s.arch_sz;
					uint8_t seg_cm = s.flags & TVP_XP3_INDEX_ENCODE_METHOD_MASK;
					if (seg_cm == TVP_XP3_INDEX_ENCODE_ZLIB) {
						pos->is_compressed = TRUE;
					} else if (seg_cm == TVP_XP3_INDEX_ENCODE_RAW) {
						pos->is_compressed = FALSE;
					} else {
						THROW(EXP_BAD_RESOURCE, "segment compress method unknown: 0x%x",
								seg_cm);
					}
					pos->offset = offset_in_archive;
					offset_in_archive += pos->ori_sz;
				}

				/* finally we can insert index entry, using utf8_name as key */
				if (index_dict == NULL) {
					/* create it */
					index_dict = strdict_create(8, STRDICT_FL_MAINTAIN_REAL_SZ);
					assert(index_dict != NULL);
				}

				dict_data_t data, tmpdata;
				data.ptr = pitem;
				GET_DICT_DATA_REAL_SZ(data) = item_sz;
				tmpdata = strdict_insert(index_dict, (char*)(pitem->utf8_name),
						data);
				if (!(GET_DICT_DATA_FLAGS(tmpdata) & DICT_DATA_FL_VANISHED)) {
					WARNING(IO, "xp3 file %s has duplicate index entry %s\n",
							phy_io->id, pitem->utf8_name);
					xfree(tmpdata.ptr);
				}
				pindex = fc_start + file_h.chunk_sz; 
			}
			xfree(index_data);
			index_data = NULL;
		} while (index_flag & TVP_XP3_INDEX_CONTINUE);

		/* now create cache entry of xp3 package */
		/* it is possible that phy_io->id is diferent from
		 * phy_fn. we have to save it. */
		p_xp3_package = xcalloc(1, sizeof(*p_xp3_package) +
				strlen(phy_fn) + 1);
		assert(p_xp3_package != NULL);

		strcpy(p_xp3_package->phy_fn, phy_fn);
		p_xp3_package->io = phy_io;
		p_xp3_package->index_dict = index_dict;
		p_xp3_package->sz = sizeof(*p_xp3_package);
		if (index_dict != NULL)
			p_xp3_package->sz += index_dict->real_data_sz;
		DEBUG(IO, "size of total index=%d\n", p_xp3_package->sz);

		/* build cache entry */
		struct cache_entry_t * ce = &p_xp3_package->ce;
		ce->id = p_xp3_package->phy_fn;
		ce->data = p_xp3_package;
		ce->sz = p_xp3_package->sz;
		ce->destroy_arg = p_xp3_package;
		ce->destroy = (cache_destroy_t)(destroy_xp3_package);
		ce->cache = NULL;
		ce->pprivate = NULL;
	} FINALLY {
		if (index_data != NULL)
			xfree(index_data);
	} CATCH(exp) {
		if (phy_io != NULL)
			io_close(phy_io);
		if (index_dict != NULL)
			dict_destroy(index_dict,
					destroy_xp3_index_item, 0);
		if (p_xp3_package != NULL)
			xfree(p_xp3_package);
		print_exception(&exp);
		switch (exp.type) {
			case EXP_BAD_RESOURCE:
				THROW(EXP_BAD_RESOURCE, "corrupted xp3 file %s", phy_fn);
				break;
			default:
				RETHROW(exp);
		}
	}
	return p_xp3_package;
}


struct io_functionor_t xp3_io_functionor;

static void
xp3_init(void)
{
	cache_init(&xp3_package_cache, "xp3 physical files cache",
			conf_get_int("sys.io.xp3.idxcachesz", 0xa00000));
	cache_init(&xp3_file_cache, "xp3 files cache",
			conf_get_int("sys.io.xp3.filecachesz", 0xa00000));
}

static void
xp3_cleanup(void)
{
	cache_destroy(&xp3_package_cache);
	cache_destroy(&xp3_file_cache);
}

static bool_t
xp3_check_usable(const char * param)
{
	assert(param != NULL);
	if (strncasecmp("XP3", param, 3) == 0)
		return TRUE;
	return FALSE;
}

static char *
_strtok(char * str, char x)
{
	assert(str != NULL);
	while ((*str != '\0') && (*str != x)) {
		str ++;
	}
	if (*str == '\0')
		return NULL;
	return str;
}

static struct io_t *
xp3_open(const char * __path)
{
	assert(__path != NULL);
	char * path = strdupa(__path);
	char * phy_fn = path;
	char * fn = _strtok(path, ':');
	assert(fn != NULL);
	*fn = '\0';
	fn ++;
	
	/* alloc xp_io_t */
	int sz_phy_fn = strlen(phy_fn) + 1;
	int sz_fn = strlen(fn) + 1;

	struct xp3_io_t * r = xcalloc(1, sizeof(*r) +
			sz_phy_fn +
			sz_fn);
	assert(r != NULL);

	r->pos = 0;
	r->phy_fn = r->__id;
	r->fn = r->__id + sz_phy_fn;

	r->io.functionor = &xp3_io_functionor;
	r->io.pprivate = r;
	r->io.rdwr = IO_READ;
	r->io.id = r->fn;


	strncpy(r->phy_fn, phy_fn, sz_phy_fn);
	strncpy(r->fn, fn, sz_fn);

	TRACE(IO, "open xp3 file: phy_fn=%s; fn=%s\n",
			r->phy_fn, r->fn);

	/* init the phy file */
	struct cache_entry_t * ce =
		cache_get_entry(&xp3_package_cache, r->phy_fn);
	struct xp3_package * xp3 = NULL;
	if (ce == NULL) {
		xp3 = init_xp3_package(r->phy_fn);
		assert(xp3 != NULL);
		cache_insert(&xp3_package_cache, &(xp3->ce));
	} else {
		assert(ce->data != NULL);
		xp3 = ce->data;
	}

	TRACE(IO, "xp3 package is %s\n", xp3->phy_fn);

	return &(r->io);
}

static void
xp3_close(struct io_t * __io)
{
	assert(__io != NULL);
	assert(__io->functionor == &xp3_io_functionor);
	struct xp3_io_t * r =
		(struct xp3_io_t*)(__io->pprivate);
	TRACE(IO, "close xp3 file %s:%s\n",
			r->phy_fn, r->fn);
	xfree(r);
}


struct io_functionor_t xp3_io_functionor = {
	.name = "xp3 compress file",
	.fclass = FC_IO,
	.check_usable = xp3_check_usable,
	.open = xp3_open,
	.close = xp3_close,
	.init = xp3_init,
	.cleanup = xp3_cleanup,
};



#else
static void
xp3_check_usable(const char * proto)
{
	return FALSE;
}

struct io_functionor_t xp3_io_functionor = {
	.name = "xp3 compress file",
	.fclass = FC_IO,
	.cleck_usable = xp3_check_usable,
};

#endif

// vim:ts=4:sw=4
