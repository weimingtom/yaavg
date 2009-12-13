/* 
 * video.h
 * by WN @ Nov. 27, 2009
 */

#ifndef __YAAVG_VIDEO_H
#define __YAAVG_VIDEO_H

#include <config.h>
#include <common/defs.h>
#include <common/functionor.h>
#include <events/phy_events.h>

extern struct function_class_t
video_function_class;

struct video_functionor_t {
	BASE_FUNCTIONOR
	struct {
		int x, y, w, h;
	} view_port;
	int weight;
	int height;
	struct list_head command_list;
	struct list_head reinit_hook_list;
	bool_t full_screen;

	void (*set_caption)(const char *);
	void (*set_icon)(const char *);
	void (*set_mouse_pos)(float x, float y);
	void (*set_mouse_pos_int)(int x, int y);

	void (*init)(void);
	void (*reinit)(void);
	void (*destroy)(void);
	void (*reshape)(int w, int h);

	void (*render_begin)(void);
	void (*render_end)(void);
	void (*swap_buffer)(void);
	void (*toggle_full_screen)(void);
	void (*screenshot)(void);

	/* fill the phy_event struct. if return 0, there's no events left */
	int (*poll_events)(struct phy_event * e);
};

#define VID_FUNC(x, f, args...)	do {if ((x) && ((x)->(f))) (x)->(f)(args);} while(0)
#define VID_set_caption(x, v)	VID_FUNC((x), set_caption, v)
#define VID_set_icon(x, v)		VID_FUNC((x), set_icon, v)
#define VID_set_mouse_pos(x, a, b)	VID_FUNC((x), set_mouse_pos, a, b)
#define VID_set_mouse_pos_int(x, a, b)		VID_FUNC((x), set_mouse_pos_int, a, b)
#define VID_init(x)	VID_FUNC((x), init)
#define VID_reinit(x)	VID_FUNC((x), reinit)
#define VID_destroy(x)	VID_FUNC((x), destroy)
#define VID_reshape(x, w, h)	VID_FUNC((x), reshape, w, h)
#define VID_render_begin(x)	VID_FUNC((x), render_begin)
#define VID_render_end(x)	VID_FUNC((x), render_end)
#define VID_swap_buffer(x)	VID_FUNC((x), swap_buffer)
#define VID_toggle_full_screen(x)	VID_FUNC((x), toggle_full_screen)
#define VID_screenshot(x)			VID_FUNC((x), screenshot)
#define VID_poll_events(x)			VID_FUNC((x), poll_events)


#endif

// vim:ts=4:sw=4

