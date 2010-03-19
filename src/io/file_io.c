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
#include <unistd.h>

struct io_functionor_t file_io_functionor;

struct file_io_t {
	struct io_t io;
	bool_t permanent_mapped;
	int64_t file_sz;
	FILE * fp;
	void * map_base;
	char __data[0];
};

static const char unknown_file_path[] = "Unknown file";

static struct io_t *
file_open(const char * path, const char * mode)
{
	assert(path != NULL);
	assert(*_strtok(path, '|') == '\0');
	FILE * fp = fopen(path, mode);
	if (fp == NULL)
		THROW(EXP_RESOURCE_NOT_FOUND, "open file \"%s\" using \"%s\" failed",
				path, mode);
	struct file_io_t * r = xcalloc(1, sizeof(*r) + strlen(path) + 1);
	assert(r != NULL);
	r->io.functionor = &file_io_functionor;
	/* set pprivate to fp can make our life eaiser */
	r->io.pprivate = fp;
	strcpy(r->__data, path);
	r->io.id = r->__data;

	r->fp = fp;
	r->permanent_mapped = FALSE;
	r->map_base = (void*)(0xffffffff);
	r->file_sz = -1;

	return &(r->io);
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
		THROW_FATAL(EXP_BAD_RESOURCE, "read(%d, %d) file %p return %d\n",
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
file_flush(struct io_t * io)
{
	assert(io != NULL);
	assert(io->functionor);
	assert(io->functionor->flush == file_flush);
	FILE * fp = io->pprivate;
	assert(fp != NULL);
	int err = fflush(fp);
	if (err != 0)
		THROW(EXP_UNCATCHABLE, "flush file io %s failed: %d",
				io->id, err);
	return 0;
}

static void
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
		THROW_FATAL(EXP_BAD_RESOURCE, "seek(%Ld, %d) file %p return %d\n",
				offset, whence, fp, ret);
	return;
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
		THROW_FATAL(EXP_BAD_RESOURCE, "tell file %p return %Ld\n",
				fp, ret);
	return ret;
}

static int64_t
file_get_sz(struct io_t * io);

static void
file_close(struct io_t * io)
{
	assert(io != NULL);
	assert(io->functionor);
	assert(io->functionor->close == file_close);

	/* in fact, we can close fp first, because
	 * if permanent_mapped is true, the file_get_sz must
	 * be called at least once. more over, stat64 doesn't rely
	 * on an opened file. */
	struct file_io_t * real_io = container_of(io,
			struct file_io_t, io);
	if (real_io->permanent_mapped) {
		assert((uintptr_t)real_io->map_base < 0xc0000000);
		assert((uintptr_t)real_io->map_base % getpagesize() == 0);
		munmap(real_io->map_base, file_get_sz(io));
	}

	FILE * fp = io->pprivate;
	assert(fp != NULL);
	fclose(fp);
	xfree(real_io);
}

static void *
file_map_to_mem(struct io_t * io, int from, int max_sz)
{
	
	assert(io != NULL);
	assert(io->functionor);
	assert(io->functionor->map_to_mem == file_map_to_mem);

	struct file_io_t * real_io = container_of(io,
			struct file_io_t, io);
	if (real_io->permanent_mapped) {
		assert(real_io->map_base <= (void*)0xc0000000);
		return real_io->map_base + from;
	}

	FILE * fp = io->pprivate;
	int fd = fileno(fp);
	WARNING(IO, "memory mapping file %d from %d, max_sz = %d, this is highly not recommanded!!!\n",
			fd, from, max_sz);

	int real_offset_page = (from >> 12);
	int inner_offset = (from - (real_offset_page << 12));

	/* we don't have mmap2, why? */
#if 0
	void * ptr = mmap2(NULL, max_sz + inner_offset, PROT_READ | PROT_WRITE,
			MAP_PRIVATE, fd, real_offset_page);
#endif
	void * ptr = mmap(NULL, max_sz + inner_offset, PROT_READ | PROT_WRITE,
			MAP_PRIVATE, fd, real_offset_page << 12);
	if ((int)(ptr) == -1)
		THROW(EXP_UNCATCHABLE, "mmap file %d from %d sz %d failed: mmap return %p",
				fd, from, max_sz, ptr);

	return ptr + inner_offset;
}

static void
file_release_map(struct io_t * io, void * ptr, int from ATTR_UNUSED, int len)
{
	assert(io != NULL);
	assert(io->functionor);
	assert(io->functionor->release_map == file_release_map);

	struct file_io_t * real_io = container_of(io,
			struct file_io_t, io);
	if (real_io->permanent_mapped)
		return;

	FILE * fp = io->pprivate;

	int fd DEBUG_DEF = fileno(fp);
	TRACE(IO, "munmap file %d len %d\n",
			fd, len);
	uintptr_t inner_offset = (uintptr_t)(ptr) % 4096;
	int err = munmap(ptr - inner_offset, len + inner_offset);
	if (err != 0)
		THROW(EXP_UNCATCHABLE, "munmap(%p, %d) error: %d",
				ptr, len, err);
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

	struct file_io_t * real_io = container_of(io,
			struct file_io_t, io);
	if (real_io->file_sz != -1)
		return real_io->file_sz;
	/* for the direct created io_t */
	if (io->id == unknown_file_path) {
		WARNING(IO, "You are getting file size of a struct io_t which created directly by FILE * %p. \n",
				real_io->fp);
		int64_t pos_save = io_tell(io);
		io_seek(io, 0, SEEK_END);
		int64_t pos = io_tell(io);
		io_seek(io, pos_save, SEEK_SET);
		real_io->file_sz = pos_save;
		return pos;
	}

	int err;
	struct stat64 buf;
	err = stat64(io->id, &buf);
	if (err < 0)
		THROW_FATAL(EXP_BAD_RESOURCE, "resource %s stat64 failed with retval %d",
				io->id, err);
	real_io->file_sz = buf.st_size;
	return real_io->file_sz;
}

static void *
file_command(struct io_t * io, const char * cmd, void * arg);

static void *
file_permanentmap(struct io_t * io)
{
	assert(io != NULL);
	assert(io->functionor);
	assert(io->functionor->command == file_command);

	WARNING(IO, "issuing permanentmap command, this is not recommanded!!!\n");
	struct file_io_t * real_io = container_of(io,
			struct file_io_t, io);
	if (real_io->permanent_mapped) {
		assert(real_io->map_base != (void*)0xffffffff);
		return real_io->map_base;
	}

	int sz = file_get_sz(io);
	/* make sure the permanent_mapped flag is FALSE */
	real_io->permanent_mapped = FALSE;
	void * ptr = file_map_to_mem(io, 0, sz);
	assert(ptr < (void*)0xc0000000);
	assert((uintptr_t)ptr % getpagesize() == 0);

	real_io->permanent_mapped = TRUE;
	real_io->map_base = ptr;
	return ptr;
}

static struct io_t *
file_build_io_from_stdfile(FILE * fp, bool_t is_write)
{
	assert(fp != NULL);
	WARNING(IO, "You are building a struct io_t directly from FILE *, it is not recommanded\n");
	WARNING(IO, "It is your responsibility to guarantee the FILE * is at proper state\n");
	WARNING(IO, "building file io from std file %p, is_write=%d\n", fp, is_write);

	struct file_io_t * r = xcalloc(1, sizeof(*r));
	assert(r != NULL);
	r->io.functionor = &file_io_functionor;
	r->io.pprivate = fp;
	r->io.id = unknown_file_path;
	r->fp = fp;
	r->permanent_mapped = FALSE;
	r->map_base = (void*)(0xffffffff);
	r->file_sz = -1;

	if (is_write)
		r->io.rdwr = IO_WRITE;
	else
		r->io.rdwr = IO_READ;

	return &(r->io);
}

static void *
file_command(struct io_t * io, const char * cmd, void * arg)
{
	DEBUG(IO, "run command %s for file io\n", cmd);
	if (strncmp("permanentmap", cmd,
				sizeof("permanentmap") - 1) == 0)
		return file_permanentmap(io);
	if (strncmp("buildfromstdfile:read", cmd,
		sizeof("buildfromstdfile:read") - 1) == 0)
			return file_build_io_from_stdfile(arg, FALSE);
	if (strncmp("buildfromstdfile:write", cmd,
		sizeof("buildfromstdfile:write") - 1) == 0)
			return file_build_io_from_stdfile(arg, TRUE);
	return NULL;
	
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
	.flush = file_flush,
	.seek = file_seek,
	.tell = file_tell,
	.close = file_close,
	.map_to_mem = file_map_to_mem,
	.release_map = file_release_map,
	.command = file_command,
	.get_sz = file_get_sz,
};

// vim:ts=4:sw=4

