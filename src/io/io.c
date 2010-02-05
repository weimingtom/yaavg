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

static struct io_t *
__io_open_proto(const char * __name, bool_t write)
{
	assert(__name != NULL);
	char * name = strdupa(__name);
	char * proto = name;
	char * real_name = split_protocol(proto);
	assert(real_name != NULL);
	if (write)
		return io_open_write(proto, real_name);
	return io_open(proto, real_name);
}

struct io_t *
io_open_proto(const char * proto_name)
{
	return __io_open_proto(proto_name, FALSE);
}

struct io_t *
io_open_write_proto(const char * proto_name)
{
	return __io_open_proto(proto_name, TRUE);
}


struct io_t *
io_open_write(const char * proto, const char * name)
{
	struct io_functionor_t * f;
	f = get_io_handler(proto);
	assert(f != NULL);
	return f->open_write(name);
}

/* *item becoms not valid after serialization */
void
serialize_and_destroy_package_items(
		struct package_items_t ** pitems, struct io_t * io)
{
	struct package_items_t * items;
	assert(pitems != NULL);
	assert(io != NULL);

	items = *pitems;
	assert(items != NULL);

	TRACE(IO, "serialize and destroy a %d items description, its total sz is %d\n",
			items->nr_items, items->total_sz);
	void * base = items;
	/* for each items in items->table, minus its value by base */
	/* caution: *table is char *, minus 1 is actually minus its valus by
	 * 4(or 8 on 64 bit machine)  */
	for (int i = 0; i < items->nr_items; i++) {
		items->table[i] =(char*)
			((void*)(items->table[i]) - base);
	}
	items->table = (char**)((void*)(items->table) - base);
	io_write_force(io, items, items->total_sz);
	xfree_null(*pitems);
	return;
}

struct package_items_t *
deserialize_package_items(struct io_t * io)
{
	assert(io != NULL);

	struct package_items_t head, *retval = NULL;
	struct exception_t exp;
	TRY(exp) {
		io_read_force(io, &head, sizeof(head));
		TRACE(IO, "deserializing a %d items description, its total sz is %d\n",
				head.nr_items, head.total_sz);
		if (head.total_sz <= 0)
			THROW(EXP_BAD_RESOURCE, "total size of a struct package_items_t is %d",
					head.total_sz);
		retval = xmalloc(head.total_sz);
		assert(retval = NULL);
		*retval = head;
		io_read_force(io, retval->__data,
				retval->total_sz - sizeof(head));

		retval->table = (char**)(retval->__data);
		int nr_items = retval->nr_items;
		if (retval->table[nr_items] != NULL)
			THROW(EXP_BAD_RESOURCE, "last item is not null");

		void * base = retval;
		for (int i = 0; i < nr_items; i++) {
			void * p = retval->table[i];
			p += (uintptr_t)base;
			retval->table[i] = (char*)p;
		}
	} NO_FINALLY
	CATCH(exp) {
		xfree_null(retval);
		/* suppress all exception */
		if (exp.type == EXP_UNCATCHABLE)
			RETHROW(exp);
		print_exception(&exp);
		return NULL;
	}
	return retval;
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

