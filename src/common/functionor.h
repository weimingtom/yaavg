/* 
 * functionor.h
 * by WN @ Nov. 27, 2009
 */

#ifndef __FUNCTIONOR_H
#define __FUNCTIONOR_H

#include <config.h>
#include <common/defs.h>
#include <common/list.h>

enum FunctionClass {
	FC_VIDEO,
	FC_OPENGL_ENGINE,
	FC_BITMAP_RESOURCE_HANDLER,
	FC_TIMER,
	FC_IO,
	NR_FCS,
};

#define BASE_FUNCTIONOR	\
	const char * name; 	\
	enum FunctionClass fclass;	\
	bool_t (*check_usable)(const char * params);	\
	void (*init)(void);			\
	void (*cleanup)(void);

struct functionor_t {
	BASE_FUNCTIONOR
};

struct function_class_t {
	enum FunctionClass fclass;
	struct functionor_t * current_functionor;
	struct functionor_t * (*functionors)[];
};

extern struct functionor_t *
find_functionor(struct function_class_t * fclass, const char * param);

#define SET_STATIC_FUNCTIONOR(var, class, param)	do {	\
		struct functionor_t * ___c___;					\
		___c___ = (class).current_functionor;				\
		if (___c___ == NULL)								\
			___c___ = find_functionor(&(class), param);	\
		var = (typeof(var))(___c___);\
} while(0)

#endif
// vim:ts=4:sw=4

