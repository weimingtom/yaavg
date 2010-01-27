/* 
 * bitmap_resource.c
 * by WN @ Jan. 24, 2010
 */

#include <config.h>
#include <common/debug.h>
#include <common/functionor.h>
#include <common/mm.h>
#include <io/io.h>
#include <bitmap/bitmap_resource.h>

#include <ctype.h>
#include <string.h>
#include <assert.h>

extern struct functionor_t dummy_bitmap_resource_functionor;
#ifdef HAVE_SDLIMAGE
extern struct functionor_t sdl_bitmap_resource_functionor;
#endif

static struct functionor_t * functionors[] = {
#ifdef HAVE_SDLIMAGE
	&sdl_bitmap_resource_functionor,
#endif
#ifdef HAVE_LIBPNG
//	&png_bitmap_resource_functionor,
#endif
	&dummy_bitmap_resource_functionor,
	NULL,
};

struct function_class_t
bitmap_resource_function_class = {
	.fclass = FC_BITMAP_RESOURCE_HANDLER,
	.current_functionor = NULL,
	.functionors = &functionors,
};

#include "bitmap_resource_functionors_dict.h"

static struct bitmap_resource_functionor_t *
get_bitmap_resource_handler(const char * id)
{
	struct functionor_t * func = NULL;
	assert(id != NULL);
	int len = strlen(id);
	assert(len >= 3);
	DEBUG(BITMAP, "find bitmap handler for %s\n", id);

	char __format_str[16], *format_str = NULL;

	/* find the last '.' */
	if (*id != '.') {
		const char * cp;
		int i;
		for (i = 15, cp = id + strlen(id);
				((i >= 0) && (cp >= id)); i--, cp --) {
			char c = *cp;
			__format_str[i] = islower(c) ? toupper(c) : c;
			if (c == '.') {
				format_str = &__format_str[i+1];
				break;
			}
		}
		if (format_str == NULL)
			THROW(EXP_UNSUPPORT_RESOURCE, "cannot find handler for id %s\n",
					id);
	} else {
		format_str = "...";
	}

	/* dirty work */
	if (strcmp(format_str, "JPEG") == 0)
		format_str = "JPG";

	DEBUG(BITMAP, "format is %s\n", format_str);

	if (format_str[0] != '.') {
		func = strdict_get(&bitmap_resource_functionors_dict, format_str).ptr;
		if (func) {
			DEBUG(BITMAP, "get handler %s from dict\n",
					func->name);
			return (struct bitmap_resource_functionor_t *)func;
		}

		func = find_functionor(&bitmap_resource_function_class,
				format_str);
		assert(func != NULL);

		DEBUG(BITMAP, "find handler %s for type %s\n",
				func->name, format_str);
		bool_t rep = strdict_replace(&bitmap_resource_functionors_dict, format_str,
				(dict_data_t)(void*)func, NULL);
		if (!rep)
			THROW(EXP_UNSUPPORT_RESOURCE, "cannot find handler for bitmap format %s\n",
				format_str);
		return (struct bitmap_resource_functionor_t *)func;
	}

	struct bitmap_resource_functionor_t * bfunc =
		(struct bitmap_resource_functionor_t*)(&dummy_bitmap_resource_functionor);
	DEBUG(BITMAP, "use dummy handler %s\n", bfunc->name);
	return bfunc;

}

static struct resource_t *
__load_bitmap_resource(struct io_t * io, const char * id,
		const char * type)
{
	struct bitmap_resource_t * b = NULL;
	struct bitmap_resource_functionor_t * f =
		get_bitmap_resource_handler(type);
	assert(f != NULL);

	if (io == NULL)
		assert(f == (void*)(&dummy_bitmap_resource_functionor));

	b = f->load(io, id);

	assert(b != NULL);
	return &(b->resource);
}

struct resource_t *
load_bitmap_resource(struct io_t * io, const char * id)
{
	return __load_bitmap_resource(io, id, id);
}

struct resource_t *
load_dummy_bitmap_resource(struct io_t * io, const char * id)
{
	return __load_bitmap_resource(NULL, id, "...");
}

void
common_bitmap_serialize(struct resource_t * r, struct io_t * io)
{
	assert(r != NULL);
	assert(io != NULL);
	struct bitmap_resource_t * b = container_of(r,
			struct bitmap_resource_t,
			resource);
	DEBUG(BITMAP, "serializing common bitmap %s\n",
			b->head.id);

	struct bitmap_t head = b->head;
	head.id = NULL;
	head.pixels = NULL;

	int sync = BEGIN_SERIALIZE_SYNC;
	io_write(io, &sync, sizeof(sync), 1);

	struct iovec vecs[3];

	vecs[0].iov_base = &head;
	vecs[0].iov_len = sizeof(head);

	vecs[1].iov_base = b->head.id;
	vecs[1].iov_len = b->head.id_sz;

	vecs[2].iov_base = b->head.pixels;
	vecs[2].iov_len = bitmap_data_size(&head);

	if (io->functionor->vmsplice_write) {
		io_vmsplice_write(io, vecs, 3);
	} else if (io->functionor->writev) {
		io_writev(io, vecs, 3);
	} else {
		io_write(io, vecs[0].iov_base, vecs[0].iov_len, 1);
		io_write(io, vecs[1].iov_base, vecs[1].iov_len, 1);
		io_write(io, vecs[2].iov_base, vecs[2].iov_len, 1);
	}

	/* wait for sync */
	/* end sync have to reside here because of the lifetime of head:
	 * it will dead right after we return from this function */
	io_read(io, &sync, sizeof(sync), 1);
	assert(sync == END_DESERIALIZE_SYNC);
	DEBUG(BITMAP, "got sync mark\n");

	DEBUG(BITMAP, "common bitmap %s send over\n",
			b->head.id);
}


// vim:ts=4:sw=4

