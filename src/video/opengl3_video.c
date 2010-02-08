/* 
 * opengl3_video.c
 * by WN @ Feb. 08, 2010
 */

#include <config.h>
#include <common/debug.h>
#include <common/exception.h>
#include <video/video.h>

#if defined HAVE_OPENGL && defined USE_OPENGL3
struct video_functionor_t opengl3_video_functionor = {
	.fclass = FC_VIDEO,
};
#else
struct video_functionor_t opengl3_video_functionor = {
	.fclass = FC_VIDEO,
};
#endif

// vim:ts=4:sw=4

