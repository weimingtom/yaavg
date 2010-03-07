/* 
 * video.c
 * by WN @ Nov. 29, 2009
 */

#include <common/debug.h>
#include <common/exception.h>
#include <video/video.h>
#include <yconf/yconf.h>
#include <utils/rect.h>

extern struct functionor_t dummy_video_functionor;

#ifdef USE_OPENGL3
extern struct functionor_t opengl3_video_functionor;
#endif
#ifdef HAVE_OPENGL
extern struct functionor_t opengl_video_functionor;
#endif
#ifdef HAVE_SDL
extern struct functionor_t sdl_video_functionor;
#endif


static struct functionor_t * functionors[] = {
#ifdef USE_OPENGL3
	&opengl3_video_functionor,
#endif
#ifdef HAVE_OPENGL
	&opengl_video_functionor,
#endif
#ifdef HAVE_SDL
	&sdl_video_functionor,
#endif
	&dummy_video_functionor,
	NULL,
};

struct function_class_t video_function_class = {
	.fclass = FC_VIDEO,
	.current_functionor = NULL,
	.functionors = &functionors,
};

static void
check_set_viewport(void)
{
	assert(CUR_VID != NULL);

	int res_w, res_h, vp_x, vp_y, vp_w, vp_h;
	
	sscanf(conf_get_string("video.resolution", "800x600"), "%dx%d",
			&res_w, &res_h);
	sscanf(conf_get_string("video.viewport", "(0,0,800,600)"),
			"(%d,%d,%d,%d)", &vp_x, &vp_y, &vp_w, &vp_h);

	struct rect_t screen = {0, 0, res_w, res_h};
	struct rect_t viewport = {vp_x, vp_y, vp_w, vp_h};

	if ((res_w <= 0) || (res_h <= 0))
		THROW(EXP_CORRUPTED_CONF, "resolution: " RECT_FMT, RECT_ARG(screen));
	if ((vp_x < 0) || (vp_y < 0) || (vp_w <= 0) || (vp_h <= 0))
		THROW(EXP_CORRUPTED_CONF, "viewport: " RECT_FMT,
				RECT_ARG(viewport));

	struct rect_t inter = rects_intersect(screen, viewport);
	if (!(rects_same(&inter, &viewport)))
		THROW(EXP_CORRUPTED_CONF, "resolution: " RECT_FMT ", viewport: " RECT_FMT,
				RECT_ARG(screen), RECT_ARG(viewport));
	CUR_VID->viewport = inter;
	CUR_VID->width = res_w;
	CUR_VID->height = res_h;
}

void
generic_vid_init(void)
{
	assert(CUR_VID != NULL);
	DEBUG(VIDEO, "init video:\n");
	check_set_viewport();

	CUR_VID->resizable = conf_get_bool("video.resizable", FALSE);
	CUR_VID->fullscreen = conf_get_bool("video.fullscreen", FALSE);
	CUR_VID->grabinput = conf_get_bool("video.grabinput", FALSE);
	CUR_VID->bpp = conf_get_int("video.bpp", 32);

	DEBUG(VIDEO, "\tresolution: %dx%d\n", CUR_VID->width, CUR_VID->height);
	DEBUG(VIDEO, "\tviewport: " RECT_FMT "\n", RECT_ARG(CUR_VID->viewport));
	DEBUG(VIDEO, "\tfullscreen: %d\n", CUR_VID->fullscreen);
	DEBUG(VIDEO, "\tgrabinput: %d\n", CUR_VID->grabinput);
	DEBUG(VIDEO, "\tbpp: %d\n", CUR_VID->bpp);

	/* nothing to do */
}

void
video_init(void)
{
	VERBOSE(VIDEO, "video initing\n");
	const char * eng_str = conf_get_string("video.engine", "opengl3");
	find_functionor(&video_function_class, eng_str);
	assert(CUR_VID != NULL);
	if ((void*)CUR_VID == (void*)&dummy_video_functionor)
		WARNING(VIDEO, "doesn't support video engine %s, use %s instead\n",
				eng_str, CUR_VID->name);
	VERBOSE(VIDEO, "selected video engine: %s\n", CUR_VID->name);
	generic_vid_init();
	vid_init();
}

void
video_cleanup(void)
{
	VERBOSE(VIDEO, "video cleanuping\n");
	vid_cleanup();
}


// vim:ts=4:sw=4

