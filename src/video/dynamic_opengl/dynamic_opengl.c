/* 
 * dynamic_opengl.c
 * by WN @ Feb. 11, 2010
 */

#include <common/debug.h>
#include "dynamic_opengl.h"

void
init_func_list(void * (getaddr)(const char *), struct func_entry * array)
{
	/* scan the array and set all entries to NULL */
	struct func_entry * parray = array;
	while (parray->name != NULL) {
		*(parray->ptr) = NULL;
		parray ++;
	}


	/* reset */
	while (array->name != NULL) {
		if (*(array->ptr) == NULL) {
			*(array->ptr) = getaddr(array->name);
			if (*(array->ptr) == NULL)
				WARNING(OPENGL, "doesn't contain %s\n", array->name);
			else
				TRACE(OPENGL, "%s:\t%p\n", array->name, *(array->ptr));
		} else {
			TRACE(OPENGL, "%s has multiple names and has already inited\n",
					array->name);
		}
		array ++;
	}
}

// vim:ts=4:sw=4

