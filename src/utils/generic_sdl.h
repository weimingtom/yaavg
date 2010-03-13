/* 
 * generic_sdl.h
 * by WN @ Feb. 08, 2010
 */

#ifndef __UTILS_GENERIC_SDL_H
#define __UTILS_GENERIC_SDL_H

#include <SDL/SDL.h>
#include <bitmap/bitmap.h>

void
generic_init_sdl(void);

void
generic_sdl_unblock_sigint(void);

void
fill_bitmap_by_sdlsurface(struct bitmap_t * h, SDL_Surface * img,
		const char * id, int id_sz);

#endif

