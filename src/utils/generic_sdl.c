/* 
 * utils/generic_sdl.c
 * by WN @ Feb. 08, 2010
 */

#include <config.h>
#ifdef HAVE_SDL

#include <utils/generic_sdl.h>
#include <yconf/yconf.h>
#include <SDL/SDL.h>

void
generic_init_sdl(void)
{
#if 0
	SDL_SetEventFilter(NULL);
#endif
	SDL_EnableUNICODE(SDL_ENABLE);
	int delay, interval;
	delay = conf_get_int("sys.events.keyrepeatdelay", -1);
	interval = conf_get_int("sys.events.keyrepeatinterval", -1);
	if (delay < 0)
		delay = SDL_DEFAULT_REPEAT_DELAY;
	if (interval < 0)
		interval = SDL_DEFAULT_REPEAT_INTERVAL;
	SDL_EnableKeyRepeat(delay, interval);
}


#endif

// vim:ts=4:sw=4

