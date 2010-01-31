/* 
 * io.c
 * by WN @ Dec. 13, 2009
 */

#include <config.h>
#include <common/dict.h>
#include <common/strhacks.h>
#include <common/debug.h>
#include <io/io.h>
#include <assert.h>
#include <string.h>

extern struct functionor_t file_io_functionor;
extern struct functionor_t xp3_io_functionor;

static struct functionor_t * functionors[] = {
	&xp3_io_functionor,
	&file_io_functionor,
	NULL,
};

struct function_class_t io_function_class = {
	.fclass = FC_IO,
	.current_functionor = NULL,
	.functionors = &functionors,
};

#include "io_functionors_dict.h"

struct io_functionor_t *
get_io_handler(const char * proto)
{
	struct io_functionor_t * r = NULL;
	assert(proto != NULL);

	char * p = strdupa(proto);
	str_toupper(p);
	TRACE(IO, "find io handler for proto \"%s\"\n",
			p);
	r = strdict_get(&io_functionors_dict, p).ptr;

	/* io_init is no reenter problem */
	if (r != NULL) {
		TRACE(IO, "get handler \"%s\" from dict for proto \"%s\"\n",
				r->name, p);
		io_init(r, proto);
		return r;
	}

	r = (struct io_functionor_t*)find_functionor(
			&io_function_class, p);
	assert(r != NULL);
	DEBUG(IO, "find handler \"%s\" for type %s\n",
			r->name, p);
	bool_t rep = strdict_replace(&io_functionors_dict,
			p, (dict_data_t)(void*)r, NULL);
	assert(rep);
	io_init(r, proto);
	return r;
}

struct io_t *
io_open(const char * proto, const char * name)
{
	struct io_functionor_t * f;
	f = get_io_handler(proto);
	assert(f != NULL);
	return f->open(name);
}

struct io_t *
io_open_write(const char * proto, const char * name)
{
	struct io_functionor_t * f;
	f = get_io_handler(proto);
	assert(f != NULL);
	return f->open_write(name);
}

void
__io_cleanup(void)
{
	struct functionor_t ** pos = functionors;
	while (*pos != NULL) {
		if ((*pos)->cleanup)
			(*pos)->cleanup();
		pos ++;
	}
}

// vim:ts=4:sw=4

