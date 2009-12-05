/* 
 * dummy_video.c
 * by WN @ Dec. 05, 2009
 */

#include <config.h>
#include <common/debug.h>
#include <video/video.h>
#include <yconf/yconf.h>
#include <string.h>

static bool_t
dummy_check_usable(const char * param)
{
	VERBOSE(VIDEO, "%s:%s\n", __func__, param);
	if (strncmp("dummy",
				conf_get_string("video.engine", NULL),
				sizeof("dummy")) == 0)
		return TRUE;
	return FALSE;
}

struct video_functionor_t dummy_video_functionor = {
	.name = "DummyVideo",
	.fclass = FC_VIDEO,
	.check_usable = dummy_check_usable,
	.command_list = LIST_HEAD_INIT(dummy_video_functionor.command_list),
	.reinit_hook_list = LIST_HEAD_INIT(dummy_video_functionor.reinit_hook_list),
};

// vim:ts=4:sw=4

