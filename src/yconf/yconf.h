/* 
 * yconf.h
 * by WN @ Nov. 24, 2009
 */

#ifndef __YCONF_H
#define __YCONF_H

#include <config.h>
#include <common/defs.h>
__BEGIN_DECLS

typedef enum _conf_type {
	TypeNone = 0,
	TypeInteger,
	TypeString,
	TypeFloat,
	TypeBool,
} conf_type_t;

extern bool_t
conf_get_bool(const char * key, bool_t def);
extern const char *
conf_get_string(const char * key, const char * def);
extern int
conf_get_int(const char * key, int def);
extern float
conf_get_float(const char * key, float def);


extern void
conf_set_string(const char * name, const char * v);
extern void
conf_set_integer(const char * name, int v);
extern void
conf_set_float(const char * name, float v);
extern void
conf_set_bool(const char * name, bool_t v);


#define max(a, b)	(((a) > (b)) ? (a) : (b))
#define min(a, b)	(((a) < (b)) ? (a) : (b))

#define max0(a, b)	max(a, b)
#define min0(a, b)	((((a) < (b)) && ((a) != 0)) ? (a) : (b))

__END_DECLS
#endif

