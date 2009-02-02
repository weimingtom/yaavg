/* by WN @ Jan 29, 2009 */
/* Throw aray config subsystem definitions */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <common/defs.h>

__BEGIN_DECLS

typedef enum _ConfType {
	TypeNone = 0,
	TypeInteger,
	TypeString,
	TypeFloat,
} ConfType;

struct ConfigEntry {
	const char * name;
	ConfType type;
	union _conf_value{
		char * s;
		int i;
		float f;
	} value;
};

extern char * ConfGetString(const char * name);
extern int ConfGetInteger(const char * name);
extern float ConfGetFloat(const char * name);

__END_DECLS

#endif

