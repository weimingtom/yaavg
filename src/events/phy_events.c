/* 
 * phy_events.c
 * by WN @ Feb. 10, 2010
 */

#include <common/debug.h>
#include <common/exception.h>
#include <events/phy_events.h>


void
print_event(struct phy_event_t * e)
{
	switch (e->type) {
		case EVENT_PHY_QUIT:
			TRACE(EVENT, "QUIT\n");
			break;
		case EVENT_PHY_KEY_DOWN:
			TRACE(EVENT, "key down, scancode=0x%x, mod=0x%x\n",
					e->u.key.scancode, e->u.key.mod);
			break;
		case EVENT_PHY_KEY_UP:
			TRACE(EVENT, "key up, scancode=0x%x, mod=0x%x\n",
					e->u.key.scancode, e->u.key.mod);
			break;
		default:
			break;
	}
	return;
}

// vim:ts=4:sw=4

