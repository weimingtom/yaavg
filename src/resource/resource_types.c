/* 
 * resource_type.c
 * by WN @ Feb. 17, 2010
 */

#include <resource/resource_types.h>
#include <resource/resource_proc.h>
#include <bitmap/bitmap.h>

#include <yconf/yconf.h>
#include <common/bithacks.h>
#include <common/cache.h>
#include <common/mm.h>

struct bitmap_t *
load_bitmap(const char * id, struct bitmap_deserlize_param * p)
{
	assert(id != NULL);
	if (p != NULL)
		assert(is_power_of_2(p->align));

	struct bitmap_t * r = NULL;
	r = get_resource(id, (deserializer_t)bitmap_deserialize, p);
	assert(r != NULL);
	return r;
}

// vim:ts=4:sw=4

