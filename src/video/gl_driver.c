/* 
 * gl_driver.c
 * by WN @ Feb. 13, 2010
 */

#include <video/gl_driver.h>

extern struct functionor_t sdl_gl_driver_functionor;

static struct functionor_t * functionors[] = {
	&sdl_gl_driver_functionor,
	NULL,
};

struct function_class_t gl_driver_function_class = {
	.fclass = FC_OPENGL_DRIVER,
	.current_functionor = NULL,
	.functionors = &functionors,
};

// vim:ts=4:sw=4

