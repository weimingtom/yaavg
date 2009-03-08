/* 
 * by WN @ Feb 06, 2009
 */


#include <common/debug.h>
#include <econfig/econfig.h>

#include <SDL/SDL.h>
#include <SDL/SDL_video.h>

#include <stdio.h>
#include <stdlib.h>
#include <video/engine_driver.h>
#include <video/engine_gl.h>

static struct SDLContext {
	struct GLContext gl_context;
	int bpp;
	int samples;
	const char * gllibrary;

	/* some sdl specific fields */
	SDL_Surface *screen;
} SDLCtx;


void * GLGetProcAddress(const char * name)
{
	return SDL_GL_GetProcAddress(name);
}

/* 
 * sdl_active: prevent multi-close SDL.
 */
static bool_t sdl_active = FALSE;

static void
close_sdl(void)
{
	if (sdl_active) {
		TRACE(SDL, "sdl close\n");
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		sdl_active = FALSE;
	}
}



static int 
init_sdl(void)
{
	int err, samples, bpp;
	const char * library_name = NULL;

	if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
		FATAL(SDL, "Failed to init SDL video subsystem, SDL report: \"%s\", "
				"check for your configuration.\n", SDL_GetError());
		/* exit immediatly */
		exit(-1);
	}

	library_name = ConfGetString("video.opengl.driver.gllibrary", NULL);
	if (library_name != NULL)
		VERBOSE(SDL, "Desire OpenGL library: %s\n", library_name);
	else
		VERBOSE(SDL, "Load default OpenGL library\n");

	if (SDL_GL_LoadLibrary(library_name) != 0) {
		FATAL(SDL, "Load OpenGL library failed: %s\n", SDL_GetError());
		close_sdl();
		exit(-1);
	}
	SDLCtx.gllibrary = library_name;

	/* check vsync */
	if (ConfGetBool("video.opengl.driver.vsync", FALSE)) {
		err = SDL_GL_SetAttribute (SDL_GL_SWAP_CONTROL, 1);
		VERBOSE(SDL, "Turn vsync on\n");
	} else {
		err = SDL_GL_SetAttribute (SDL_GL_SWAP_CONTROL, 0);
		VERBOSE(SDL, "Turn vsync off\n");
	}

	if (err != 0)
		WARNING(SDL, "Set vsync failed.\n");

	if ((samples = ConfGetInteger("video.opengl.driver.multisample", 0)) != 0) {
		SDL_GL_SetAttribute (SDL_GL_MULTISAMPLEBUFFERS, 1);
		err = SDL_GL_SetAttribute (SDL_GL_MULTISAMPLESAMPLES, samples);
		VERBOSE(SDL, "Set multisampling: %d\n", samples);
		if (err != 0)
			WARNING(SDL, "Set multisample failed.\n");
	}
	SDLCtx.samples = samples;

	bpp = ConfGetInteger("video.opengl.driver.bpp", 16);

	if (bpp >= 32) {
		SDL_GL_SetAttribute (SDL_GL_RED_SIZE, 8);
		SDL_GL_SetAttribute (SDL_GL_GREEN_SIZE, 8);
		SDL_GL_SetAttribute (SDL_GL_BLUE_SIZE, 8);
		SDL_GL_SetAttribute (SDL_GL_ALPHA_SIZE, 8);
		SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute (SDL_GL_STENCIL_SIZE, 8);
	} else {
		SDL_GL_SetAttribute (SDL_GL_RED_SIZE, 5);
		SDL_GL_SetAttribute (SDL_GL_GREEN_SIZE, 5);
		SDL_GL_SetAttribute (SDL_GL_BLUE_SIZE, 5);
		SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, 16);
	}
	SDLCtx.bpp = bpp;

	SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);

	sdl_active = TRUE;
	return 0;
}


static int
open_window(void)
{	
	/* SDL_SetVideoMode's params */
	int w, h, bpp, flags;
	bool_t fullscreen, resizable;
	SDL_Surface * screen = NULL;

	TRACE(SDL, "try SDL OpenWindow\n");
	if (!sdl_active) {
		ERROR(SDL, "try OpenWindow before init sdl platform.\n");
		return -1;
	}
	
	w = ConfGetInteger("video.resolution.w", 640);
	h = ConfGetInteger("video.resolution.h", 480);
	fullscreen = ConfGetBool("video.fullscreen", FALSE);
	resizable = ConfGetBool("video.resizable", FALSE);

	flags = SDL_OPENGL;

	if (resizable)
		flags |= SDL_RESIZABLE;
	if (fullscreen)
		flags |= SDL_FULLSCREEN;

	TRACE(SDL, "ready to init window:\n");
	TRACE(SDL, "\t width = %d\n", w);
	TRACE(SDL, "\t height = %d\n", h);
	TRACE(SDL, "\t fullscreen = %d\n", fullscreen);
	TRACE(SDL, "\t resizable = %d\n", resizable);

	screen = SDL_SetVideoMode(w, h, SDLCtx.bpp, flags);
	if (screen == NULL) {
		FATAL(SDL, "Failed to set video mode to %ix%i, SDL report: \"%s\", "
				"check for your configuration!\n", w, h,
				SDL_GetError());
		close_sdl();
		return -1;
	}

	/* Set context structure */
	SDLCtx.gl_context.base.width = w;
	SDLCtx.gl_context.base.height = h;
	SDLCtx.gl_context.base.is_full_screen = fullscreen;
	SDLCtx.gl_context.platform = "SDL";

	SDLCtx.screen = screen;
	return 0;
}

int
DriverInit(void)
{
	int err;

	TRACE(SDL, "sdl platform init\n");

	memset(&SDLCtx, '\0', sizeof(SDLCtx));

	if ((err = init_sdl()) != 0)
		return err;

	atexit(close_sdl);
	return 0;
}

void
GLClose(void)
{
	close_sdl();
	return;
}


struct GLContext * GLOpenWindow(void)
{
	int err;
	err = open_window();
	if (err != 0)
		return NULL;
	return &SDLCtx.gl_context;
}

void GLReopenWindow(struct GLContext * ctx)
{
	struct GLContext * new_ctx;
	int err;

	if (ctx != &SDLCtx.gl_context) {
		FATAL(SDL, "Context %p is not the original one (%p)\n",
				ctx, &SDLCtx.gl_context);
		exit(-1);
	}
	close_sdl();
	if ((err = init_sdl()) != 0) {
		FATAL(SDL, "Init sdl error: %d\n", err);
		exit(-1);
	}

	if ((err = open_window()) != 0) {
		FATAL(SDL, "Init open window error: %d\n", err);
		exit(-1);
	}

	return;
}

void
VideoSwapBuffers(void)
{
	SDL_GL_SwapBuffers();
}

void
VideoSetCaption(const char * caption)
{
	TRACE(SDL, "Set WM caption to %s\n", caption);
	SDL_WM_SetCaption(caption, NULL);
	return;
}

void
VideoSetIcon(const icon_t icon)
{
	WARNING(SDL, "Video has not implentmented\n");
	return;
}
