/* 
 * io.h
 * by WN @ Dec. 13, 2009
 */

#ifndef __IO_H
#define __IO_H

#include <sys/uio.h>
#include <common/functionor.h>
#include <assert.h>

/* for ssize_t */
#include <stdio.h>

#define IO_READ	(1)
#define IO_WRITE	(2)
struct io_functionor_t;
struct io_t {
	struct io_functionor_t * functionor;
	int rdwr;
	void * pprivate;
};

extern struct function_class_t
io_function_class;

/* Those function should never return error (negitive number),
 * if error arise, throw an exception instead. */

struct io_functionor_t {
	BASE_FUNCTIONOR
	struct io_t * (*open)(const char *);
	struct io_t * (*open_write)(const char *);
	int (*read)(struct io_t * io, void * ptr,
			int size, int nr);
	int (*write)(struct io_t * io, void * ptr,
			int size, int nr);
	ssize_t (*readv)(struct io_t * io, struct iovec * iovec,
			int nr);
	ssize_t (*writev)(struct io_t * io, struct iovec * iovec,
			int nr);
	ssize_t (*vmsplice)(struct io_t * io, struct iovec * iovec,
			int nr);
	int (*seek)(struct io_t * io, int offset,
			int whence);
	void (*close)(struct io_t * io);
};

extern struct io_functionor_t *
get_io_handler(const char * proto);

struct io_t *
io_open(const char * proto, const char * name);

struct io_t *
io_open_write(const char * proto, const char * name);

static inline int
io_read(struct io_t * io, void * ptr,
		int size, int nr)
{
	assert(io &&
			(io->functionor) &&
			(io->functionor->read));
	return io->functionor->read(io, ptr,
			size, nr);
}

static inline int
io_write(struct io_t * io, void * ptr,
		int size, int nr)
{
	assert(io &&
			(io->functionor) &&
			(io->functionor->write));
	return io->functionor->write(io, ptr,
			size, nr);
}

static inline int
io_seek(struct io_t * io, int offset,
		int whence)
{
	assert(io &&
			(io->functionor) &&
			(io->functionor->seek));
	return io->functionor->seek(io, offset,
			whence);
}

static inline void
io_close(struct io_t * io)
{
	assert(io &&
			(io->functionor) &&
			(io->functionor->close));
	io->functionor->close(io);
}

static inline int
io_writev(struct io_t * io, struct iovec * vecs, int nr)
{
	assert(io &&
			(io->functionor) &&
			(io->functionor->writev));
	return io->functionor->writev(io, vecs, nr);
}

static inline int
io_vmsplice(struct io_t * io, struct iovec * vecs, int nr)
{
	assert(io &&
			(io->functionor) &&
			(io->functionor->vmsplice));
	return io->functionor->vmsplice(io, vecs, nr);
}



static inline int
io_readv(struct io_t * io, struct iovec * vecs, int nr)
{
	assert(io &&
			(io->functionor) &&
			(io->functionor->readv));
	return io->functionor->readv(io, vecs, nr);
}


#endif

// vim:ts=4:sw=4

