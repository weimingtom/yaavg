/* 
 * resource.h
 * by WN @ Jan. 25, 2010
 */

#ifndef __RESOURCE_H
#define __RESOURCE_H

#include <io/io.h>
#include <common/cache.h>

enum resource_type {
	RESOURCE_TYPE_BITMAP,
	NR_RESOURCE_TYPES,
};

struct resource_t {
	const char * id;
	int res_sz;
	void (*serialize)(struct resource_t * r, struct io_t * io);
	void (*destroy)(struct resource_t * r);
	/* cache_entry is maintained by resource_proc,
	 * don't touch it when creating resource_t. */
	struct cache_entry_t cache_entry;
	void * ptr;
	void * pprivate;
};

/* format of id:
 *	'0:file:/xxx/xxx.png'
 * */

struct resource_t *
load_resource(const char * id);

#endif

// vim:ts=4:sw=4

