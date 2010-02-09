#include <common/defs.h>
#include <common/debug.h>
#include <common/exception.h>
#include <common/init_cleanup_list.h>
#include <common/dict.h>
#include <common/mm.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>

init_func_t init_funcs[] = {
	__dbg_init,
	NULL,
};


cleanup_func_t cleanup_funcs[] = {
	__dbg_cleanup,
	NULL,
};


struct kvp {
	char * key;
	char * value;
};

#define DEFE(a, b)	{a, #b}

static struct kvp kvps[] = {
DEFE("video.resolution.w", 800),
DEFE("video.resolution.h", 600),
DEFE("video.viewport.w", 800),
DEFE("video.viewport.h", 600),
DEFE("video.mspf.fallback", 100),
DEFE("video.mspf", 17),
DEFE("video.fullscreen", FALSE),
DEFE("video.gl.glx.fullscreen.engine", 3),
DEFE("video.resizable", FALSE),
DEFE("video.opengl.gllibrary", NULL),
DEFE("video.opengl.gl3context", TRUE),
DEFE("video.opengl.gl3forwardcompatible", FALSE),
DEFE("video.opengl.bpp", 32),
DEFE("video.opengl.swapcontrol", 0),
DEFE("video.opengl.multisample", 0),
DEFE("video.screenshotdir", "/tmp"),
DEFE("video.caption", "test"),
DEFE("video.sdl.blocksigint", TRUE),
DEFE("sys.resource.cachesz",	1048576),
DEFE("video.opengl.texture.totalhwsize", 0),
DEFE("video.opengl.texture.maxsize", 0),
DEFE("video.opengl.texture.enableCOMPRESSION", TRUE),
DEFE("video.opengl.texture.enableNPOT", TRUE),
DEFE("video.opengl.texture.enableRECT", TRUE),
DEFE("video.opengl.glx.confinemouse", FALSE),
DEFE("video.opengl.glx.grabkeyboard", FALSE),
{NULL, NULL},
};

int main()
{
	dbg_init(NULL);

	catch_var(struct dict_t *, strdict, NULL);
	define_exp(exp);
	TRY(exp) {

		set_catched_var(strdict, strdict_create(sizeof(kvps) / sizeof(kvps[0]) - 1,
					STRDICT_FL_DUPKEY | STRDICT_FL_DUPDATA));
		assert(strdict != NULL);

		struct kvp * pkvp = &kvps[0];
		while (pkvp->key != NULL) {
			dict_data_t x;
			x.str = pkvp->value;
			strdict_insert(strdict, pkvp->key, x);
			pkvp ++;
		}

		/* iterate over the dict */
		struct dict_entry_t * ep = dict_get_next(strdict, NULL);
		while (ep != NULL) {
			WARNING(SYSTEM, "key=%s, value=%s\n",
					(char*)ep->key, (char*)ep->data.str);
			ep = dict_get_next(strdict, ep);
		}

		WARNING(SYSTEM, "----------------------------------\n");
		pkvp = &kvps[0];
		while (pkvp->key != NULL) {
			WARNING(SYSTEM, "key=%s, value=%s\n",
					pkvp->key, strdict_get(strdict, pkvp->key).str);
			pkvp ++;
		}

	} FINALLY {
		get_catched_var(strdict);
		strdict_destroy(strdict);
	} CATCH(exp) {
		switch (exp.type) {
			default:
				RETHROW(exp);
		}
	}

	do_cleanup();
	return 0;
}

// vim:ts=4:sw=4
