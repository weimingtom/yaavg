/* 
 * file_io.c
 * by WN @ Dec. 13, 2009
 */

#include <common/mm.h>
#include <common/debug.h>
#include <common/exception.h>
#include <io/io.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/mman.h>

struct io_functionor_t file_io_functionor;

static struct io_t *
file_open(const char * path, const char * mode)
{
	assert(path != NULL);
	FILE * fp = fopen(path, mode);
	if (fp == NULL)
		THROW(EXP_RESOURCE_NOT_FOUND, "open file \"%s\" using \"%s\" failed",
				path, mode);
	struct io_t * r = xcalloc(1, sizeof(*r) + strlen(path) + 1);
	assert(r != NULL);
	r->functionor = &file_io_functionor;
	r->pprivate = fp;
	strcpy(r->__data, path);
	r->id = r->__data;
	return r;
}

static struct io_t *
file_read_open(const char * path)
{
	struct io_t * r = file_open(path, "rb");
	if (r == NULL)
		return NULL;
	r->rdwr = IO_READ;
	return r;
}

static struct io_t *
file_write_open(const char * path)
{
	struct io_t * r = file_open(path, "wb");
	if (r == NULL)
		return NULL;
	r->rdwr = IO_WRITE;
	return r;
}

static int
file_read(struct io_t * io, void * ptr,
		int size, int nr)
{
	assert(io != NULL);
	assert(io->functionor);
	assert(io->functionor->read == file_read);
	assert(io->rdwr = IO_READ);

	FILE * fp = io->pprivate;
	assert(fp != NULL);
	int ret = fread(ptr, size, nr, fp);
	if (ret < 0)
		THROW(EXP_BAD_RESOURCE, "read(%d, %d) file %p return %d\n",
				size, nr, fp, ret);
	return ret;
}

static int
file_write(struct io_t * io, void * ptr,
		int size, int nr)
{
	assert(io != NULL);
	assert(io->functionor);
	assert(io->functionor->write == file_write);
	assert(io->rdwr = IO_WRITE);

	FILE * fp = io->pprivate;
	assert(fp != NULL);
	return fwrite(ptr, size, nr, fp);
}

static int
file_seek(struct io_t * io, int64_t offset,
		int whence)
{
	assert(io != NULL);
	assert(io->functionor);
	assert(io->functionor->seek == file_seek);
	FILE * fp = io->pprivate;
	assert(fp != NULL);
	int ret = fseeko64(fp, offset, whence);
	if (ret < 0)
		THROW(EXP_BAD_RESOURCE, "seek(%Ld, %d) file %p return %d\n",
				offset, whence, fp, ret);
	return ret;
}

static int64_t
file_tell(struct io_t * io)
{
	assert(io != NULL);
	assert(io->functionor);
	assert(io->functionor->tell == file_tell);

	FILE * fp = io->pprivate;
	assert(fp != NULL);

	int64_t ret = ftello64(fp);
	if (ret < 0)
		THROW(EXP_BAD_RESOURCE, "tell file %p return %Ld\n",
				fp, ret);
	return ret;
}

static void
file_close(struct io_t * io)
{
	assert(io != NULL);
	assert(io->functionor);
	assert(io->functionor->close == file_close);
	FILE * fp = io->pprivate;
	assert(fp != NULL);
	fclose(fp);
	xfree(io);
}

static void *
file_map_to_mem(struct io_t * io, int from, int max_sz)
{
	
	assert(io != NULL);
	assert(io->functionor);
	assert(io->functionor->map_to_mem == file_map_to_mem);
	FILE * fp = io->pprivate;

	int fd = fileno(fp);
	TRACE(IO, "memory mapping file %d from %d, max_sz = %d\n",
			fd, from, max_sz);

	void * ptr = mmap(NULL, max_sz, PROT_READ | PROT_WRITE,
			MAP_PRIVATE, fd, from);
	if (ptr == NULL)
		THROW(EXP_UNCATCHABLE, "mmap file %d from %d sz %d failed",
				fd, from, max_sz);

	return ptr;
}

static void
file_release_map(struct io_t * io, void * ptr, int len)
{
	assert(io != NULL);
	assert(io->functionor);
	assert(io->functionor->release_map == file_release_map);

	FILE * fp = io->pprivate;

	int fd = fileno(fp);
	
	TRACE(IO, "munmap file %d len %d\n",
			fd, len);
	int err = munmap(ptr, len);
	if (err != 0)
		THROW(EXP_UNCATCHABLE, "munmap(%p, %d) error: %d",
				ptr, len);
	return;
}


static bool_t
file_check_usable(const char * param)
{
	assert(param != NULL);
	if (strncasecmp("file", param, 4) == 0)
		return TRUE;
	return FALSE;
}

static int64_t
file_get_sz(struct io_t * io)
{
	assert(io != NULL);
	assert(io->functionor);
	assert(io->functionor->get_sz == file_get_sz);

	int err;
	struct stat64 buf;
	err = stat64(io->id, &buf);
	if (err < 0)
		THROW(EXP_BAD_RESOURCE, "resource %s stat64 failed with retval %d",
				io->id, err);
	return buf.st_size;
}

struct io_functionor_t file_io_functionor = {
	.name = "libc file",
	.inited = TRUE,
	.fclass = FC_IO,
	.check_usable = file_check_usable,
	.open = file_read_open,
	.open_write = file_write_open,
	.read = file_read,
	.write = file_write,
	.seek = file_seek,
	.tell = file_tell,
	.close = file_close,
	.map_to_mem = file_map_to_mem,
	.release_map = file_release_map,
	.get_sz = file_get_sz,
};

// vim:ts=4:sw=4

