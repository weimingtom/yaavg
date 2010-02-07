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

static struct functionor_t * functionors[] = {
#if 0
	&opengl3_video_functionor,
	&opengl_video_functionor,
	&sdl_video_functionor,
	&dummy_video_functionor,
#endif
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
	sscanf(conf_get_string("video.viewport", "(0,0,800,600)"), "(%d,%d,%d,%d)",
			&vp_x, &vp_y, &vp_w, &vp_h);

	struct rect_t screen = {0, 0, res_w, res_h};
	struct rect_t viewport = {0, 0, res_w, res_h};

	if ((res_w <= 0) || (res_h <= 0))
		THROW(EXP_CORRUPTED_CONF, "resolution: " RECT_FMT, RECT_ARG(screen));
	if ((vp_x < 0) || (vp_y < 0) || (vp_w <= 0) || (vp_h <= 0))
		THROW(EXP_CORRUPTED_CONF, "viewport: " RECT_FMT,
				RECT_ARG(viewport));

	struct rect_t inter =  rects_intersect(screen, viewport);
	if (!(rects_same(inter, viewport)))
		THROW(EXP_CORRUPTED_CONF, "resolution: " RECT_FMT ", viewport: " RECT_FMT,
				RECT_ARG(screen), RECT_ARG(viewport));
	CUR_VID->viewport = viewport;
	CUR_VID->width = res_w;
	CUR_VID->height = res_h;
}

void
generic_video_init(void)
{
	assert(CUR_VID != NULL);
	DEBUG(VIDEO, "init video:");
	check_set_viewport();
	CUR_VID->fullscreen = conf_get_bool("video.fullscreen", FALSE);
	DEBUG(VIDEO, "\tresolution: %dx%d\n", CUR_VID->width, CUR_VID->height);
	DEBUG(VIDEO, "\tviewport: " RECT_FMT "\n", RECT_ARG(CUR_VID->viewport));
	DEBUG(VIDEO, "\tfullscreen: %d\n", CUR_VID->fullscreen);
	/* nothing to do */
}

#if 0
void
prepare_video(void)
{
	const char * engine = conf_get_string("video.engine", "opengl3");
	struct functionor_t * __vf = find_functionor(&video_function_class,
			engine);

	assert(__vf != NULL);
	if (__vf == &dummy_video_functionor)
		WARNING(VIDEO, "we are using dummy video engine\n");

	struct video_functionor_t * vf = (struct video_functionor_t*)__vf;

	DEBUG(VIDEO, "find video functionor \"%s\" for engine \"%s\"\n",
			vf->name, engine);
	assert(vf == CUR_VID);
}

void
common_video_init(void)
{
	int res_w, res_h, vp_x, vp_y, vp_w, vp_h;
	sscanf(conf_get_string("video.resolution", "800x600"), "%dx%d",
			&res_w, &res_h);
	sscanf(conf_get_string("video.viewport", "(0,0,800,600)"), "(%d,%d,%d,%d)",
			&vp_x, &vp_y, &vp_w, &vp_h);

	/* clamp the screen size */
	res_w = (res_w > 640) ? res_w : 640;
	res_h = (res_h > 480) ? res_h : 480;
	DEBUG(VIDEO, "\tresolution: %dx%d\n", res_w, res_h);
	DEBUG(VIDEO, "\tviewport: (%d,%d,%d,%d)\n", vp_x, vp_y, vp_w, vp_h);

	CUR_VID->viewport.x = vp_x;
	CUR_VID->viewport.y = vp_y;
	CUR_VID->viewport.w = vp_w;
	CUR_VID->viewport.h = vp_h;
	CUR_VID->width = res_w;
	CUR_VID->height = res_h;
	CUR_VID->fullscreen = conf_get_bool("video.fullscreen", FALSE);
	DEBUG(VIDEO, "\tfullscreen: %d\n", CUR_VID->fullscreen);
	CUR_VID->command_list = NULL;
	CUR_VID->reinit_hook_list = NULL;
}
#endif
// vim:ts=4:sw=4

