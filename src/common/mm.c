/* 
 * mm.c
 * by WN @ Nov. 19, 2009
 */

#include <common/debug.h>
#include <common/exception.h>
#include <common/list.h>
#include <common/mm.h>

#include <assert.h>
#include <memory.h>

void
generic_free(void * ptr)
{
	xfree(ptr);
}

// vim:ts=4:sw=4

