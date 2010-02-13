/* 
 * dynamic_opengl.h
 * by WN @ Feb. 11, 2010
 */

#ifndef __DYNAMIC_OPENGL_H
#define __DYNAMIC_OPENGL_H

struct func_entry {
	const char * name;
	void ** ptr;
};

extern void
init_func_list(void * (*)(const char *), struct func_entry * array);

#endif

// vim:ts=4:sw=4


