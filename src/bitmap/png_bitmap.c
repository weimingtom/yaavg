/* 
 * png_bitmap.c
 * by WN @ Dec. 13, 2009
 */

#include <config.h>
#include <common/debug.h>
#include <common/exception.h>
#include <bitmap/bitmap.h>

#include <string.h>

struct bitmap_functionor_t png_bitmap_functionor;

static bool_t
png_check_usable(const char * param)
{
	return FALSE;
}

struct bitmap_functionor_t png_bitmap_functionor = {
	.name = "libpng pngloader",
	.fclass = FC_BITMAP_HANDLER,
	.check_usable = png_check_usable,
};

// vim:ts=4:sw=4

