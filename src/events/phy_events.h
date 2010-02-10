/* 
 * phy_events.h
 * by WN @ Dec. 12, 2009
 */

#ifndef __PHY_EVENTS_H
#define __PHY_EVENTS_H

#include <common/defs.h>
#include <stdint.h>

enum phy_event_type {
	EVENT_NO_EVENT = 0,
	EVENT_keybegin,
	EVENT_PHY_KEY_DOWN,
	EVENT_PHY_KEY_UP,
	EVENT_keyend,
	EVENT_mousebegin,
	EVENT_PHY_MOUSE_MOVE,
	EVENT_PHY_MOUSE_UP,
	EVENT_PHY_MOUSE_DOWN,
	EVENT_mouseend,
	EVENT_sysbegin,
	EVENT_PHY_QUIT,
	EVENT_sysend,
	EVENT_videobegin,
	EVENT_PHY_VIDEO_ACTIVE,
	EVENT_PHY_VIDEO_EXPOSE,
	EVENT_PHY_VIDEO_RESIZE,
	EVENT_videoend,
};

#define IS_XX_EVENT(x, e)	(((EVENT_##x##begin) < (e)) && ((EVENT_##x##end) > (e)))
#define IS_KEY_EVENT(e)	IS_XX_EVENT(key, e)
#define IS_MOUSE_EVENT(e)	IS_XX_EVENT(mouse, e)
#define IS_SYS_EVENT(e)	IS_XX_EVENT(sys, e)
#define IS_VIDEO_EVENT IS_XX_EVENT(video, e)

/* same as SDL mod */
enum key_mod {
	KM_NONE  = 0x0000,
	KM_LSHIFT= 0x0001,
	KM_RSHIFT= 0x0002,
	KM_LCTRL = 0x0040,
	KM_RCTRL = 0x0080,
	KM_LALT  = 0x0100,
	KM_RALT  = 0x0200,
	KM_LMETA = 0x0400,
	KM_RMETA = 0x0800,
	KM_NUM   = 0x1000,
	KM_CAPS  = 0x2000,
	KM_MODE  = 0x4000,
	KM_RESERVED = 0x8000
};

struct key_event {
	uint8_t scancode;
	enum key_mod modifier;
	uint16_t ascii_code;
};

#define IS_LEFT_PRESSED(s)	((s) & 0x01)
#define IS_RIGHT_PRESSED(s)	((s) & 0x02)
#define IS_B3_PRESSED(s)	((s) & 0x04)
#define IS_B4_PRESSED(s)	((s) & 0x08)

struct mouse_event {
	uint8_t state;	/* mouse button states */
	uint16_t x, y;
	/* mouse acceleration */
	int16_t dx, dy;
};


struct active_event {
	bool_t active;
	uint8_t state;
};

struct resize_event {
	int w;
	int h;
};

union _phy_event {
	struct key_event key;
	struct mouse_event mouse;
	struct active_event active;
	struct resize_event resize;
};

struct phy_event_t {
	enum phy_event_type type;
	union _phy_event u;
};

void
print_event(struct phy_event_t * e);

#endif

// vim:ts=4:sw=4

