/* 
 * io.h
 * by WN @ Dec. 13, 2009
 */

#ifndef __IO_H
#define __IO_H

#include <sys/uio.h>
#include <common/functionor.h>
#include <common/exception.h>
#include <common/mm.h>
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

/* 
 * this structure MUST be allocated by xfreeable
 * method. table must pointed into __data, and
 * all entries of table should also pointed into it.
 * the last entry in table always 0.
 */
struct package_items_t {
	char ** table;
	int nr_items;
	/* for serialization used */
	int total_sz;
	uint8_t __data[0];
};

struct package_items_t *
deserialize_package_items(struct io_t * io);

void
serialize_and_destroy_package_items(
		struct package_items_t ** pitems, struct io_t * io);

/* Those function should never return error (negitive number),
 * if error arise, throw an exception instead. */

struct io_functionor_t {
	BASE_FUNCTIONOR
	bool_t inited;
	struct io_t * (*open)(const char *);
	struct io_t * (*open_write)(const char *);
	/* read shouldn't return error. If error, it should issue an exception */
	/* however, retval of read may different from nr. see io_read_force */
	int (*read)(struct io_t * io, void * ptr,
			int size, int nr);
	int (*write)(struct io_t * io, void * ptr,
			int size, int nr);
	int (*flush)(struct io_t * io);
	ssize_t (*readv)(struct io_t * io, struct iovec * iovec,
			int nr);
	ssize_t (*writev)(struct io_t * io, struct iovec * iovec,
			int nr);
	ssize_t (*vmsplice_write)(struct io_t * io, struct iovec * iovec,
			int nr);
	ssize_t (*vmsplice_read)(struct io_t * io, struct iovec * iovec,
			int nr);
	/* seek and tell should return 0 only, if error, it should issue an exception */
	void (*seek)(struct io_t * io, int64_t offset,
			int whence);
	int64_t (*tell)(struct io_t * io);
	int64_t (*get_sz)(struct io_t * io);

	void (*close)(struct io_t * io);
	void * (*command)(struct io_t * io, const char * cmd, void * arg);

	/* below functions are designed for special propose, never call them even
	 * if you believe you know what you are doing. */
	void * (*map_to_mem)(struct io_t * io, int from, int max_sz);
	void (*release_map)(struct io_t * io, void * ptr, int from, int len);
	/* internal buffer functions provide a fast way to access memory mapped file buffer.
	 * some implementation locks cache, doesn't allow any cache operation before
	 * release_internal_buffer */
	void * (*get_internal_buffer)(struct io_t * io);
	void (*release_internal_buffer)(struct io_t * io, void * ptr);

	/* the get_package_items is used for package file. the return ptr is malloced
	 * by the io functionor, and should be xfree()d by the caller.
	 * the format of returned 
	 * */
	struct package_items_t * (*get_package_items)(const char * name);
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
io_open_proto(const char * proto_name);

struct io_t *
io_open_write_proto(const char * proto_name);

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
io_flush(struct io_t * io)
{
	assert(io && (io->functionor));
	assert(io->rdwr & IO_WRITE);
	if (io->functionor->flush)
		return io->functionor->flush(io);
	return 0;
}

static inline void
io_seek(struct io_t * io, int64_t offset,
		int whence)
{
	assert(io &&
			(io->functionor) &&
			(io->functionor->seek));
	io->functionor->seek(io, offset,
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

static inline int64_t
io_get_sz(struct io_t * io)
{
	assert(io && (io->functionor));
	
	if (io->functionor->get_sz) {
		return io->functionor->get_sz(io);
	}

	int64_t pos_save, pos;
	pos_save = io_tell(io);
	io_seek(io, 0, SEEK_END);
	pos = io_tell(io);
	io_seek(io, pos_save, SEEK_SET);
	return pos;
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
	err = io_read(io, data, sz, 1);
	if (err != 1)
		THROW(EXP_BAD_RESOURCE, "read file %s failed: expect %d but read %d",
				io->id, 1, err);
}


static inline void
io_write_force(struct io_t * io, void * data, int sz)
{
	int err;
	assert(io != NULL);
	if (sz <= 0)
		return;
	assert(data != NULL);
	err = io_write(io, data, 1, sz);
	if (err != sz)
		THROW(EXP_BAD_RESOURCE, "write file %s failed: expect %d but read %d",
				io->id, sz, err);
}


static inline void *
io_map_to_mem(struct io_t * io, int from, int max_sz)
{
	assert(io && (io->functionor));
	if (io->functionor->map_to_mem)
		return io->functionor->map_to_mem(io, from, max_sz);

	int64_t sz = io_get_sz(io);
	assert(sz < 0x7fffffff);

	if (from + max_sz > sz)
		max_sz = sz - from;

	void * ptr = xmalloc((int)(max_sz));
	assert(ptr != NULL);
	int64_t save_pos = io_tell(io);

	io_seek(io, 0, SEEK_SET);
	io_read_force(io, ptr, max_sz);
	io_seek(io, save_pos, SEEK_SET);
	return ptr;
}

static inline void
io_release_map(struct io_t * io, void * ptr, int from, int len)
{
	assert(io && (io->functionor));
	if (io->functionor->release_map)
		return io->functionor->release_map(io, ptr, from, len);
	xfree(ptr);
}

static inline void *
io_get_internal_buffer(struct io_t * io)
{
	assert(io && (io->functionor));
	if (io->functionor->get_internal_buffer)
		return io->functionor->get_internal_buffer(io);
	return io_map_to_mem(io, 0, io_get_sz(io));
}
static inline void
io_release_internal_buffer(struct io_t * io, void * ptr)
{
	assert(io && (io->functionor));
	if (io->functionor->release_internal_buffer)
		return io->functionor->release_internal_buffer(io, ptr);
	return io_release_map(io, ptr, 0, io_get_sz(io));
}


static inline void *
iof_command(struct io_functionor_t * iof, const char * cmd, void * arg)
{
	assert(iof);
	if (iof->command == NULL)
		return NULL;
	return iof->command(NULL, cmd, arg);
}

static inline struct package_items_t *
iof_get_package_items(struct io_functionor_t * iof, const char * cmd)
{
	assert(iof);
	if (iof->get_package_items == NULL)
		THROW(EXP_UNSUPPORT_OPERATION, "%s doesn't support get_package_items", iof->name);
	return iof->get_package_items(cmd);
}


static inline void *
io_command(struct io_t * io, const char * cmd, void * arg)
{
	assert(io != NULL);
	assert(io->functionor);
	if (io->functionor->command == NULL)
		return NULL;
	return io->functionor->command(io, cmd, arg);
}

static inline uint8_t
io_read_byte(struct io_t * io)
{
	uint8_t x;
	io_read_force(io, &x, sizeof(x));
	return x;
}

static inline uint64_t
io_read_le64(struct io_t * io)
{
	uint64_t x;
	io_read_force(io, &x, sizeof(x));
	return le64toh(x);
}

static inline uint64_t
io_read_be64(struct io_t * io)
{
	uint32_t x;
	io_read_force(io, &x, sizeof(x));
	return be64toh(x);
}

static inline uint32_t
io_read_le32(struct io_t * io)
{
	uint32_t x;
	io_read_force(io, &x, sizeof(x));
	return le32toh(x);
}

static inline uint32_t
io_read_be32(struct io_t * io)
{
	uint32_t x;
	io_read_force(io, &x, sizeof(x));
	return be32toh(x);
}


#endif

// vim:ts=4:sw=4

