/* 
 * io.h
 * by WN @ Dec. 13, 2009
 */

#ifndef __RESOURCES_H
#define __RESOURCES_H

#include <common/functionor.h>
#include <assert.h>

#define RES_READ	(1)
#define RES_WRITE	(2)
struct io_functionor_t;
struct io_t {
	struct io_functionor_t * functionor;
	int rdwr;
	void * pprivate;
};

extern struct function_class_t
io_function_class;

struct io_functionor_t {
	BASE_FUNCTIONOR
	struct io_t * (*open)(const char *);
	struct io_t * (*open_write)(const char *);
	int (*read)(struct io_t * res, void * ptr,
			int size, int nr);
	int (*write)(struct io_t * res, void * ptr,
			int size, int nr);
	int (*seek)(struct io_t * res, int offset,
			int whence);
	void (*close)(struct io_t * res);
};

extern struct io_functionor_t *
get_io_handler(const char * proto);

struct io_t *
res_open(const char * proto, const char * name);

struct io_t *
res_open_write(const char * proto, const char * name);

static inline int
res_read(struct io_t * res, void * ptr,
		int size, int nr)
{
	assert(res &&
			(res->functionor) &&
			(res->functionor->read));
	return res->functionor->read(res, ptr,
			size, nr);
}

static inline int
res_write(struct io_t * res, void * ptr,
		int size, int nr)
{
	assert(res &&
			(res->functionor) &&
			(res->functionor->write));
	return res->functionor->write(res, ptr,
			size, nr);
}

static inline int
res_seek(struct io_t * res, int offset,
		int whence)
{
	assert(res &&
			(res->functionor) &&
			(res->functionor->seek));
	return res->functionor->seek(res, offset,
			whence);
}

static inline void
res_close(struct io_t * res)
{
	assert(res &&
			(res->functionor) &&
			(res->functionor->close));
	res->functionor->close(res);
}

#endif

// vim:ts=4:sw=4

