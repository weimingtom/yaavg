/* 
 * phy_events.h
 * by WN @ Dec. 12, 2009
 */

#ifndef __PHY_EVENTS_H
#define __PHY_EVENTS_H

#include <common/defs.h>
#include <stdint.h>

enum phy_event_type {
	PHY_NO_EVENTS = 0,
	RKEYBEGIN,
	PHY_KEY_DOWN,
	PHY_KEY_UP,
	RKEYEND,
	RMOUSEBEGIN,
	PHY_MOUSE_MOVE,
	PHY_MOUSE_UP,
	PHY_MOUSE_DOWN,
	RMOUSEEND,
	RSYSBEGIN,
	PHY_QUIT,
	RSYSEND,
	RVIDEOBEGIN,
	PHY_VIDEO_ACTIVE,
	PHY_VIDEO_EXPOSE,
	PHY_VIDEO_RESIZE,
	RVIDEOEND,
};

#define IS_XX_EVENT(x, e)	(((R##x##BEGIN) < (e)) && ((R##x##END) > (e)))
#define IS_KEY_EVENT(e)	IS_XX_EVENT(KEY, e)
#define IS_MOUSE_EVENT(e)	IS_XX_EVENT(MOUSE, e)
#define IS_SYS_EVENT(e)	IS_XX_EVENT(SYS, e)
#define IS_VIDEO_EVENT IS_XX_EVENT(VIDEO, e)

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
	enum phy_event_type type;
	uint8_t scancode;
	enum key_mod modifier;
	uint16_t ascii_code;
};

#define IS_LEFT_PRESSED(s)	((s) & 0x01)
#define IS_RIGHT_PRESSED(s)	((s) & 0x02)
#define IS_B3_PRESSED(s)	((s) & 0x04)
#define IS_B4_PRESSED(s)	((s) & 0x08)

struct mouse_event {
	enum phy_event_type type;
	uint8_t state;	/* mouse button states */
	uint16_t x, y;
	/* mouse acceleration */
	int16_t dx, dy;
};


struct active_event {
	enum phy_event_type type;
	bool_t active;
	uint8_t state;
};

struct resize_event {
	enum phy_event_type type;
	int w;
	int h;
};

union _phy_event {
	enum phy_event_type type;
	struct key_event key;
	struct mouse_event mouse;
	struct active_event active;
	struct resize_event resize;
};

struct phy_event {
	union _phy_event u;
};

static void inline print_event(struct phy_event * e)
{
	return;
}

#endif

// vim:ts=4:sw=4

