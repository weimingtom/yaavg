/* 
 * generic_sdl.c
 * by WN @ Feb. 09, 2010
 */

#include <config.h>

#ifdef HAVE_SDL


#include <common/defs.h>
#include <common/exception.h>
#include <common/debug.h>

#include <SDL/SDL.h>
#include <SDL/SDL_video.h>
#include <utils/generic_sdl.h>
#include <yconf/yconf.h>

static void
init_sdl_opengl(void)
{
	const char * libname = conf_get_string("video.opengl.gllibrary", NULL);
	DEBUG(VIDEO, "opengl library name: %s\n", libname);
	if (SDL_GL_LoadLibrary(libname) != 0) {
		ERROR(VIDEO, "load opengl library %s failed: %s\n", libname, SDL_GetError());
		THROW_FATAL(EXP_UNCATCHABLE, "load opengl library failed");
	}

	/* code is from darkplaces */
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	int bpp = conf_get_int("video.opengl.bpp", 16);
	if (bpp >= 32) {
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
		/* different from darkplaces, we neend't 24 bit depth buffer */
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	} else {
		SDL_GL_SetAttribute (SDL_GL_RED_SIZE, 5);
		SDL_GL_SetAttribute (SDL_GL_GREEN_SIZE, 5);
		SDL_GL_SetAttribute (SDL_GL_BLUE_SIZE, 5);
		/* there's no 'ALPHA_SIZE', why? */
		SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, 16);
	}

	if (conf_get_bool("video.gl.stereo", FALSE))
		SDL_GL_SetAttribute(SDL_GL_STEREO, 1);

	int swapcontrol = conf_get_int("video.opengl.swapcontrol", 0);
	DEBUG(VIDEO, "vsync: %d\n", swapcontrol);
	int err = SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, swapcontrol);
	if (err < 0)
		WARNING(VIDEO, "set vsync failed: %s\n", SDL_GetError());

	int samples = conf_get_int("video.opengl.multisample", 0);
	DEBUG(VIDEO, "samples: %d\n", samples);
	if (samples > 0) {
		err = SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		if (err < 0)
			WARNING(VIDEO, "set opengl multisample buffers failes: %s\n", SDL_GetError());
		err = SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, samples);
		if (err < 0)
			WARNING(VIDEO, "set opengl multisample samples failes: %s\n", SDL_GetError());
	}
}

void 
init_sdl_video(bool_t use_opengl)
{
	catch_var(bool_t, inited, FALSE);
	define_exp(exp);
	TRY(exp) {
		int err;
		err = SDL_InitSubSystem(SDL_INIT_VIDEO);
		if (err < 0) {
			FATAL(VIDEO, "init sdl video subsystem failed: %s\n",
					SDL_GetError());
			THROW_FATAL(EXP_UNCATCHABLE, "init sdl failed");
		}
		set_catched_var(inited, TRUE);
		if (use_opengl)
			init_sdl_opengl();
		generic_int_sdl();
	} FINALLY{ 	}
	CATCH(exp) {
		get_catched_var(inited);
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

