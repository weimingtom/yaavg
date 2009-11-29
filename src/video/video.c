/* 
 * video.c
 * by WN @ Nov. 29, 2009
 */

#include <video/video.h>

struct function_class_t display_function_class = {
	.fclass = FC_DISPLAY,
	.functionor_list = LIST_HEAD_INIT(display_function_class.functionor_list),
	.current_functionor = NULL,
};


// vim:ts=4:sw=4

