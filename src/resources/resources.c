/* 
 * resources.c
 * by WN @ Dec. 13, 2009
 */

#include <config.h>
#include <common/dict.h>
#include <common/strhacks.h>
#include <common/debug.h>
#include <resources/resources.h>
#include <assert.h>
#include <string.h>

extern struct functionor_t file_resources_functionor;

static struct functionor_t * functionors[] = {
	&file_resources_functionor,
	NULL,
};

struct function_class_t resources_function_class = {
	.fclass = FC_RESOURCES,
	.current_functionor = NULL,
	.functionors = &functionors,
};

#include "resources_functionors_dict.h"

struct resources_functionor_t *
get_resources_handler(const char * proto)
{
	struct resources_functionor_t * r = NULL;
	assert(proto != NULL);

	char * p = strdupa(proto);
	str_toupper(p);
	TRACE(RESOURCES, "find resources handler for proto \"%s\"\n",
			p);
	r = strdict_get(&resources_functionors_dict, p).ptr;
	if (r != NULL) {
		TRACE(RESOURCES, "get handler \"%s\" from dict for proto \"%s\"\n",
				r->name, p);
		return r;
	}

	r = (struct resources_functionor_t*)find_functionor(
			&resources_function_class, p);
	assert(r != NULL);
	DEBUG(RESOURCES, "find handler \"%s\" for type %s\n",
			r->name, p);
	bool_t rep = strdict_replace(&resources_functionors_dict,
			p, (dict_data_t)(void*)r, NULL);
	assert(rep);
	return r;
}

struct resources_t *
res_open(const char * proto, const char * name)
{
	struct resources_functionor_t * f;
	f = get_resources_handler(proto);
	assert(f != NULL);
	return f->open(name);
}

struct resources_t *
res_open_write(const char * proto, const char * name)
{
	struct resources_functionor_t * f;
	f = get_resources_handler(proto);
	assert(f != NULL);
	return f->open_write(name);
}


// vim:ts=4:sw=4

