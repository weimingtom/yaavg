/* 
 * sdl_timer.c
 * by WN @ Dec. 05, 2009
 */
#include <SDL/SDL.h>
#include <utils/timer.h>
#include <yconf/yconf.h>

#ifdef HAVE_SDL

static bool_t
check_usable(const char * param ATTR_UNUSED)
{
	if (conf_get_bool("disableSDL", FALSE))
		return FALSE;
	return TRUE;
}

static void
delay(tick_t ms)
{
	/* SDL_Delay can be used even before SDL_Init */
	/* In some system, SDL_Delay can be interrupted by signal,
	 * in other system, can't... */
	SDL_Delay(ms);
}

static tick_t
get_ticks(void)
{
	return (tick_t)SDL_GetTicks();
}
#else

static bool_t
check_usable(const char * param ATTR_UNUSED)
{
	return FALSE;
}

static tick_t
get_ticks(void)
{
	return 0;
}

static void
delay(tick_t ms ATTR_UNUSED)
{
	return;
}
#endif

struct timer_functionor_t sdl_timer_functionor = {
	.name = "SDLTimer",
	.fclass = FC_TIMER,
	.check_usable = check_usable,
	.delay = delay,
	.get_ticks = get_ticks,
};
// vim:ts=4:sw=4

