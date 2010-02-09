/* 
 * by WN @ Feb. 09, 2010
 * generic_sdl.h
 */
#ifndef __VIDEO_GENERIC_SDL_H
#define __VIDEO_GENERIC_SDL_H

#include <config.h>
#include <common/defs.h>
#ifdef HAVE_SDL
#include <SDL/SDL.h>
#include <SDL/SDL_video.h>

void
init_sdl_video(bool_t use_opengl);

void
close_sdl_video(void);

#endif
#endif

// vim:ts=4:sw=4

