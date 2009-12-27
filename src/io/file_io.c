/* 
 * file_io.c
 * by WN @ Dec. 13, 2009
 */

#include <common/mm.h>
#include <common/debug.h>
#include <common/exception.h>
#include <io/io.h>
#include <stdio.h>

struct io_functionor_t file_io_functionor;

static struct io_t *
file_open(const char * path, const char * mode)
{
	assert(path != NULL);
	FILE * fp = fopen(path, mode);
	if (fp == NULL)
		THROW(EXP_RESOURCE_NOT_FOUND, "unable to open file %s with mode %s",
				path, mode);
	struct io_t * r = calloc(1, sizeof(*r));
	assert(r != NULL);
	r->functionor = &file_io_functionor;
	r->pprivate = fp;
	return r;
}

static struct io_t *
file_read_open(const char * path)
{
	struct io_t * r = file_open(path, "rb");
	if (r == NULL)
		return NULL;
	r->rdwr = RES_READ;
	return r;
}

static struct io_t *
file_write_open(const char * path)
{
	struct io_t * r = file_open(path, "wb");
	if (r == NULL)
		return NULL;
	r->rdwr = RES_WRITE;
	return r;
}

static int
file_read(struct io_t * io, void * ptr,
		int size, int nr)
{
	assert(io != NULL);
	assert(io->functionor);
	assert(io->functionor->read == file_read);
	assert(io->rdwr = RES_READ);

	FILE * fp = io->pprivate;
	assert(fp != NULL);
	return fread(ptr, size, nr, fp);
}

static int
file_write(struct io_t * io, void * ptr,
		int size, int nr)
{
	assert(io != NULL);
	assert(io->functionor);
	assert(io->functionor->write == file_write);
	assert(io->rdwr = RES_WRITE);

	FILE * fp = io->pprivate;
	assert(fp != NULL);
	return fwrite(ptr, size, nr, fp);
}

static int
file_seek(struct io_t * io, int offset,
		int whence)
{
	assert(io != NULL);
	assert(io->functionor);
	assert(io->functionor->seek == file_seek);
	FILE * fp = io->pprivate;
	assert(fp != NULL);
	return fseek(fp, offset, whence);
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

static bool_t
file_check_usable(const char * param)
{
	assert(param != NULL);
	if (strncasecmp("file", param, 4) == 0)
		return TRUE;
	return FALSE;
}

struct io_functionor_t file_io_functionor = {
	.name = "libc file",
	.fclass = FC_IO,
	.check_usable = file_check_usable,
	.open = file_read_open,
	.open_write = file_write_open,
	.read = file_read,
	.write = file_write,
	.seek = file_seek,
	.close = file_close,
};

// vim:ts=4:sw=4

