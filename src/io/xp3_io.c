/* 
 * xp3_io.c
 * by WN @ Jan. 31, 2009
 *
 * for kirikiri's xp3 compress format
 */

#include <config.h>
#include <common/defs.h>
#include <common/mm.h>
#include <common/debug.h>
#include <common/exception.h>
#include <common/dict.h>
#include <common/cache.h>
#include <yconf/yconf.h>

#include <io/io.h>
#include <assert.h>
#include <unistd.h>

#ifdef HAVE_ZLIB
# include <zlib.h>
# include <stdio.h>

struct xp3_io_t {
	struct io_t io;
	char * pkg_fn;
	char * fn;
	char * id;
	uint64_t file_sz;
	int64_t pos;
	char __id[0];
	/* the content of __id:
	 * pkg_fn, fn, id*/
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
	/* if at least 1 segment is compressed, the file
	 * is compressed. see xp3_file. we cannot justice this
	 * by comparing ori_sz and arch_sz, because sometime
	 * the compressed data size is larger than or equal to
	 * the original data size.
	 * */
	bool_t is_compressed;
	char * utf8_name;
	/* name_sz contain the last '\0' */
	int name_sz;
	uint32_t file_hash;
	int nr_segments;
	struct xp3_index_item_segment * segments;
	uint64_t ori_sz;	/* uncompressed size */
	uint64_t arch_sz;	/* size in archive */
	uint8_t __data[0];
};

struct xp3_package {
	struct cache_entry_t ce;
	bool_t permanent_map;
	/* this is the package file io */
	struct io_t * io;
	struct dict_t * index_dict;
	int nr_items;
	char pkg_fn[0];
};

struct xp3_file {
	/* if at least 1 segment is compressed, the file
	 * is compressed. see xp3_index_item.is_compressed */
	bool_t is_compressed;
	struct cache_entry_t ce;
	unsigned int file_sz;
	char * utf8_name;
	char * package_name;
	char * id;
	int nr_segments;
	union {
		uint8_t * data;
		struct xp3_index_item_segment * segments;
	} u;
	uint8_t __data[0];
};

static void NONE_filter(uint8_t * data ATTR_UNUSED, int sz ATTR_UNUSED,
		struct xp3_package * package ATTR_UNUSED,
		struct xp3_file * file ATTR_UNUSED, int from ATTR_UNUSED)
{
	return;
}

static void (*xp3_filter)(uint8_t * data, int sz,
		struct xp3_package * package,
		struct xp3_file * file, int from) = NONE_filter;
static void (*xp3_revert_filter)(uint8_t * data, int sz,
		struct xp3_package * package,
		struct xp3_file * file, int from) = NONE_filter;

static void
FATE_SN_style_filter(uint8_t * data, int sz,
		struct xp3_package * package ATTR_UNUSED,
		struct xp3_file * file ATTR_UNUSED, int from)
{
	/* not 0x54, no problem */
	for (int i = 0; i < sz; i++)
		data[i] = data[i] ^ 54;

	/* It seems that Fata/Stay Night xor byte of
	 * 0x13 with 0x01, and byte of 0x2ea29 with 0x3 */
	if ((from <= 0x13) && (0x13 < from + sz))
		data[(0x13 - from)] ^= 0x01;
	if ((from <= 0x2ea29) && (0x2ea29 < from + sz))
		data[(0x2ea29 - from)] ^= 0x03;
}

