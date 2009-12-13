/* 
 * bitmap.c
 * by WN @ Dec. 13, 2009
 */

#include <config.h>
#include <common/debug.h>
#include <common/functionor.h>
#include <common/dict.h>
#include <bitmap/bitmap.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

extern struct functionor_t dummy_bitmap_functionor;
extern struct functionor_t sdl_bitmap_functionor;

static struct functionor_t * functionors[] = {
	&sdl_bitmap_functionor,
//	&png_bitmap_functionor,
	&dummy_bitmap_functionor,
	NULL,
};

struct function_class_t bitmap_function_class = {
	.fclass = FC_BITMAP_HANDLER,
	.current_functionor = NULL,
	.functionors = &functionors,
};

#include "bitmap_functionors_dict.h"

struct bitmap_functionor_t *
get_bitmap_handler(const char * name)
{
	struct bitmap_functionor_t * func = NULL;
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

	func = strdict_get(&bitmap_functionors_dict, format_str).ptr;
	if (func) {
		TRACE(BITMAP, "get handler %s from dict\n",
				func->name);
		return func;
	}

	func = (struct bitmap_functionor_t*)find_functionor(&bitmap_function_class,
			format_str);
	assert(func != NULL);
	DEBUG(BITMAP, "find handler %s for type %s\n",
			func->name, format_str);
	bool_t rep = strdict_replace(&bitmap_functionors_dict, format_str,
			(dict_data_t)(void*)func, NULL);
	assert(rep);
	return func;
}

struct bitmap_t *
load_bitmap(const char * name)
{
	assert(name != NULL);
	struct bitmap_functionor_t * functionor = get_bitmap_handler(name);
	return functionor->load_bitmap(name);
}

void
destroy_bitmap(struct bitmap_t * b)
{
	assert(b != NULL);
	b->functionor->destroy_bitmap(b);
}

// vim:ts=4:sw=4

