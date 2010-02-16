/* 
 * by WN @ Feb. 17, 2010
 */

#ifndef __RESOURCE_TYPES_H
#define __RESOURCE_TYPES_H

#include <common/defs.h>
#include <bitmap/bitmap.h>

__BEGIN_DECLS

extern struct bitmap_t *
load_bitmap(const char * id, struct bitmap_deserlize_param * p);

__END_DECLS

#endif

