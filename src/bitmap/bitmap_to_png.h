/* 
 * bitmap_to_png.h
 * by WN @ Feb. 03, 2009
 */

#ifndef __BITMAP_TO_PNG_H
#define __BITMAP_TO_PNG_H

#include <common/defs.h>
#include <bitmap/bitmap.h>
#include <io/io.h>

__BEGIN_DECLS

extern void
bitmap_to_png(struct bitmap_t * b, struct io_t * io);

__END_DECLS
#endif

