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
extern struct functionor_t sdl_bitmap_resource_functionor;

static struct functionor_t * functionors[] = {
	&sdl_bitmap_resource_functionor,
//	&png_bitmap_resource_functionor,
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

	char format_str[4];

	for (int i = 0; i < 3; i++) {
		int c = id[len - 3 + i];
		format_str[i] = islower(c) ? toupper(c) : c;
	}
	format_str[3] = '\0';
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
		assert(rep);
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


// vim:ts=4:sw=4

