/* 
 * generic_sdl.c
 * by WN @ Feb. 09, 2010
 */

#include <config.h>
#include <common/defs.h>
#include <common/exception.h>
#include <common/debug.h>

#ifdef HAVE_SDL

#include <SDL/SDL.h>
#include <SDL/SDL_video.h>
#include <utils/generic_sdl.h>
#include <yconf/yconf.h>

void 
init_sdl_video(bool_t use_opengl)
{
	bool_t inited = FALSE;
	define_exp(exp);
	TRY(exp) {
		int err;
		err = SDL_InitSubSystem(SDL_INIT_VIDEO);
		if (err < 0) {
			FATAL(VIDEO, "init sdl video subsystem failed: %s\n",
					SDL_GetError());
			THROW_FATAL(EXP_UNCATCHABLE, "init sdl failed");
		}
		inited = TRUE;
	} FINALLY{ 	}
	CATCH(exp) {
		if (inited)
			SDL_QuitSubSystem(SDL_INIT_VIDEO);
		RETHROW(exp);
	}
	return;
}

void
destroy_sdl_video(void)
{
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}


#endif
// vim:ts=4:sw=4

