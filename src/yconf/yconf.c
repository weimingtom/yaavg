/* 
 * yconf.c
 * by WN @ Nov. 24, 2009
 */

#include <config.h>
#include <common/defs.h>
#include <common/dict.h>
#include <common/init_cleanup_list.h>
#include <yconf/yconf.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

typedef enum {
	TypeNone = 0,
	TypeInteger,
	TypeString,
	TypeFloat,
	TypeBool,
} conf_type_t;

struct kvp {
	const char * key;
	conf_type_t type;
	dict_data_t value;
};

#define ENTRY_INT(a, b)	{a, TypeInteger, (dict_data_t)(int)b}
#define ENTRY_STR(a, b)	{a, TypeString, (dict_data_t)(const char *)b}
#define ENTRY_FLT(a, b)	{a, TypeFloat, (dict_data_t)(float)b}
#define ENTRY_BOL(a, b)	{a, TypeFloat, (dict_data_t)(bool_t)b}
static struct kvp kvps[] = {
ENTRY_STR("video.engine", "dummy"),
ENTRY_BOL("disableSDL", FALSE),
ENTRY_INT("timer.mspf.fallback", 100),
ENTRY_INT("timer.fps", 20),
ENTRY_INT("video.resolution.w", 800),
ENTRY_INT("video.resolution.h", 600),
ENTRY_INT("video.viewport.w", 800),
ENTRY_INT("video.viewport.h", 600),
ENTRY_BOL("video.fullscreen", FALSE),
ENTRY_INT("video.gl.glx.fullscreen.engine", 3),
ENTRY_BOL("video.resizable", FALSE),
ENTRY_STR("video.opengl.gllibrary", ""),
ENTRY_BOL("video.opengl.gl3context", TRUE),
ENTRY_BOL("video.opengl.gl3forwardcompatible", FALSE),
ENTRY_INT("video.opengl.bpp", 32),
ENTRY_INT("video.opengl.swapcontrol", 0),
ENTRY_INT("video.opengl.multisample", 0),
ENTRY_STR("video.screenshotdir", "/tmp"),
ENTRY_STR("video.caption", "test"),
ENTRY_BOL("video.sdl.blocksigint", TRUE),
ENTRY_INT("sys.mem.threshold",	1048576),
ENTRY_INT("video.opengl.texture.totalhwsize", 0),
ENTRY_INT("video.opengl.texture.maxsize", 0),
ENTRY_BOL("video.opengl.texture.enableCOMPRESSION", TRUE),
ENTRY_BOL("video.opengl.texture.enableNPOT", TRUE),
ENTRY_BOL("video.opengl.texture.enableRECT", TRUE),
ENTRY_BOL("video.opengl.glx.confinemouse", FALSE),
ENTRY_BOL("video.opengl.glx.grabkeyboard", FALSE),
{NULL, TypeNone, {NULL}},
};

static struct dict_t * conf_dict = NULL;

void
__yconf_cleanup(void)
{
	if (conf_dict != NULL) {
		strdict_destroy(conf_dict);
		conf_dict = NULL;
	}
}

void
__yconf_init(void)
{
	if (conf_dict == NULL) {
		conf_dict = strdict_create(sizeof(kvps) / sizeof(struct kvp),
				STRDICT_FL_FIXED);
		assert(conf_dict != NULL);
	}

	struct kvp * p = &kvps[0];
	while (p->key != NULL) {
		strdict_insert(conf_dict, p->key, p->value);
		p ++;
	}
}

#define getv(x)	dict_data_t x = strdict_get(conf_dict, key);	\
	if (GET_DICT_DATA_FLAGS(x) & DICT_DATA_FL_VANISHED)	{		\
		TRACE(YCONF, "conf_dict doesn't contain key %s, return default value\n", key); \
		return def; }

#define DEFINE_GET_FUNC(t1, t2, t3)	\
	t1 conf_get_##t2(const char * key, t1 def) { \
		TRACE(YCONF, "getting confkey %s as "#t2"\n", key);	\
		getv(d);								\
		return d.t3;							\
	}
#define DEFINE_SET_FUNC(t1, t2)	\
void conf_set_##t2(const char * key, t1 v) { \
	assert(key != NULL);					\
	strdict_insert(conf_dict, key, (dict_data_t)v);\
	return;								\
}

#define DEFINE_GET_SET_FUNC(t1, t2, t3)	\
	DEFINE_GET_FUNC(t1, t2, t3)			\
	DEFINE_SET_FUNC(t1, t2)

/* in stdbool.h, bool is a defined symbol, and will be expand to _Bool. */
#ifdef bool
# undef bool
#endif
DEFINE_GET_SET_FUNC(bool_t, bool, bol)
DEFINE_GET_SET_FUNC(const char *, string, str)
DEFINE_GET_SET_FUNC(int, int, val)
DEFINE_GET_SET_FUNC(float, float, flt)

// vim:ts=4:sw=4
