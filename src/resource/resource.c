/* 
 * resource.c
 * by WN @ Jan. 25, 2010
 */

#include <stdlib.h>

#include <config.h>
#include <common/debug.h>
#include <common/exception.h>
#include <io/io.h>
#include <resource/resource.h>
#include <bitmap/bitmap_resource.h>

typedef struct resource_t * (*loader_t)(struct io_t *, const char *);

static struct loader_table_entry {
	loader_t normal_loader;
	loader_t dummy_loader;
} loader_table[NR_RESOURCE_TYPES] = {
	[RESOURCE_TYPE_BITMAP] = {
		.normal_loader	= load_bitmap_resource,
		.dummy_loader	= load_dummy_bitmap_resource,
	},
};

struct resource_t *
load_resource(const char * __id)
{
	DEBUG(RESOURCE, "loading resource %s\n", __id);
	char * id = strdupa(__id);
	char * type, * io_proto, * io_name;
	type = id;
	io_proto = split_resource_type(type);
	io_name = split_protocol(io_proto);

	TRACE(RESOURCE, "type=%s, io_proto=%s, io_name=%s\n",
			type, io_proto, io_name);

	enum resource_type t = atoi(type);
	/* enum always >= 0 */
	assert(t < NR_RESOURCE_TYPES);

	struct io_t * io = NULL;
	struct resource_t * r = NULL;
	struct loader_table_entry * loaders = &loader_table[t];
	struct exception_t exp;
	TRY(exp) {
		io = io_open(io_proto, io_name);
		r = loaders->normal_loader(io, __id);
	} FINALLY {
		if (io != NULL)
			io_close(io);
	} CATCH(exp) {
		switch (exp.type) {
			case EXP_UNSUPPORT_FORMAT:
			case EXP_UNSUPPORT_RESOURCE:
			case EXP_RESOURCE_NOT_FOUND:
			case EXP_BAD_RESOURCE:
				print_exception(&exp);
				r = loaders->dummy_loader(NULL, __id);
				break;
			default:
				RETHROW(exp);
		}
	}
	return r;
}

// vim:ts=4:sw=4

