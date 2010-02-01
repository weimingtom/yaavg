/* 
 * io.h
 * by WN @ Dec. 13, 2009
 */

#ifndef __IO_H
#define __IO_H

#include <sys/uio.h>
#include <common/functionor.h>
#include <common/exception.h>
#include <assert.h>

/* for ssize_t */
#include <stdio.h>

#include <endian.h>

#define IO_READ	(1)
#define IO_WRITE	(2)
struct io_functionor_t;
struct io_t {
	struct io_functionor_t * functionor;
	int rdwr;
	const char * id;
	void * pprivate;
	char __data[0];
};

extern struct function_class_t
io_function_class;

/* Those function should never return error (negitive number),
 * if error arise, throw an exception instead. */

struct io_functionor_t {
	BASE_FUNCTIONOR
	bool_t inited;
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
	ssize_t (*vmsplice_write)(struct io_t * io, struct iovec * iovec,
			int nr);
	ssize_t (*vmsplice_read)(struct io_t * io, struct iovec * iovec,
			int nr);
	int (*seek)(struct io_t * io, int64_t offset,
			int whence);
	int64_t (*tell)(struct io_t * io);
	void (*close)(struct io_t * io);
	void * (*command)(const char * cmd, void * arg);
};

extern struct io_functionor_t *
get_io_handler(const char * proto);

static void inline
io_init(struct io_functionor_t * r, const char * proto)
{
	if (r == NULL)
		return;
	if (!r->inited) {
		if (r->check_usable)
			if(!(r->check_usable(proto)))
				THROW(EXP_UNSUPPORT_IO,
						"IO for \"%s\" is unsupported currently",
						proto);
		if (r->init)
			r->init();
		r->inited = TRUE;
	}
}

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
io_seek(struct io_t * io, int64_t offset,
		int whence)
{
	assert(io &&
			(io->functionor) &&
			(io->functionor->seek));
	return io->functionor->seek(io, offset,
			whence);
}

static inline int64_t
io_tell(struct io_t * io)
{
	assert(io &&
			(io->functionor) &&
			(io->functionor->seek));
	return io->functionor->tell(io);
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
io_vmsplice_write(struct io_t * io, struct iovec * vecs, int nr)
{
	assert(io &&
			(io->functionor) &&
			(io->functionor->vmsplice_write));
	return io->functionor->vmsplice_write(io, vecs, nr);
}



static inline int
io_vmsplice_read(struct io_t * io, struct iovec * vecs, int nr)
{
	assert(io &&
			(io->functionor) &&
			(io->functionor->vmsplice_read));
	return io->functionor->vmsplice_read(io, vecs, nr);
}

static inline int
io_readv(struct io_t * io, struct iovec * vecs, int nr)
{
	assert(io &&
			(io->functionor) &&
			(io->functionor->readv));
	return io->functionor->readv(io, vecs, nr);
}

static inline void
io_read_force(struct io_t * io, void * data, int sz)
{
	int err;
	assert(io != NULL);
	if (sz <= 0)
		return;
	assert(data != NULL);
	err = io_read(io, data, 1, sz);
	if (err != sz)
		THROW(EXP_BAD_RESOURCE, "read file %s failed: expect %d but read %d",
				io->id, sz, err);
}

static inline void *
iof_command(struct io_functionor_t * iof, const char * cmd, void * arg)
{
	assert(iof);
	if (iof->command == NULL)
		return NULL;
	return iof->command(cmd, arg);
}


static inline void *
io_command(struct io_t * io, const char * cmd, void * arg)
{
	assert(io != NULL);
	assert(io->functionor);
	return iof_command(io->functionor, cmd, arg);
}

static inline uint8_t
io_read_byte(struct io_t * io)
{
	int err;
	uint8_t x;
	err = io_read(io, &x, sizeof(x), 1);
	if (err != 1)
		THROW(EXP_BAD_RESOURCE, "read byte from %s failed: return %d",
				io->id, err);
	return x;
}

static inline uint64_t
io_read_le64(struct io_t * io)
{
	int err;
	uint64_t x;
	err = io_read(io, &x, sizeof(x), 1);
	if (err != 1)
		THROW(EXP_BAD_RESOURCE, "read uin64 (le) from %s failed: return %d",
				io->id, err);
	return le64toh(x);
}

static inline uint64_t
io_read_be64(struct io_t * io)
{
	int err;
	uint32_t x;
	err = io_read(io, &x, sizeof(x), 1);
	if (err != 1)
		THROW(EXP_BAD_RESOURCE, "read uin64 (be) from %s failed: return %d",
				io->id, err);
	return be64toh(x);
}

static inline uint32_t
io_read_le32(struct io_t * io)
{
	int err;
	uint32_t x;
	err = io_read(io, &x, sizeof(x), 1);
	if (err != 1)
		THROW(EXP_BAD_RESOURCE, "read uin32 (le) from %s failed: return %d",
				io->id, err);
	return le32toh(x);
}

static inline uint32_t
io_read_be32(struct io_t * io)
{
	int err;
	uint32_t x;
	err = io_read(io, &x, sizeof(x), 1);
	if (err != 1)
		THROW(EXP_BAD_RESOURCE, "read uin32 (be) from %s failed: return %d",
				io->id, err);
	return be32toh(x);
}


#endif

// vim:ts=4:sw=4

