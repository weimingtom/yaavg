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

static char *
_strtok(char * id, char tok)
{
	assert(tok != '\0');
	while ((*id != '\0') && (*id != tok))
		id ++;
	if (id == '\0')
		return NULL;
	return id;
}

#define NEXT_PART(x, s) do {\
	(x) = _strtok((s), ':');\
	assert(*(x) == ':');	\
	*(x) = '\0';			\
	x ++;					\
} while (0)

struct resource_t *
load_resource(const char * __id)
{
	DEBUG(RESOURCE, "loading resource %s\n", __id);
	char * id = strdupa(__id);
	char * type, * iot, * name;
	type = id;
	NEXT_PART(iot, type);
	NEXT_PART(name, iot);

	TRACE(RESOURCE, "type=%s, iot=%s, name=%s\n",
			type, iot, name);

	enum resource_type t = atoi(type);
	assert((t >= 0) && (t < NR_RESOURCE_TYPES));

	struct io_t * io = NULL;
	struct resource_t * r = NULL;
	struct loader_table_entry * loaders = &loader_table[t];
	struct exception_t exp;
	TRY(exp) {
		io = io_open(iot, name);
		r = loaders->normal_loader(io, __id);
	} FINALLY {
		if (io != NULL)
			io_close(io);
	} CATCH(exp) {
		switch (exp.type) {
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

