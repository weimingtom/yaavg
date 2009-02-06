/* 
 * by WN @ Feb 05, 2009
 */
#include <stdint.h>
#include <SDL/SDL.h>

#include <common/defs.h>
#include <common/debug.h>
#include <common/utils.h>
/* 
 * Use SDL implentment utils.h
 */

void Delay(tick_t ms)
{
	/* SDL_Delay can be used even before SDL_Init */
	/* In some system, SDL_Delay can be interrupted by signal,
	 * in other system, can't... */
	SDL_Delay(ms);
}

tick_t GetTicks(void)
{
	return (tick_t)SDL_GetTicks();
}

