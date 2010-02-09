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
#include <common/mm.h>
#include <video/video.h>
#include <yconf/yconf.h>

static uint8_t * frontbuffer = NULL;
static uint8_t * backbuffer = NULL;

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
	generic_vid_init();

	int buffer_sz = CUR_VID->viewport.w * CUR_VID->viewport.h * 4;
	TRACE(VIDEO, "alloc 2 * %d bytes\n", buffer_sz);
	frontbuffer = xcalloc(1, buffer_sz);
	assert(frontbuffer != NULL);
	backbuffer = xcalloc(1, buffer_sz);
	assert(backbuffer != NULL);
	/* nothing to do */
}

static void
dummy_cleanup(void)
{
	DEBUG(VIDEO, "dummy video destroying\n");
	xfree_null(frontbuffer);
	xfree_null(backbuffer);
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

