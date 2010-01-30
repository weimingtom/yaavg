/* 
 * video.c
 * by WN @ Nov. 29, 2009
 */

#include <common/debug.h>
#include <common/exception.h>
#include <video/video.h>
#include <yconf/yconf.h>

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

// vim:ts=4:sw=4

