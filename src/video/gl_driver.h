/* 
 * gl_driver.h
 * by WN @ Feb. 11, 2010
 */

#ifndef __GL_DRIVER_H
#define __GL_DRIVER_H

#include <config.h>
#include <common/functionor.h>
#include <events/phy_events.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <video/dynamic_opengl/opengl_funcs.h>
#include <assert.h>

extern struct function_class_t
gl_driver_function_class;

struct gl_driver_functionor_t {
	BASE_FUNCTIONOR
	const GLubyte * vendor;
	const GLubyte * renderer;
	const GLubyte * version;
	const GLubyte * glsl_version;
	/* a table and a very long string */
	const GLubyte ** extensions;

	/* if set to 3 first, init opengl3 */
	int major_version;
	int minor_version;
	int full_version;

	int max_texture_size;
	bool_t texture_NPOT;
	bool_t texture_RECT;
	bool_t texture_COMPRESSION;

	void (*init_opengl_funcs)(void);

	/* same as video.h */
	void (*set_caption)(const char *);
	void (*set_icon)(const char *);
	void (*set_mouse_pos)(float x, float y);
	void (*set_mouse_pos_int)(int x, int y);

	void (*reinit)(void);
	void (*reshape)(int w, int h);

	void (*render_frame)(void);
	void (*swapbuffer)(void);

	/* fill the phy_event struct. if return 0, there's no events left */
	int (*poll_events)(struct phy_event_t * e);
};

#define CUR_DRV	((struct gl_driver_functionor_t*)(gl_driver_function_class.current_functionor))

#define DRV_VOIDFUNC(f, ...) do { \
	assert(CUR_DRV != NULL);	\
	if (CUR_DRV->f) 			\
		CUR_DRV->f(__VA_ARGS__);		\
} while(0)

#define DRV_FUNC(f, def, ...) ({	\
		typeof(CUR_DRV->f(__VA_ARGS__)) ___ret___;	\
		assert(CUR_DRV != NULL);\
		if (CUR_DRV->f)	\
			___ret___ = CUR_DRV->f(__VA_ARGS__);	\
		else													\
			___ret___ = def;									\
		___ret___;												\
})

#define drv_init()	DRV_VOIDFUNC(init)
#define drv_cleanup() DRV_VOIDFUNC(cleanup)

#endif

// vim:ts=4:sw=4

