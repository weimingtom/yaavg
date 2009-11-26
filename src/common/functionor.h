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
	FC_DISPLAY,
	FC_OPENGL_ENGINE,
	FC_PIC_LOADER,
	NR_FCS,
};

#define BASE_FUNCTIONOR	\
	const char * name; 	\
	enum FunctionClass fclass;	\
	struct list_head list;	\
	bool_t (*check_usable)(const char * params);

struct functionor_t {
	BASE_FUNCTIONOR
};

struct function_class_t {
	enum FunctionClass fclass;
	struct list_head functionor_list;
	struct functionor_t * current_functionor;
};

#endif
// vim:ts=4:sw=4

