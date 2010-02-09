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
#include <utils/rect.h>
#include <assert.h>
extern struct function_class_t
video_function_class;

struct video_functionor_t {
	BASE_FUNCTIONOR
	struct rect_t viewport;
	int width;
	int height;

	struct list_head * command_list;
	struct list_head * reinit_hook_list;
	bool_t fullscreen;

	void (*set_caption)(const char *);
	void (*set_icon)(const char *);
	void (*set_mouse_pos)(float x, float y);
	void (*set_mouse_pos_int)(int x, int y);

	void (*reinit)(void);
	void (*reshape)(int w, int h);

	void (*render_frame)(void);
	void (*swapbuffer)(void);
	void (*toggle_full_screen)(void);
	struct bitmap_t * (*screenshot)(void);

	/* fill the phy_event struct. if return 0, there's no events left */
	int (*poll_events)(struct phy_event * e);
};

#define CUR_VID	((struct video_functionor_t*)(video_function_class.current_functionor))

#define VID_VOIDFUNC(f, ...) do { \
	assert(CUR_VID != NULL);	\
	if (CUR_VID->f) 			\
		CUR_VID->f(__VA_ARGS__);		\
} while(0)

#define VID_FUNC(f, def, ...) ({	\
		typeof(f(__VA_ARGS___)) ___ret___;	\
		assert(CUR_VID != NULL);\
		if (CUR_VID->f)	\
			___ret___ = CUR_VID->f(__VA_ARGS__);	\
		else													\
			___ret___ = def;									\
		___ret___;												\
})

#define vid_set_caption(v)	VID_VOIDFUNC(set_caption, (v))
#define vid_init()			VID_VOIDFUNC(init)
#define vid_cleanup()			VID_VOIDFUNC(cleanup)
#define vid_begin_frame()		VID_VOIDFUNC(begin_frame)
#define vid_render_frame()	VID_VOIDFUNC(render_frame)
#define vid_end_frame()		VID_VOIDFUNC(end_frame)
#define vid_swapbuffer()		VID_VOIDFUNC(swapbuffer)
#define vid_poll_events(e)		VID_FUNC(poll_events, 0, e)


inline static void 
set_video_command_list(struct list_head * l)
{
	assert(CUR_VID != NULL);
	CUR_VID->command_list = l;
}

inline static void 
set_video_reinit_hook_list(struct list_head * l)
{
	assert(CUR_VID != NULL);
	CUR_VID->reinit_hook_list = l;
}

/* not an exported function, for video functionor used internally */
extern void
generic_video_init(void);

extern void
video_init(void);

extern void
video_cleanup(void);

#endif

// vim:ts=4:sw=4

