#include <config.h>
#include <common/debug.h>
#include <common/init_cleanup_list.h>
#include <common/dict.h>
#include <yconf/yconf.h>

#include <signal.h>

init_func_t init_funcs[] = {
	__dbg_init,
	__yconf_init,
	NULL,
};


cleanup_func_t cleanup_funcs[] = {
	__yconf_cleanup,
	__dbg_cleanup,
	NULL,
};

int
main()
{
	int x;
	const char * str;
	bool_t b;
	
	do_init();
	x = conf_get_int("XXXX", 12);
	VERBOSE(SYSTEM, "XXXX = %d\n", x);

	x = conf_get_int("video.mspf.fallback", 120);
	VERBOSE(SYSTEM, "video.mspf.fallback = %d\n", x);

	str = conf_get_string("video.screenshotdir", "/home");
	VERBOSE(SYSTEM, "video.screenshotdir = %s\n", str);

	conf_set_string("video.screenshotdir", "/home");
	str = conf_get_string("video.screenshotdir", "/tmp");
	VERBOSE(SYSTEM, "video.screenshotdir = %s\n", str);

	b = conf_get_bool("video.opengl.glx.grabkeyboard", FALSE);
	VERBOSE(SYSTEM, "video.opengl.glx.grabkeyboard = %d\n", b);

	do_cleanup();
	return 0;
}

// vim:ts=4:sw=4
