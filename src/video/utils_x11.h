/* 
 * by WN @ May 30, 2009
 */

#ifndef UTILS_X11_H
#define UTILS_X11_H

#include <config.h>

#ifdef VIDEO_OPENGL_GLX_DRIVER

#include <X11/Xlib.h>

void
XPrintDefaultError(Display * dpy, XErrorEvent * event);

#endif
#endif	/* VIDEO_OPENGL_GLX_DRIVER */
// vim:tabstop=4:shiftwidth=4

