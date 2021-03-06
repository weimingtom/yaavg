/* 
 * by WN @ Feb. 09, 2010
 * generic_sdl.h
 */
#ifndef __VIDEO_GENERIC_SDL_VIDEO_H
#define __VIDEO_GENERIC_SDL_VIDEO_H

#include <config.h>
#include <common/defs.h>
#ifdef HAVE_SDL
#include <SDL/SDL.h>
#include <SDL/SDL_video.h>

SDL_Surface *
generic_init_sdl_video(bool_t use_opengl);

void
generic_destroy_sdl_video(void);

void
generic_sdl_toggle_fullscreen(void);

#endif
#endif

// vim:ts=4:sw=4

