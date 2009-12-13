/* 
 * file_resources.c
 * by WN @ Dec. 13, 2009
 */

#include <common/mm.h>
#include <common/debug.h>
#include <common/exception.h>
#include <resources/resources.h>
#include <stdio.h>

DEFINE_MEM_CACHE(__file_resources_cache, "cache of file resources",
		sizeof(struct resources_t));
static struct mem_cache_t * pfile_resources_cache = &__file_resources_cache;

struct resources_functionor_t file_resources_functionor;

static struct resources_t *
file_open(const char * path, const char * mode)
{
	assert(path != NULL);
	FILE * fp = fopen(path, mode);
	if (fp == NULL)
		THROW(EXP_RESOURCE_NOT_FOUND, "unable to open file %s with mode %s",
				path, mode);
	struct resources_t * r = mem_cache_zalloc(pfile_resources_cache);
	assert(r != NULL);
	r->functionor = &file_resources_functionor;
	r->pprivate = fp;
	return r;
}

static struct resources_t *
file_read_open(const char * path)
{
	struct resources_t * r = file_open(path, "rb");
	if (r == NULL)
		return NULL;
	r->rdwr = RES_READ;
	return r;
}

static struct resources_t *
file_write_open(const char * path)
{
	struct resources_t * r = file_open(path, "wb");
	if (r == NULL)
		return NULL;
	r->rdwr = RES_WRITE;
	return r;
}

static int
file_read(struct resources_t * res, void * ptr,
		int size, int nr)
{
	assert(res != NULL);
	assert(res->functionor);
	assert(res->functionor->read == file_read);
	assert(res->rdwr = RES_READ);

	FILE * fp = res->pprivate;
	assert(fp != NULL);
	return fread(ptr, size, nr, fp);
}

static int
file_write(struct resources_t * res, void * ptr,
		int size, int nr)
{
	assert(res != NULL);
	assert(res->functionor);
	assert(res->functionor->write == file_write);
	assert(res->rdwr = RES_WRITE);

	FILE * fp = res->pprivate;
	assert(fp != NULL);
	return fwrite(ptr, size, nr, fp);
}

static int
file_seek(struct resources_t * res, int offset,
		int whence)
{
	assert(res != NULL);
	assert(res->functionor);
	assert(res->functionor->seek == file_seek);
	FILE * fp = res->pprivate;
	assert(fp != NULL);
	return fseek(fp, offset, whence);
}

static void
file_close(struct resources_t * res)
{
	assert(res != NULL);
	assert(res->functionor);
	assert(res->functionor->close == file_close);
	FILE * fp = res->pprivate;
	assert(fp != NULL);
	fclose(fp);
	mem_cache_free(pfile_resources_cache, res);
}

static bool_t
file_check_usable(const char * param)
{
	assert(param != NULL);
	if (strncasecmp("file", param, 4) == 0)
		return TRUE;
	return FALSE;
}

struct resources_functionor_t file_resources_functionor = {
	.name = "libc file",
	.fclass = FC_RESOURCES,
	.check_usable = file_check_usable,
	.open = file_read_open,
	.open_write = file_write_open,
	.read = file_read,
	.write = file_write,
	.seek = file_seek,
	.close = file_close,
};

// vim:ts=4:sw=4

