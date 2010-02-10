#include <config.h>

#ifdef HAVE_SDL

#include <common/debug.h>
#include <events/phy_events.h>
#include <SDL/SDL.h>

int
generic_sdl_poll_events(struct phy_event_t * phe)
{
	assert(phe != NULL);
	SDL_Event e;
	int ret = SDL_PollEvent(&e);

	/* form the phe */
	switch (e.type) {
		case SDL_QUIT:
			phe->type = EVENT_PHY_QUIT;
			break;
		default:
			phe->type = EVENT_NO_EVENT;
	}
	return ret;
}

#endif

// vim:ts=4:sw=4

