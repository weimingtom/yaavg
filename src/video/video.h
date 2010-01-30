/* 
 * video.h
 * by WN @ Nov. 27, 2009
 */

#ifndef __YAAVG_VIDEO_H
#define __YAAVG_VIDEO_H

#include <config.h>
#include <common/defs.h>
#include <common/functionor.h>
#include <bitmap/bitmap.h>
#include <events/phy_events.h>
#include <assert.h>
extern struct function_class_t
video_function_class;

struct video_functionor_t {
	BASE_FUNCTIONOR
	struct {
		int x, y, w, h;
	} viewport;
	int width;
	int height;
	struct list_head * command_list;
	struct list_head * reinit_hook_list;
	bool_t fullscreen;

	void (*set_caption)(const char *);
	void (*set_icon)(const char *);
	void (*set_mouse_pos)(float x, float y);
	void (*set_mouse_pos_int)(int x, int y);

	void (*init)(void);
	void (*reinit)(void);
	void (*destroy)(void);
	void (*reshape)(int w, int h);

	void (*begin_frame)(void);
	void (*render_frame)(void);
	void (*end_frame)(void);
	void (*swapbuffer)(void);
	void (*toggle_full_screen)(void);
	struct bitmap_t * (*screenshot)(void);

	/* fill the phy_event struct. if return 0, there's no events left */
	int (*poll_events)(struct phy_event * e);
};

#define CUR_VID	((struct video_functionor_t*)(video_function_class.current_functionor))

extern void
prepare_video(void);

void
common_video_init(void);

#define VID_VOIDFUNC(f, ...) do { \
	assert(CUR_VID != NULL);	\
	if (CUR_VID->f) 			\
		CUR_VID->f(__VA_ARGS__);		\
} while(0)

#define VID_FUNC(f, def, ...) ({	\
		typeof(def) ___ret___;	\
		assert(CUR_VID != NULL);\
		if (CUR_VID->f)	\
			___ret___ = CUR_VID->f(__VA_ARGS__);	\
		else													\
			___ret___ = def;									\
		___ret___;												\
})

static void inline
set_video_command_list(struct list_head * l)
{
	assert(CUR_VID != NULL);
	CUR_VID->command_list = l;
}

static void inline
set_video_reinit_hook_list(struct list_head * l)
{
	assert(CUR_VID != NULL);
	CUR_VID->reinit_hook_list = l;
}

#define video_set_caption(v)	VID_VOIDFUNC(set_caption, (v))
#define video_init()			VID_VOIDFUNC(init)
#define video_destroy()			VID_VOIDFUNC(destroy)
#define video_begin_frame()		VID_VOIDFUNC(begin_frame)
#define video_render_frame()	VID_VOIDFUNC(render_frame)
#define video_end_frame()		VID_VOIDFUNC(end_frame)
#define video_swapbuffer()		VID_VOIDFUNC(swapbuffer)
#define video_poll_events(e)		VID_FUNC(poll_events, 0, e)

#if 0
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
#endif

// vim:ts=4:sw=4

