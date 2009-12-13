/* 
 * video.c
 * by WN @ Nov. 29, 2009
 */

#include <video/video.h>

extern struct functionor_t dummy_video_functionor;

static struct functionor_t * functionors[] = {
	&dummy_video_functionor,
	NULL,
};

struct function_class_t video_function_class = {
	.fclass = FC_VIDEO,
	.current_functionor = NULL,
	.functionors = &functionors,
};

// vim:ts=4:sw=4

