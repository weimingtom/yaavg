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
	struct io_t * io;
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

#define XP3_HEAD_SZ	(11)

#define TVP_XP3_INDEX_ENCODE_METHOD_MASK 0x07
#define TVP_XP3_INDEX_ENCODE_RAW      0
#define TVP_XP3_INDEX_ENCODE_ZLIB     1
#define TVP_XP3_INDEX_CONTINUE   0x80
#define TVP_XP3_FILE_PROTECTED (1<<31)
#define TVP_XP3_SEGM_ENCODE_METHOD_MASK  0x07
#define TVP_XP3_SEGM_ENCODE_RAW       0
#define TVP_XP3_SEGM_ENCODE_ZLIB      1

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


static struct xp3_package *
init_xp3_package(const char * phy_fn)
{
	struct io_t * phy_io = NULL;
	uint8_t * index_data = NULL;
	int index_size;
	struct exception_t exp;
	TRY(exp) {
		phy_io = io_open("FILE", phy_fn);
		assert(phy_io != NULL);
		check_xp3_head(phy_io);

		for (;;) {
			uint64_t index_offset = io_read_le64(phy_io);
			DEBUG(IO, "index offset=0x%Lx\n", index_offset);
			io_seek(phy_io, index_offset, SEEK_SET);

			uint8_t index_flag = io_read_byte(phy_io);
			DEBUG(IO, "index_flag = 0x%u\n", index_flag);

			uint8_t cm = index_flag & TVP_XP3_INDEX_ENCODE_METHOD_MASK;

			switch (cm) {
				case TVP_XP3_INDEX_ENCODE_ZLIB: {
					uint64_t r_compressed_size = io_read_le64(phy_io);
					uint64_t r_index_size = io_read_le64(phy_io);

					DEBUG(IO, "r_compressed_size = %Ld\n", r_compressed_size);
					DEBUG(IO, "r_index_size = %Ld\n", r_index_size);
					if ((r_compressed_size > 0x7fffffff) || (r_index_size > 0x7fffffff))
						THROW(EXP_BAD_RESOURCE, "r_compressed_size or r_index_size too large(%Lu, %Lu) in "
								"file %s\n",
								r_compressed_size, r_index_size, phy_io->id);

					index_size = r_index_size;
					int compressed_size = r_compressed_size;

					index_data = xmalloc(index_size);
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
						THROW(EXP_BAD_RESOURCE, "uncompress file %s failed: result=%d",
								phy_io->id, result);
					if ((int)dest_len != index_size)
						THROW(EXP_BAD_RESOURCE, "uncompress file %s failed: dest_len=%lu, index_size=%d",
								phy_io->id, dest_len, index_size);
				}
					break;
				case TVP_XP3_INDEX_ENCODE_RAW: {
					uint64_t r_index_size = io_read_le64(phy_io);
					if (r_index_size > 0x7fffffff)
						THROW(EXP_BAD_RESOURCE, "too large index %Lu in file %s",
								r_index_size, phy_io->id);

					index_size = r_index_size;
					DEBUG(IO, "index_size=%d\n", index_size);
					index_data = xmalloc(index_size);
					assert(index_data != NULL);
					io_read_force(phy_io, index_data, index_size);
				}
					break;
				default:
					THROW(EXP_BAD_RESOURCE, "compress method of file %s unknown",
							phy_io->id);
			}

			/* read index information from memory */
			unsigned int ch_file_start = 0;
			unsigned int ch_file_size = index_size;
#if 0
			for(;;) {
				
			}
#endif
			break;
		}
	} FINALLY {
	} CATCH(exp) {
		if (phy_io != NULL)
			io_close(phy_io);
		if (index_data != NULL)
			xfree(index_data);
		RETHROW(exp);
	}
	return NULL;
}


static struct cache_t xp3_package_cache;
static struct cache_t xp3_file_cache;

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
	if (ce == NULL) {
		struct xp3_package * xp = init_xp3_package(r->phy_fn);
		assert(xp != NULL);
		cache_insert(&xp3_package_cache, &(xp->ce));
	}

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

