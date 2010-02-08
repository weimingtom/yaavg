/* 
 * opengl3_video.c
 * by WN @ Feb. 08, 2010
 */

#include <config.h>
#include <common/debug.h>
#include <common/exception.h>
#include <video/video.h>

#if defined HAVE_OPENGL
struct video_functionor_t opengl_video_functionor = {
};
#else
struct video_functionor_t opengl_video_functionor = {
	
};
#endif

// vim:ts=4:sw=4

