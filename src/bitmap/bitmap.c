/* 
 * bitmap.c
 * by WN @ Dec. 13, 2009
 */

#include <config.h>
#include <common/debug.h>
#include <common/functionor.h>
#include <common/dict.h>
#include <common/mm.h>
#include <bitmap/bitmap.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

extern struct functionor_t dummy_bitmap_resource_functionor;
extern struct functionor_t sdl_bitmap_resource_functionor;

static struct functionor_t * functionors[] = {
//	&sdl_bitmap_resource_functionor,
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

struct bitmap_resource_functionor_t *
get_bitmap_resource_handler(const char * name)
{
	struct functionor_t * func = NULL;
	assert(name != NULL);
	int len = strlen(name);
	assert(len >= 3);
	TRACE(BITMAP, "find bitmap handler for %s\n", name);

	char format_str[4];

	for (int i = 0; i < 3; i++) {
		int c = name[len - 3 + i];
		format_str[i] = islower(c) ? toupper(c) : c;
	}
	format_str[3] = '\0';
	TRACE(BITMAP, "format is %s\n", format_str);

	func = strdict_get(&bitmap_resource_functionors_dict, format_str).ptr;
	if (func) {
		TRACE(BITMAP, "get handler %s from dict\n",
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

struct bitmap_t *
alloc_bitmap(struct bitmap_t * phead)
{
	int id_sz = (uintptr_t)(phead->id);
	struct bitmap_t * res = xmalloc(sizeof(*phead) +
			id_sz +
			bitmap_data_size(phead) +
			PIXELS_ALIGN - 1);
	assert(res != NULL);
	*res = *phead;
	res->id = (char *)(res->__data);
	res->pixels = PIXELS_PTR(res->__data + id_sz);
	return res;
}

void
free_bitmap(struct bitmap_t * ptr)
{
	xfree(ptr);
}

struct bitmap_t *
bitmap_deserialize(struct io_t * io)
{
	struct bitmap_t head;
	assert(io && (io->functionor->read));

	TRACE(BITMAP, "bitmap deserializing from io \"%s\"\n",
			io->functionor->name);
	
	/* read head */
	io_read(io, &head, sizeof(head), 1);
	int ds = bitmap_data_size(&head); 
	int id_sz = (uintptr_t)(head.id);
	assert(id_sz > 0);


	/* check */
	assert((uintptr_t)(head.pixels) == ds);

	TRACE(BITMAP, "read head, w=%d, h=%d, bpp=%d, id_sz=%d\n",
			head.w, head.h, head.bpp, id_sz);

	struct bitmap_t * r = alloc_bitmap(&head);
	assert(r != NULL);

	/* read id */
	io_read(io, r->id, id_sz, 1);
	TRACE(BITMAP, "read id: %s\n", r->id);

	/* read pixels */
	io_read(io, r->pixels, ds, 1);
	return r;
}

// vim:ts=4:sw=4

