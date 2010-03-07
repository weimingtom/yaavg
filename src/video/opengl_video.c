/* 
 * opengl3_video.c
 * by WN @ Feb. 08, 2010
 */

#include <config.h>
#include <common/debug.h>
#include <common/exception.h>
#include <common/dict.h>
#include <common/bithacks.h>
#include <common/cache.h>
#include <yconf/yconf.h>
#include <video/video.h>
#include <video/gl_driver.h>
#include <video/generic_opengl.h>
#include <video/dynamic_opengl/dynamic_opengl.h>
#include <video/dynamic_opengl/opengl_funcs.h>
#include <video/opengl_texture.h>

#include <stdarg.h>
#include <stdio.h>

#if defined HAVE_OPENGL

static bool_t
gl_check_usable(const char * param)
{
	VERBOSE(OPENGL, "%s:%s\n", __func__, param);
	if (strcasecmp(param, "opengl") == 0)
		return TRUE;
	return FALSE;
}

static void
reset_hints(void)
{
	/* set all hints to nicest */
	if (GL_full_version < MKVER(3, 0)) {
		/* these hints are deprecated in opengl 3, and will cause
		 * error in forward compatible context. However, in
		 * opengl_video, we are not using such context. */
		gl(Hint, GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
		gl(Hint, GL_POINT_SMOOTH_HINT, GL_NICEST);
		gl(Hint, GL_FOG_HINT, GL_NICEST);

		if (GL_full_version >= MKVER(1, 4))
			gl(Hint, GL_GENERATE_MIPMAP_HINT, GL_NICEST);
	}

	if (GL_full_version >= MKVER(1, 3))
		gl(Hint, GL_TEXTURE_COMPRESSION_HINT, GL_NICEST);

	gl(Hint, GL_LINE_SMOOTH_HINT, GL_NICEST);
	gl(Hint, GL_POLYGON_SMOOTH_HINT, GL_NICEST);

	if (GL_full_version >= MKVER(2, 0))
		gl(Hint, GL_FRAGMENT_SHADER_DERIVATIVE_HINT, GL_NICEST);

	/* turn off edge flag */
	gl(EdgeFlag, GL_FALSE);
}

static void
reshape(void)
{
	struct rect_t vp = CUR_VID->viewport;
	DEBUG(OPENGL, "viewport: (%d, %d, %d, %d)\n",
			vp.x, vp.y, vp.w, vp.h);
	gl(Viewport, vp.x, vp.y, vp.w, vp.h);
	if (GL_major_version < 3) {
		/* revert y axis */
		gl(MatrixMode, GL_PROJECTION);
		gl(LoadIdentity);
		gl(Ortho, -1, 1, -1, 1, -1, 1);

		gl(MatrixMode, GL_MODELVIEW);
		gl(LoadIdentity);
	}
}

static void
gl_init(void)
{
	DEBUG(OPENGL, "init opengl video\n");

	/* find the opengl driver */
	const char * drv_str = conf_get_string("video.opengl.driver", "sdl");
	find_functionor(&gl_driver_function_class, drv_str);
	assert(CUR_DRV != NULL);

	/* set to 0 to indicate we are using lower version of opengl */
	CUR_DRV->major_version = 0;
	drv_init();

#define set_func(x)	CUR_VID->x = CUR_DRV->x
	set_func(set_caption);
	set_func(set_icon);
	set_func(set_mouse_pos);
	set_func(set_mouse_pos_int);
	set_func(reinit);
	set_func(reshape);
	set_func(render_frame);
	set_func(swapbuffer);
	set_func(poll_events);
	set_func(toggle_fullscreen);
	set_func(swapbuffer);
#undef set_func

	/* init opengl information */
	assert(CUR_DRV->get_proc_address != NULL);

	init_func_list(CUR_DRV->get_proc_address, __gl_func_map);
	check_opengl_features();

	reset_hints();
	GL_POP_ERROR();

	gl(PixelStorei, GL_UNPACK_ALIGNMENT, UNPACK_ALIGNMENT);
	gl(Enable, GL_BLEND);

	gl(ClearColor, 0.0, 0.0, 0.0, 0.0);
	gl(Clear, GL_COLOR_BUFFER_BIT);
	GL_POP_ERROR();

	reshape();
	GL_POP_ERROR();


	/* init the txarray_cache */
	opengl_texture_cache_init();

	return;
}

static void
gl_cleanup(void)
{
	DEBUG(OPENGL, "closing opengl video\n");

	opengl_texture_cache_cleanup();
	cleanup_opengl_features();
	drv_cleanup();


	return;
}

static void
gl_test_screen(const char * b)
{
	gl(Clear, GL_COLOR_BUFFER_BIT);

	gl(Color4f, 1.0, 1.0, 1.0, 1.0);

	static GLfloat axis[] = {
		-1.0, 0,
		1.0, 0,
		0, -1.0,
		0, 1.0,
		0, 0,
		0.1, 0.5,
	};

	gl(EnableClientState, GL_VERTEX_ARRAY);
	gl(VertexPointer, 2, GL_FLOAT, 0, axis);
	gl(DrawArrays, GL_LINES, 0, sizeof(axis) / sizeof(GLfloat));
	gl(DisableClientState, GL_VERTEX_ARRAY);

	static struct vec3 pvecs[4] = {
		[0] = {-1.0,  1.0, 0.0},
		[1] = {-1.0, -1.0, 0.0},
		[2] = { 1.0, -1.0, 0.0},
		[3] = { 1.0,  1.0, 0.0},
	};

	static struct vec3 tvecs[4] = {
		[0] = { 0.0, 0.0, 0.0},
		[1] = { 0.0, 1, 0.0},
		[2] = { 1,  1, 0.0},
		[3] = { 1, 0.0, 0.0},
	};
	draw_texture(b, pvecs, tvecs,
			GL_LINEAR, GL_LINEAR,
			GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

	GL_POP_ERROR();
}

struct video_functionor_t opengl_video_functionor = {
	.name = "OpenGLVideo",
	.fclass = FC_VIDEO,
	.check_usable = gl_check_usable,
	.init = gl_init,
	.cleanup = gl_cleanup,
	.test_screen = gl_test_screen,
	/* redirect to driver's poll_events */
	.poll_events = NULL,
	.toggle_fullscreen = NULL,
	.swapbuffer = NULL,
};

#else

static bool_t
gl_check_usable(const char * param)
{
	VERBOSE(OPENGL, "%s:%s\n", __func__, param);
	return FALSE;
}

struct video_functionor_t opengl_video_functionor = {
	.name = "OpenGLVideo",
	.fclass = FC_VIDEO,
	.check_usable = gl_check_usable,
};
#endif

// vim:ts=4:sw=4

