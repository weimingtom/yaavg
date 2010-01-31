/* 
 * dummy_video.c
 * by WN @ Dec. 05, 2009
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include <config.h>
#include <common/debug.h>
#include <video/video.h>
#include <yconf/yconf.h>


static bool_t
dummy_check_usable(const char * param)
{
	VERBOSE(VIDEO, "%s:%s\n", __func__, param);
	return TRUE;
}

static void
dummy_init(void)
{
	VERBOSE(VIDEO, "initing dummy video engine:\n");
	common_video_init();

	/* nothing to do */
}

static void
dummy_cleanup(void)
{
	DEBUG(VIDEO, "dummy video destroying\n");
}

static int
dummy_poll_events(struct phy_event * e)
{
	int err;
	struct timeval tv;
	fd_set set;

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	FD_ZERO(&set);
	FD_SET(STDIN_FILENO, &set);
	err = TEMP_FAILURE_RETRY(
			select(FD_SETSIZE, &set,
				NULL, NULL,
				&tv));
	if (err == 0)
		return 0;
	
	assert(err == 1);
	e->u.type = PHY_QUIT;
	return 1;
}

struct video_functionor_t dummy_video_functionor = {
	.name = "DummyVideo",
	.fclass = FC_VIDEO,
	.check_usable = dummy_check_usable,
	.init = dummy_init,

	.cleanup = dummy_cleanup,
	.command_list = NULL,
	.reinit_hook_list = NULL,

	.poll_events = dummy_poll_events,
};

// vim:ts=4:sw=4

