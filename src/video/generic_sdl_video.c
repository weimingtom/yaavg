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
#include <video/video.h>
#include <video/generic_sdl_video.h>
#include <utils/generic_sdl.h>
#include <yconf/yconf.h>
#include <events/phy_events.h>

static SDL_Surface * main_screen = NULL;

static void
init_sdl_opengl(int bpp)
{
	static_catch_var(bool_t, libgl_loaded, FALSE);

	get_catched_var(libgl_loaded);
	if (!libgl_loaded) {
		const char * libname = conf_get_string("video.opengl.gllibrary", NULL);
		DEBUG(VIDEO, "opengl library name: %s\n", libname);
		if (SDL_GL_LoadLibrary(libname) != 0) {
			ERROR(VIDEO, "load opengl library %s failed: %s\n", libname, SDL_GetError());
			THROW_FATAL(EXP_UNCATCHABLE, "load opengl library failed");
		}
		set_catched_var(libgl_loaded, TRUE);
	}

	/* code is from darkplaces */
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
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

	if (conf_get_bool("video.opengl.stereo", FALSE))
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

static_catch_var(bool_t, video_inited, FALSE);

static void
init_sdl_video_base(bool_t use_opengl,
		int bpp, bool_t grabinput)
{
	define_exp(exp);
	TRY(exp) {
		get_catched_var(video_inited);
		if (video_inited) {
			FATAL(VIDEO, "sdl video subsystem has already inited\n");
			THROW_FATAL(EXP_UNCATCHABLE, "sdl video subsystem has already inited\n");
		}

		int err;
		err = SDL_InitSubSystem(SDL_INIT_VIDEO);
		if (err < 0) {
			FATAL(VIDEO, "init sdl video subsystem failed: %s\n",
					SDL_GetError());
			THROW_FATAL(EXP_UNCATCHABLE, "init sdl failed");
		}
		set_catched_var(video_inited, TRUE);
		if (use_opengl)
			init_sdl_opengl(bpp);
		if (grabinput)
			SDL_WM_GrabInput(SDL_GRAB_ON);
		else
			SDL_WM_GrabInput(SDL_GRAB_OFF);
		generic_init_sdl();
	} FINALLY{ 	}
	CATCH(exp) {
		get_catched_var(video_inited);
		if (video_inited) {
			set_catched_var(video_inited, FALSE);
			SDL_QuitSubSystem(SDL_INIT_VIDEO);
		}
		RETHROW(exp);
	}
	return;
}

SDL_Surface *
generic_init_sdl_video(bool_t use_opengl)
{
	assert(CUR_VID != NULL);
	DEBUG(VIDEO, "initing sdl video\n");

	init_sdl_video_base(use_opengl,
			CUR_VID->bpp, CUR_VID->grabinput);

	catch_var(SDL_Surface *, screen, NULL);
	define_exp(exp);
	TRY(exp) {
		int flags = 0;
		if (CUR_VID->fullscreen)
			flags |= SDL_FULLSCREEN;
		if (CUR_VID->resizable)
			flags |= SDL_RESIZABLE;
		if (use_opengl)
			flags |= SDL_OPENGL;
		set_catched_var(screen,
				SDL_SetVideoMode(
					CUR_VID->width,
					CUR_VID->height,
					CUR_VID->bpp,
					flags));
		if (screen == NULL) {
			ERROR(VIDEO, "SDL_SetVideoMode failed: %s, try to disable \"video.opengl.multisample\"\n",
					SDL_GetError());
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
			set_catched_var(screen,
					SDL_SetVideoMode(
						CUR_VID->width,
						CUR_VID->height,
						CUR_VID->bpp,
						flags));
			if (screen == NULL) {
				ERROR(VIDEO, "still failed: %s\n", SDL_GetError());
				THROW_FATAL(EXP_UNCATCHABLE, "SDL_SetVideoMode failed");
			}
		}

		/* notice the '!' */
		/* unblock sigint should be done after the init of screen */
		if (!conf_get_bool("video.sdl.blocksigint", TRUE))
			generic_sdl_unblock_sigint();

		if (screen->flags & SDL_FULLSCREEN)
			CUR_VID->fullscreen = TRUE;
		else
			CUR_VID->fullscreen = FALSE;

	} FINALLY { }
	CATCH(exp) {
		get_catched_var(screen);
		if (screen != NULL) {
			SDL_FreeSurface(screen);
			set_catched_var(screen, NULL);
		}
		generic_destroy_sdl_video();
		RETHROW(exp);
	}
	main_screen = screen;
	return screen;
}

void
generic_sdl_toggle_fullscreen(void)
{
	if (SDL_WM_ToggleFullScreen(main_screen)) {
		VERBOSE(VIDEO, "toggle fullscreen success\n");
		if (main_screen->flags & SDL_FULLSCREEN)
			CUR_VID->fullscreen = TRUE;
		else
			CUR_VID->fullscreen = FALSE;
	} else {
		VERBOSE(VIDEO, "toggle fullscreen failed\n");
	}
}

/* don't unload opengl driver */
void
generic_destroy_sdl_video(void)
{
	get_catched_var(video_inited);
	if (!video_inited)
		WARNING(VIDEO, "sdl video subsystem has not been initted\n");
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	set_catched_var(video_inited, FALSE);
}

#endif
// vim:ts=4:sw=4

