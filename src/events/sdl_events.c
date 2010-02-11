#include <config.h>

#ifdef HAVE_SDL

#include <common/debug.h>
#include <events/phy_events.h>
#include <SDL/SDL.h>

int
generic_sdl_poll_events(struct phy_event_t * evt)
{
	assert(evt != NULL);
	SDL_Event e;
	int ret = SDL_PollEvent(&e);

	/* form the evt */
	switch (e.type) {
		case SDL_QUIT:
			evt->type = EVENT_PHY_QUIT;
			break;
		case SDL_KEYDOWN:
			evt->type = EVENT_PHY_KEY_DOWN;
			evt->u.key.scancode = e.key.keysym.scancode;
			evt->u.key.mod = e.key.keysym.mod;
			break;
		case SDL_KEYUP:
			evt->type = EVENT_PHY_KEY_UP;
			evt->u.key.scancode = e.key.keysym.scancode;
			evt->u.key.mod = e.key.keysym.mod;
			break;
		default:
			evt->type = EVENT_NO_EVENT;
	}
	return ret;
}

#endif

// vim:ts=4:sw=4

