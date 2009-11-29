/* 
 * video.h
 * by WN @ Nov. 27, 2009
 */

#ifndef __YAAVG_VIDEO_H
#define __YAAVG_VIDEO_H

#include <config.h>
#include <common/defs.h>
#include <common/functionor.h>

extern struct function_class_t
display_function_class;

struct display_functionor {
	BASE_FUNCTIONOR
	const char * name;
	struct list_head reinit_hook_list;
	struct list_head command_list;
	struct {
		int x, y, w, h;
	} view_port;
	int weight;
	int height;
	bool_t full_screen;
	void (*init)(void);
	void (*reinit)(void);
	void (*destroy)(void);
	void (*reshape)(int w, int h);
	void (*render)(void);
	void (*swap_buffer)(void);
	void (*toggle_full_screen)(void);
	void (*screenshot)(void);
	void (*set_caption)(const char *);
	void (*set_icon)(const char *);
	void (*set_mouse_pos)(float x, float y);
	void (*set_mouse_pos_int)(int x, int y);
};

#endif

// vim:ts=4:sw=4