/* if src == NULL, only compute the utf8 len */
static int
ucs2le_to_utf8(char * dest, int dest_len,
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

static const void *
find_chunk(const void * data, const uint8_t * name,
		struct chunk_head * ph, const void * end, bool_t if_throw)
{
	assert(data != NULL);
	assert(name != NULL);
	assert(ph != NULL);

	const void * pos = data;

	do {
		*ph = *((struct chunk_head *)(pos));
		ph->chunk_sz = le64toh(ph->chunk_sz);

		if (ph->chunk_sz > 0x7fffffff)
			THROW(EXP_BAD_RESOURCE, "chunk too large: chunk_sz=%Ld",
					ph->chunk_sz);
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
	io_read_force(io, mark, XP3_HEAD_SZ);
	if (memcmp(XP3Mark, mark, XP3_HEAD_SZ) != 0)
		THROW(EXP_BAD_RESOURCE, "xp3 file %s magic mismatch",
				io->id);
	DEBUG(IO, "xp3 file %s magic matches\n", io->id);
}

static void
destroy_xp3_index_item(struct dict_entry_t * e, uintptr_t unused ATTR_UNUSED)
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

	TRACE(IO, "freeing a %d bytes item %s\n",
			GET_DICT_DATA_REAL_SZ(e->data),
			item->utf8_name);
	xfree_null(item);
	return;
}

static void
destroy_xp3_package(struct xp3_package * pkg)
{
	DEBUG(IO, "destroying xp3 package %s\n", pkg->pkg_fn);
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

static void
destroy_xp3_file(struct xp3_file * file)
{
	DEBUG(IO, "destroying xp3 file %s\n", file->id);
	xfree(file);
}

static struct cache_t xp3_package_cache;
static struct cache_t xp3_file_cache;
static struct dict_t * permanent_map_dict = NULL;

/* 
 * the purpose of extract_index is to move some code out from
 * init_xp3_package.
 */
static void *
extract_index(struct io_t * pkg_io, uint8_t * index_flag, int * pindex_sz,
		void * index_data)
{
	uint64_t index_offset = io_read_le64(pkg_io);
	DEBUG(IO, "index offset=0x%Lx\n", index_offset);
	io_seek(pkg_io, index_offset, SEEK_SET);

	*index_flag = io_read_byte(pkg_io);
	DEBUG(IO, "index_flag = 0x%x\n", *index_flag);

	uint8_t cm = *index_flag & TVP_XP3_INDEX_ENCODE_METHOD_MASK;
	int index_size;

	switch (cm) {
		case TVP_XP3_INDEX_ENCODE_ZLIB: {
			uint64_t r_compressed_size = io_read_le64(pkg_io);
			uint64_t r_index_size = io_read_le64(pkg_io);

			DEBUG(IO, "r_compressed_size = %Ld\n", r_compressed_size);
			DEBUG(IO, "r_index_size = %Ld\n", r_index_size);
			if ((r_compressed_size > 0x7fffffff) ||
					(r_index_size > 0x7fffffff))
				THROW(EXP_BAD_RESOURCE,
						"r_compressed_size or "
						"r_index_size too large(%Lu, %Lu)",
						r_compressed_size, r_index_size);

			index_size = r_index_size;
			*pindex_sz = index_size;
			int compressed_size = r_compressed_size;

			index_data = xrealloc(index_data, index_size);
			assert(index_data != NULL);
			uint8_t * compressed_data = alloca(compressed_size);
			assert(compressed_data != NULL);

			io_read_force(pkg_io, compressed_data, compressed_size);

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
				THROW(EXP_BAD_RESOURCE,
						"uncompress failed: dest_len=%lu, index_size=%d",
						dest_len, index_size);
		}
		break;
		case TVP_XP3_INDEX_ENCODE_RAW: {
			uint64_t r_index_size = io_read_le64(pkg_io);
			if (r_index_size > 0x7fffffff)
				THROW(EXP_BAD_RESOURCE, "too large index %Lu",
						r_index_size);

			index_size = r_index_size;
			DEBUG(IO, "index_size=%d\n", index_size);
			index_data = xrealloc(index_data, index_size);
			assert(index_data != NULL);
			io_read_force(pkg_io, index_data, index_size);
		}
		break;
		default:
		THROW(EXP_BAD_RESOURCE, "compress method unknown");
	}
	return index_data;
}

static bool_t
load_chunks(const void * start,
		struct chunk_head * file_h,
		struct chunk_head * info_h,
		struct chunk_head * segments_h,
		struct chunk_head * adlr_h,
		void const ** fc_start,
		void const ** info_start,
		void const ** segments_start,
		void const ** adlr_start,
		const void * index_end)
{
	*fc_start = find_chunk(start, (uint8_t*)"File",
			file_h, index_end, FALSE);
	TRACE(IO, "File chunk start at %p, size=%Ld\n",
			*fc_start, file_h->chunk_sz);
	if (*fc_start == NULL)
		return FALSE;

	/* info trunk */
	*info_start = find_chunk(*fc_start, (uint8_t*)"info",
			info_h, *fc_start + file_h->chunk_sz, TRUE);
	TRACE(IO, "info chunk start at %p, size=%Ld\n",
			*info_start, info_h->chunk_sz);

	/* segments trunk */
	*segments_start = find_chunk(*fc_start, (uint8_t*)"segm",
			segments_h, *fc_start + file_h->chunk_sz, TRUE);
	TRACE(IO, "segments chunk found at %p, size=%Ld\n",
			*segments_start, segments_h->chunk_sz);

	/* adlr chunk */
	*adlr_start = find_chunk(*fc_start, (uint8_t*)"adlr",
			adlr_h, *fc_start + file_h->chunk_sz, TRUE);
	TRACE(IO, "adlr chunk start at %p, size=%Ld\n",
			*adlr_start, adlr_h->chunk_sz);
	return TRUE;
}

static struct xp3_index_item *
build_struct_item(const void * info_start,
		const void * segments_start,
		const void * adlr_start,
		int segments_chunk_sz,
		int * item_sz)
{
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

	tole32(ih.flags);
	tole64(ih.ori_sz);
	tole64(ih.arch_sz);
	tole16(ih.name_len);

	TRACE(IO, "flags=0x%x, ori_sz=%Ld, arch_sz=%Ld, name_len=%d\n",
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

	if (segments_chunk_sz % sizeof(struct segment) != 0)
		THROW(EXP_BAD_RESOURCE, "corrupted segments chunk: size=%d",
				segments_chunk_sz);

	int nr_segments = segments_chunk_sz / sizeof(struct segment);

	/* now we can alloc dict entry */
	*item_sz = sizeof(struct xp3_index_item) + name_sz +
		sizeof(struct xp3_index_item_segment) * nr_segments + 7;

	catch_var(struct xp3_index_item *, pitem, NULL);
	define_exp(exp);
	TRY(exp) {
		set_catched_var(pitem, xcalloc(1, *item_sz));
		assert(pitem != NULL);
		pitem->utf8_name = (char*)pitem->__data;
		/* name_sz contain the last '\0' */
		pitem->segments = ALIGN_UP_PTR((void*)(&(pitem->__data[name_sz])), 8);

		pitem->name_sz = name_sz;
		pitem->ori_sz = ih.ori_sz;
		pitem->arch_sz = ih.arch_sz;
		pitem->nr_segments = nr_segments;

		pitem->file_hash = read_le32(adlr_start);
		TRACE(IO, "file_hash=0x%x\n", pitem->file_hash);
		/* item.is_compressed is used to track the compress of
		 * a file. if at least 1 segment is compressed, it
		 * is TRUE. */
		pitem->is_compressed = FALSE;

		ucs2le_to_utf8(pitem->utf8_name, name_sz,
				p_ih->ucs2le_name, ih.name_len);
		pitem->utf8_name[name_sz - 1] = '\0';

		TRACE(IO, "xp3 item name = %s\n", pitem->utf8_name);

		/* fill segments */
		uint64_t offset_in_archive = 0;

		for (int i = 0; i < nr_segments; i++) {
			struct segment s = 
				((const struct segment *)(segments_start))[i];
			tole32(s.flags);
			tole64(s.start);
			tole64(s.ori_sz);
			tole64(s.arch_sz);
			TRACE(IO, "segment %d: flags=0x%x, "
					"start=0x%Lx, ori_sz=0x%Lx, arch_sz=0x%Lx\n",
					i, s.flags, s.start, s.ori_sz, s.arch_sz);

			struct xp3_index_item_segment * pseg = &(pitem->segments[i]);
			pseg->flags = s.flags;
			pseg->start = s.start;
			pseg->ori_sz = s.ori_sz;
			pseg->arch_sz = s.arch_sz;

			uint8_t seg_cm = s.flags & TVP_XP3_INDEX_ENCODE_METHOD_MASK;
			if (seg_cm == TVP_XP3_INDEX_ENCODE_ZLIB) {
				pseg->is_compressed = TRUE;
			} else if (seg_cm == TVP_XP3_INDEX_ENCODE_RAW) {
				pseg->is_compressed = FALSE;
				if (s.ori_sz != s.arch_sz)
					THROW(EXP_BAD_RESOURCE,
							"uncompressed segment, "
							"but arch_sz (%Ld) and ori_sz (%Ld) different",
							s.arch_sz, s.ori_sz);
			} else {
				THROW(EXP_BAD_RESOURCE,
						"segment compress method unknown: 0x%x",
						seg_cm);
			}

			pitem->is_compressed = (pitem->is_compressed ||
					pseg->is_compressed);

			pseg->offset = offset_in_archive;
			offset_in_archive += pseg->ori_sz;
		}

		/* check the size of the item */
		if ((!pitem->is_compressed) && (pitem->ori_sz != pitem->arch_sz))
			THROW(EXP_BAD_RESOURCE, "item %s is uncompressed "
					"but ori_sz(%Ld) and arch_sz(%Ld) different",
					pitem->utf8_name, pitem->ori_sz, pitem->arch_sz);
	} FINALLY {
	} CATCH(exp) {
		get_catched_var(pitem);
		xfree_null(pitem);
		RETHROW(exp);
	}
	return pitem;
}

static struct xp3_package *
init_xp3_package(const char * pkg_fn)
{
	/* we define pitem here to guarantee the exception handler can free it */
	int index_size;
	uint8_t index_flag = 0;

	catch_var(struct xp3_index_item *, pitem, NULL);
	catch_var(struct xp3_package *, p_xp3_package, NULL);
	catch_var(struct dict_t *, index_dict, NULL);
	catch_var(struct io_t *, pkg_io, NULL);
	catch_var(void *, index_data, NULL);
	define_exp(exp);
	TRY(exp) {
		set_catched_var(pkg_io, io_open_proto(pkg_fn));
		assert(pkg_io != NULL);
		check_xp3_head(pkg_io);

		int nr_items = 0;
		do {
			/* extract_index realloc index_data, need to be freed in caller */
			set_catched_var(index_data,
					extract_index(pkg_io, &index_flag, &index_size,
						index_data));

#if 0
			FILE* xxfp = fopen("/tmp/xxx", "wb");
			fwrite(index_data, index_size, 1, xxfp);
			fclose(xxfp);
#endif

			/* read index information from memory */
			const void * pindex = index_data;
			const void * index_end = index_data + index_size;
			while(pindex < index_end) {
				/* load trunks */
				struct chunk_head file_h;
				struct chunk_head info_h;
				struct chunk_head segments_h;
				struct chunk_head adlr_h;

				const void * fc_start;
				const void * info_start;
				const void * segments_start;
				const void * adlr_start;

				if (!load_chunks(pindex,
							&file_h,
							&info_h,
							&segments_h,
							&adlr_h,
							&fc_start,
							&info_start,
							&segments_start,
							&adlr_start,
							index_end)) {
					WARNING(IO, "doesn't find file chunk in this index, "
							"xp3 file is %s\n",
							pkg_fn);
					break;
				}

				int item_sz;
				set_catched_var(pitem, build_struct_item(info_start,
						segments_start,
						adlr_start,
						segments_h.chunk_sz, &item_sz));

				/* finally we can insert index entry, using utf8_name as key */
				if (index_dict == NULL) {
					/* create it */
					set_catched_var(index_dict,
							strdict_create(8, STRDICT_FL_MAINTAIN_REAL_SZ));
					assert(index_dict != NULL);
				}
				dict_data_t data, tmpdata;
				data.ptr = pitem;
				GET_DICT_DATA_REAL_SZ(data) = item_sz;
				tmpdata = strdict_insert(index_dict, pitem->utf8_name, data);
				if (!(DICT_DATA_NULL(tmpdata))) {
					WARNING(IO, "xp3 file %s has duplicate index entry %s\n",
							pkg_io->id, pitem->utf8_name);
					xfree_null(tmpdata.ptr);
				}

				/* after we insert pitem, we must reset it to NULL to prevent
				 * the exception handler xfree it twice
				 * (once in freeing the dict) */
				set_catched_var(pitem, NULL);
				pindex = fc_start + file_h.chunk_sz; 
				nr_items ++;
			}
			xfree_null(index_data);
			set_catched_var(index_data, NULL);
		} while (index_flag & TVP_XP3_INDEX_CONTINUE);

		if (index_dict == NULL)
			THROW(EXP_BAD_RESOURCE, "xp3 file doesn't contain index");

		/* now create cache entry of xp3 package */
		/* it is possible that pkg_io->id is diferent from
		 * pkg_fn. we have to save it. */
		set_catched_var(p_xp3_package, xcalloc(1, sizeof(*p_xp3_package) +
				strlen(pkg_fn) + 1));
		assert(p_xp3_package != NULL);

		strcpy(p_xp3_package->pkg_fn, pkg_fn);
		p_xp3_package->nr_items = nr_items;
		if (nr_items <= 0) {
			assert(index_dict == NULL);
			WARNING(IO, "XP3 package %s doesn't contain any item\n", pkg_fn);
		} else {
			assert(index_dict != NULL);
		}

		p_xp3_package->io = pkg_io;
		p_xp3_package->index_dict = index_dict;

		int total_sz = sizeof(*p_xp3_package);
		if (index_dict != NULL)
			total_sz += index_dict->real_data_sz;
		DEBUG(IO, "size of total index=%d\n", total_sz);

		/* build cache entry */
		struct cache_entry_t * ce = &p_xp3_package->ce;
		ce->id = p_xp3_package->pkg_fn;
		ce->data = p_xp3_package;
		ce->sz = total_sz;
		ce->destroy_arg = p_xp3_package;
		ce->destroy = (cache_destroy_t)(destroy_xp3_package);
		ce->cache = NULL;
		ce->pprivate = NULL;

		/* check permanent_map_dict th find whether to permanent_map */
		if (permanent_map_dict != NULL) {
			dict_data_t data = strdict_get(permanent_map_dict,
						p_xp3_package->io->id);
			if ((!(DICT_DATA_NULL(data))) && (data.bol))
			{
				WARNING(IO, "xp3 package file %s is permanent mapped, "
						"this is not recommended\n",
						p_xp3_package->io->id);
				io_command(p_xp3_package->io,
						"permanentmap", NULL);
			}
		}

	} FINALLY {
		get_catched_var(index_data);
		get_catched_var(pitem);
		xfree_null_catched(index_data);
	} CATCH(exp) {
		get_catched_var(pkg_io);
		get_catched_var(index_dict);
		get_catched_var(p_xp3_package);
		get_catched_var(pitem);

		if (pkg_io != NULL)
			io_close(pkg_io);
		pkg_io = NULL;
		if (index_dict != NULL)
			dict_destroy(index_dict,
					destroy_xp3_index_item, 0);
		index_dict = NULL;
		xfree_null_catched(p_xp3_package);
		xfree_null_catched(pitem);
		switch (exp.type) {
			case EXP_BAD_RESOURCE:
				THROW(EXP_BAD_RESOURCE, "corrupted xp3 file %s", pkg_fn);
				break;
			default:
				RETHROW(exp);
		}
	}

	return p_xp3_package;
}

static struct xp3_package *
get_xp3_package(const char * pkg_fn)
{
	struct cache_entry_t * ce =
		cache_get_entry(&xp3_package_cache, pkg_fn);
	struct xp3_package * xp3 = NULL;
	if (ce == NULL) {
		xp3 = init_xp3_package(pkg_fn);
		assert(xp3 != NULL);
		cache_insert(&xp3_package_cache, &(xp3->ce));
	} else {
		assert(ce->data != NULL);
		xp3 = ce->data;
	}
	return xp3;
}



/* ********************************************** */

static struct xp3_file *
init_xp3_file(const char * __id)
{
	char * id = strdupa(__id);
	char * fn = id;
	assert(*fn != '\0');

	char * pkg_fn = split_name(fn);
	assert(pkg_fn != NULL);
	assert(*pkg_fn != '\0');


	int fn_sz = strlen(fn) + 1;
	int pkg_fn_sz = strlen(pkg_fn) + 1;
	int id_sz = fn_sz + pkg_fn_sz;
	assert(id_sz == (int)(strlen(__id) + 1));

	TRACE(IO, "init xp3 file %s:%s\n", pkg_fn, fn);
	struct xp3_package * xp3 = get_xp3_package(pkg_fn);
	assert(xp3 != NULL);
	assert(xp3->index_dict != NULL);
	assert(xp3->io != NULL);

	catch_var(void *, tmp_storage, NULL);
	catch_var(struct xp3_file *, file, NULL);
	define_exp(exp);
	TRY(exp) {

		/* search from index */
		dict_data_t dd = strdict_get(
				xp3->index_dict,
				fn);
		if (GET_DICT_DATA_FLAGS(dd) & DICT_DATA_FL_VANISHED)
			THROW(EXP_RESOURCE_NOT_FOUND, "resource %s in xp3 package %s "
					"not found",
					fn, pkg_fn);
		struct xp3_index_item * item = dd.ptr;
		assert(item != NULL);

		/* if the item is uncompressed, we needn't load all data now */
		int total_sz;
		if (item->is_compressed) {
			TRACE(IO, "xp3 item %s is compressed\n", fn);
			total_sz = sizeof(*file) + pkg_fn_sz + fn_sz
				+ id_sz + item->ori_sz + 7;
		} else {
			/* if uncompressed, we only copy the segments description */
			TRACE(IO, "xp3 item %s is uncompressed\n", fn);
			total_sz = sizeof(*file) + pkg_fn_sz + fn_sz  + id_sz + 
				item->nr_segments * sizeof(struct xp3_index_item_segment) + 7;
		}
		set_catched_var(file, xmalloc(total_sz));
		assert(file != NULL);

		file->package_name = (char*)file->__data;
		file->utf8_name = file->package_name + pkg_fn_sz;
		file->id = file->utf8_name + fn_sz;
		file->is_compressed = item->is_compressed;
		file->nr_segments = item->nr_segments;
		file->file_sz = item->ori_sz;
		if (file->file_sz > (1UL << 31) - 1)
			THROW(EXP_BAD_RESOURCE, "xp3 item too large: %d\n",
					file->file_sz);

		if (file->is_compressed)
			file->u.data = ALIGN_UP_PTR((void*)(file->id + id_sz), 8);
		else
			file->u.segments  = ALIGN_UP_PTR((void*)(file->id + id_sz), 8);

		strcpy(file->package_name, pkg_fn);
		strcpy(file->utf8_name, fn);
		strcpy(file->id, __id);

		TRACE(IO, "file %s, ori_sz=%Ld, arch_sz=%Ld, is_compressed=%d\n",
				fn, item->ori_sz, item->arch_sz, file->is_compressed);

		/* copy the data */
		if (!file->is_compressed) {
			/* only copy segments description */
			memcpy(file->u.segments, item->segments,
					item->nr_segments * sizeof(struct xp3_index_item_segment));
		} else {
			uint64_t total_ori_sz = 0;
			uint64_t total_arch_sz = 0;
			for (int i = 0; i < item->nr_segments; i++) {
				struct xp3_index_item_segment * seg = &item->segments[i];
				TRACE(IO, "segment %d: ori_sz=%Ld, arch_sz=%Ld, "
						"start=0x%Lx, offset=0x%Lx\n",
						i, seg->ori_sz, seg->arch_sz, seg->start, seg->offset);

				if (seg->is_compressed) {
					TRACE(IO, "seg %d is compressed\n", i);
					set_catched_var(tmp_storage,
							xrealloc(tmp_storage, seg->arch_sz));
					assert(tmp_storage != NULL);
					/* read tmp_storage */
					io_seek(xp3->io, seg->start, SEEK_SET);
					io_read_force(xp3->io, tmp_storage, seg->arch_sz);

					/* uncompress */
					void * dest = file->u.data + seg->offset;
					unsigned long dest_len = item->ori_sz - seg->offset;
					int result = uncompress(
							dest,
							&dest_len,
							tmp_storage,
							seg->arch_sz);
					DEBUG(IO, "unzip segment %d, result=%d, dest_len=%lu\n",
							i, result, dest_len);
					if ((result != Z_OK) || (dest_len != seg->ori_sz))
						THROW(EXP_BAD_RESOURCE, "uncompress failed: "
								"result=%d, dest_len=%lu, expect %Ld",
								result, dest_len, seg->ori_sz);
				} else {
					TRACE(IO, "seg %d is uncompressed\n", i);
					if (seg->arch_sz != seg->ori_sz)
						THROW(EXP_BAD_RESOURCE, "uncompressed segment, "
								"but arch_sz (%Ld) and ori_sz (%Ld) different",
								seg->arch_sz, seg->ori_sz);
					io_seek(xp3->io, seg->start, SEEK_SET);
					io_read_force(xp3->io, file->u.data +
							seg->offset, seg->ori_sz);
				}


				xp3_filter(file->u.data + seg->offset,
						seg->ori_sz, xp3, file, seg->offset);

				total_ori_sz += seg->ori_sz;
				total_arch_sz += seg->arch_sz;
			}	/* nr_segments */

			if ((total_ori_sz != item->ori_sz) ||
					(total_arch_sz != item->arch_sz))
				THROW(EXP_BAD_RESOURCE, "arch file %s in xp3 package %s "
						"is corrupted: ori_sz: (%Ld, %Ld); "
						"arch_sz: (%Ld, %Ld)",
						fn, pkg_fn,
						total_ori_sz, item->ori_sz,
						total_arch_sz, item->arch_sz);
		}	/* file->is_compressed */

		/* fill the cache entry */
		struct cache_entry_t * ce = &(file->ce);
		ce->id = file->id;
		TRACE(IO, "fill cache: id=%s\n", ce->id);
		ce->data = file;
		ce->sz = total_sz;
		ce->destroy_arg = file;
		ce->destroy = (cache_destroy_t)(destroy_xp3_file);
		ce->cache = NULL;
		ce->pprivate = NULL;
	} FINALLY {
		get_catched_var(tmp_storage);
		xfree_null_catched(tmp_storage);
	} CATCH(exp) {
		get_catched_var(file);
		xfree_null_catched(file);
		print_exception(&exp);
		THROW(EXP_BAD_RESOURCE, "error when init file %s from xp3 package %s",
				fn, pkg_fn);
	}
	return file;
}

static struct xp3_file *
get_xp3_file(const char * __id)
{
	struct cache_entry_t * ce =
		cache_get_entry(&xp3_file_cache, __id);
	struct xp3_file * file = NULL;
	if (ce == NULL) {
		file = init_xp3_file(__id);
		assert(file != NULL);
		cache_insert(&xp3_file_cache, &(file->ce));
	} else {
		assert(ce->data != NULL);
		file = ce->data;
	}
	return file;
}



/* ********************************************** */

struct io_functionor_t xp3_io_functionor;

static void
xp3_init(void)
{
	cache_init(&xp3_package_cache, "xp3 package files cache",
			conf_get_int("sys.io.xp3.idxcachesz", 0xa00000));
	cache_init(&xp3_file_cache, "xp3 files cache",
			conf_get_int("sys.io.xp3.filecachesz", 0xa00000));

	const char * filter = conf_get_string("sys.io.xp3.filter",
			NULL);

	xp3_filter = NONE_filter;
	xp3_revert_filter = NONE_filter;
	if (filter != NULL) {
		if (strcmp("FATE/Stay Night", filter) == 0) {
			VERBOSE(IO, "xp3 file io use \"FATE/Stay Night\" style filter\n");
			xp3_filter = FATE_SN_style_filter;
			xp3_revert_filter = FATE_SN_style_filter;
		}
	}
}

static void
xp3_cleanup(void)
{
	cache_destroy(&xp3_package_cache);
	cache_destroy(&xp3_file_cache);
	if (permanent_map_dict != NULL)
		strdict_destroy(permanent_map_dict);
}

static bool_t
xp3_check_usable(const char * param)
{
	assert(param != NULL);
	if (strncasecmp("XP3", param, 3) == 0)
		return TRUE;
	return FALSE;
}

/* __id should be: aaa.png|FILE:bbb.xp3 */
static struct io_t *
xp3_open(const char * __id)
{
	assert(__id != NULL);
	/* check whether the file is correct, and insert into cache */
	struct xp3_file * fp = get_xp3_file(__id);
	assert(fp != NULL);

	/* build the xp3_io_t */
	int fn_sz = strlen(fp->utf8_name) + 1;
	int pkg_name_sz = strlen(fp->package_name) + 1;
	int id_sz = strlen(__id) + 1;
	assert(id_sz == fn_sz + pkg_name_sz);

	struct xp3_io_t * r = xcalloc(1, sizeof(*r) +
			fn_sz + pkg_name_sz + id_sz);
	assert(r != NULL);

	r->pos = 0;
	r->file_sz = fp->file_sz;
	r->id = r->__id;
	r->fn = r->id + id_sz;
	r->pkg_fn = r->fn + fn_sz;

	strcpy(r->id, __id);
	strcpy(r->fn, fp->utf8_name);
	strcpy(r->pkg_fn, fp->package_name);

	r->io.functionor = &xp3_io_functionor;
	r->io.pprivate = r;
	r->io.rdwr = IO_READ;
	r->io.id = r->id;

	DEBUG(IO, "open xp3 file: pkg_fn=%s; fn=%s, id=%s\n",
			r->pkg_fn, r->fn, r->id);
	return &(r->io);
}

static int
xp3_read(struct io_t * __io, void * ptr, int size, int nr)
{
	assert(__io != NULL);

	if (ptr == NULL) {
		assert(size * nr == 0);
		return 0;
	}

	if (size * nr == 0)
		return 0;

	assert(size > 0);
	assert(nr > 0);

	struct xp3_io_t * io = container_of(__io,
			struct xp3_io_t, io);
	assert(io == __io->pprivate);

	struct xp3_file * file = get_xp3_file(io->id);
	assert(file != NULL);

	assert(io->file_sz == file->file_sz);
	assert((0 <= io->pos) && (io->pos <= (int64_t)io->file_sz));

	int i;
	if (file->is_compressed) {
		for (i = 0; i < nr; i++) {
			int s = min((uint64_t)size, io->file_sz - io->pos);
			assert(s >= 0);
			memcpy(ptr, file->u.data + io->pos,
					s);
			io->pos += s;
			ptr += s;
			if (s != size)
				return i;
		}
		return i;
	}

	/* for uncompressed file */
	struct xp3_index_item_segment * segs = file->u.segments;
	struct xp3_index_item_segment * pseg = segs;

	struct xp3_package * pkg = get_xp3_package(file->package_name);
	assert(pkg != NULL);
	struct io_t * pkg_io = pkg->io;
	assert(pkg_io != NULL);

	assert(segs != NULL);

	for (i = 0; i < nr; i++) {
		int s = min((uint64_t)size, io->file_sz - io->pos);
		assert(s >= 0);

		/* copy: from pos io->pos, sz=s */

		while (pseg->offset + pseg->ori_sz <= (uint64_t)io->pos) {
			pseg ++;
			assert(pseg - segs < file->nr_segments);
		}

		int copied = 0;
		while (copied < s) {
			int from = pseg->offset;
			if (from < io->pos)
				from = io->pos;
			int sz = io->pos + s - from;
			if ((uint64_t)sz > pseg->ori_sz)
				sz = pseg->ori_sz;

			/* copy and filter */
			io_seek(pkg_io, pseg->start + (from - pseg->offset), SEEK_SET);
			io_read_force(pkg_io, ptr, sz);
			xp3_filter(ptr, sz, pkg, file, from);
			ptr += sz;

			copied += sz;
			if (copied < s) {
				pseg ++;
				assert(pseg - segs < file->nr_segments);
			}
		}

		io->pos += s;
		if (s != size)
			break;
	}
	return i;
}

static struct xp3_file *
get_xp3_file_from_io(struct io_t * __io)
{
	assert(__io != NULL);
	struct xp3_io_t * io = container_of(__io,
			struct xp3_io_t, io);
	assert(io == __io->pprivate);

	struct xp3_file * file = get_xp3_file(io->id);
	assert(file != NULL);
	return file;
}

static void *
xp3_map_to_mem(struct io_t * __io, int from, int max_sz)
{
	struct xp3_file * file = get_xp3_file_from_io(__io);

	if ((uint64_t)(from + max_sz) > file->file_sz)
		max_sz = file->file_sz - from;
	if (file->is_compressed) {
		void * ptr = xmalloc(max_sz);
		assert(ptr != NULL);
		memcpy(ptr, file->u.data + from, max_sz);
		return ptr;
	} else if (file->nr_segments > 1) {
		void * ptr = xmalloc(max_sz);
		assert(ptr != NULL);
		int64_t save_pos = io_tell(__io);
		io_seek(__io, 0, SEEK_SET);
		io_read_force(__io, ptr, max_sz);
		io_seek(__io, save_pos, SEEK_SET);
		return ptr;
	} else {
		TRACE(IO, "doing memory map\n");
		assert(file->nr_segments == 1);
		assert(!file->is_compressed);

		/* map should start at the beginning of the segment,
		 * no matter the 'from' */
		int start = file->u.segments[0].start;

		struct xp3_package * pkg = get_xp3_package(file->package_name);
		assert(pkg != NULL);

		/* the range of a map is the whole segment, no matter the size
		 * argument */
		void * ptr = io_map_to_mem(pkg->io, start,
				file->u.segments[0].ori_sz);
		assert(ptr != NULL);

		/* IMPORTANT: if map_to_mem twice, the data in mapped memory
		 * will be filtered twice and corrupted. to prevent this problem, we:
		 * 1. notice user not to map one file twice;
		 * 2. when unmap, call revert filter
		 * */
		xp3_filter(ptr, file->u.segments[0].ori_sz, pkg, file, start);
		return ptr + from;
	}
}

static void
xp3_release_map(struct io_t * __io, void * ptr, int from, int max_sz)
{
	struct xp3_file * file = get_xp3_file_from_io(__io);

	if ((file->is_compressed) || (file->nr_segments > 1)) {
		xfree_null(ptr);
	} else {
		if ((uint64_t)(from + max_sz) > file->file_sz)
			max_sz = file->file_sz - from;
		struct xp3_package * pkg = get_xp3_package(file->package_name);
		assert(pkg != NULL);
		/* see comments in xp3_map_to_mem */

		int start = file->u.segments[0].start;

		xp3_revert_filter(ptr, file->u.segments[0].ori_sz,
				pkg, file, start);
		io_release_map(pkg->io, ptr - from, start,
				file->u.segments[0].ori_sz);
	}
}

static void *
xp3_get_internal_buffer(struct io_t * __io)
{
	struct xp3_file * file = get_xp3_file_from_io(__io);

	if (file->is_compressed) {
		lock_cache(&xp3_file_cache);
		return file->u.data;
	} else {
		return xp3_map_to_mem(__io, 0, file->file_sz);
	}
}

static void
xp3_release_internal_buffer(struct io_t * __io, void * ptr)
{
	struct xp3_file * file = get_xp3_file_from_io(__io);

	if (file->is_compressed) {
		unlock_cache(&xp3_file_cache);
	} else {
		xp3_release_map(__io, ptr, 0, file->file_sz);
	}
}

static void
xp3_close(struct io_t * __io)
{
	assert(__io != NULL);
	assert(__io->functionor == &xp3_io_functionor);
	struct xp3_io_t * r =
		(struct xp3_io_t*)(__io->pprivate);
	TRACE(IO, "close xp3 file %s\n",
			r->id);
	xfree_null(r);
}

static struct package_items_t *
xp3_get_package_items(const char * fn)
{
	/* init the pkg file */
	DEBUG(IO, "get_package_items for xp3 package %s\n", fn);
	struct xp3_package * xp3 = get_xp3_package(fn);
	assert(xp3 != NULL);

	struct dict_t * d = xp3->index_dict;
	int total_fn_sz = 0;
	int nr_fn = 0;
	struct dict_entry_t * de = NULL;
	/* iterate over each index entry, compute the size of all filename */
	de = dict_get_next(d, NULL);
	while (de != NULL) {
		struct xp3_index_item * item = de->data.ptr;
		total_fn_sz += item->name_sz;
		nr_fn ++;
		de = dict_get_next(d, de);
	}
	assert(nr_fn == xp3->nr_items);

	DEBUG(IO, "file %s contains %d files, total filename length is %d\n",
			xp3->pkg_fn, nr_fn, total_fn_sz);

	if ((nr_fn <= 0) || (total_fn_sz <= 0)) {
		THROW(EXP_BAD_RESOURCE, "xp3 package %s doesn't contain any file, "
				"or file name error: (%d, %d)\n",
				xp3->pkg_fn, nr_fn, total_fn_sz);
		return NULL;
	}

	struct package_items_t * retval = NULL;
	int total_sz = sizeof(*retval) +
		(nr_fn + 1) * sizeof(char*) +
		total_fn_sz;
	retval = xmalloc(total_sz);
	assert(retval != NULL);

	retval->table = (void*)retval->__data;
	retval->nr_items = nr_fn;
	retval->total_sz = total_sz;

	/* notice: table's type is char **, + nr_fn is actually +4*nr_fn */
	char * ptr = (char*)(&retval->table[nr_fn + 1]);
	nr_fn = 0;
	/* iterate again */
	de = dict_get_next(d, NULL);
	while (de != NULL) {
		struct xp3_index_item * item = de->data.ptr;
		memcpy(ptr, item->utf8_name, item->name_sz);

		/* copy file name */
		/* register table entry */
		retval->table[nr_fn] = ptr;
		ptr += item->name_sz;

		nr_fn ++;
		de = dict_get_next(d, de);
	}
	/* the last NULL */
	retval->table[nr_fn] = NULL;
	return retval;
}


static void *
xp3_permanentmap(const char * fn)
{
	WARNING(IO, "issuing permanentmap command, this is not recommended!!!\n");
	/* insert the fn into permanent_map_dict */
	dict_data_t data, old_data;
	data.bol = TRUE;
	if (permanent_map_dict == NULL) {
		permanent_map_dict = strdict_create(
				0, STRDICT_FL_DUPKEY);
	}
	old_data = strdict_insert(permanent_map_dict,
			fn, data);
	assert(DICT_DATA_NULL(old_data));
	return NULL;
}

/* special method for XP3 */
/* syntax of cmd:
 *	get_package_items:<xp3file>
 *		(return a table of string containing all file names in that xp3
 *		package)
 *		(the return value of get_package_items **MUST** be freed manually)
 *	permanentmap:<xp3file> permanentmap should be called before the xp3 init.
 *	if not, it takes effect after package cache cleanup and package reinit
 */
static void *
xp3_command(struct io_t * io ATTR_UNUSED, const char * cmd,
		void * arg ATTR_UNUSED)
{
	DEBUG(IO, "run command %s for xp3 io\n", cmd);
	if (strncmp("permanentmap:", cmd,
				sizeof("permanentmap:") - 1) == 0)
		return xp3_permanentmap(cmd + sizeof("permanentmap:") - 1);
	return NULL;
}

static int64_t
xp3_tell(struct io_t * __io)
{
	struct xp3_io_t * io = container_of(__io,
			struct xp3_io_t, io);
	assert(io == __io->pprivate);
	return io->pos;
}

static void
xp3_seek(struct io_t * __io, int64_t offset,
		int whence)
{
	struct xp3_io_t * io = container_of(__io,
			struct xp3_io_t, io);
	assert(io == __io->pprivate);

	int64_t pos = io->pos;
	switch (whence) {
		case SEEK_SET:
			pos = offset;
			break;
		case SEEK_CUR:
			pos += offset;
			break;
		default:
			pos = io->file_sz + offset;
			break;
	}
	if ((pos < 0) || (pos > (int64_t)io->file_sz))
		THROW(EXP_BAD_RESOURCE, "seek xp3 file %s (%Ld, %d) failed: "
				"out of range. max pos is %Ld",
				io->id, offset, whence, io->file_sz);
	io->pos = pos;
	return;
}

static int64_t
xp3_get_sz(struct io_t * __io)
{
	struct xp3_io_t * io = container_of(__io,
			struct xp3_io_t, io);
	assert(io == __io->pprivate);
	return io->file_sz;
}

struct io_functionor_t xp3_io_functionor = {
	.name = "xp3 compress file",
	.fclass = FC_IO,
	.check_usable = xp3_check_usable,
	.open = xp3_open,
	.read = xp3_read,
	.seek = xp3_seek,
	.tell = xp3_tell,
	.close = xp3_close,
	.get_sz = xp3_get_sz,
	.map_to_mem = xp3_map_to_mem,
	.release_map = xp3_release_map,
	.get_internal_buffer = xp3_get_internal_buffer,
	.release_internal_buffer = xp3_release_internal_buffer,
	.init = xp3_init,
	.cleanup = xp3_cleanup,
	.command = xp3_command,
	.get_package_items = xp3_get_package_items,
};



#else
static void
xp3_check_usable(const char * proto)
{
	return FALSE;
}

static void *
xp3_command(const char * cmd, void * arg)
{
	return NULL,
}

struct io_functionor_t xp3_io_functionor = {
	.name = "xp3 compress file",
	.fclass = FC_IO,
	.cleck_usable = xp3_check_usable,
	.commoand = xp3_command,
};

#endif

// vim:ts=4:sw=4

