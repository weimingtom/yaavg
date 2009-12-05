#include <config.h>
#include <common/debug.h>
#include <common/exception.h>
#include <common/dict.h>
#include <common/init_cleanup_list.h>
#include <common/functionor.h>
#include <video/video.h>

#include <signal.h>

init_func_t init_funcs[] = {
	__dbg_init,
	__dict_init,
	__yconf_init,
	NULL,
};


cleanup_func_t cleanup_funcs[] = {
	__yconf_cleanup,
	__dict_cleanup,
	__dbg_cleanup,
	NULL,
};

int
main()
{
	struct video_functionor_t * vid = NULL;
	do_init();

	struct exception_t exp;
	TRY(exp) {
		vid = (struct video_functionor_t *)find_functionor(&video_function_class, "dummy");
		VERBOSE(SYSTEM, "found video engine: %s\n", vid->name);


	} FINALLY {
		if (vid != NULL) {
			if (vid->destroy != NULL)
				vid->destroy();
			vid = NULL;
		}
	}
	CATCH(exp) {
		switch(exp.type) {
			case EXP_FUNCTIONOR_NOT_FOUND:
				ERROR(SYSTEM, "unable to find suitable video functionor\n");
				break;
			default:
				RETHROW(exp);
		}
	}

	do_cleanup();
	raise(SIGUSR1);
	return 0;
}

// vim:ts=4:sw=4

