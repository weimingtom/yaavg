#include <stdio.h>
#include <stdlib.h>

#include <SDL/SDL_image.h>

#include <common/debug.h>
#include <common/exception.h>
#include <common/init_cleanup_list.h>

init_func_t init_funcs[] = {
	__dbg_init,
	NULL,
};

cleanup_func_t cleanup_funcs[] = {
	__dbg_cleanup,
	NULL,
};



int main(int argc, char * argv[])
{

	do_init();
	if (argv[1] == NULL)
		return -1;

	for (int i = 1; i < argc; i++) {
		VERBOSE(BITMAP, "loading %s\n", argv[i]);
		SDL_Surface * s = IMG_Load(argv[i]);
		if (s == NULL) {
			THROW(EXP_UNCATCHABLE, "load error: s == NULL");
		}

		VERBOSE(BITMAP, "w=%d, h=%d\n", s->w, s->h);
		VERBOSE(BITMAP, "pixel format:\n");
		struct SDL_PixelFormat * f = s->format;
		VERBOSE(BITMAP, "\tbits pre pixel=%d\n", f->BitsPerPixel);
		VERBOSE(BITMAP, "\tbytes pre pixel=%d\n", f->BytesPerPixel);
		VERBOSE(BITMAP, "\t(R, G, B, A)loss=(%d, %d, %d, %d)\n",
				f->Rloss,
				f->Gloss,
				f->Bloss,
				f->Aloss
			   );

		VERBOSE(BITMAP, "\t(R, G, B, A)shift=(%d, %d, %d, %d)\n",
				f->Rshift,
				f->Gshift,
				f->Bshift,
				f->Ashift
			   );

		VERBOSE(BITMAP, "\t(R, G, B, A)mask=(0x%x, 0x%x, 0x%x, 0x%x)\n",
				f->Rmask,
				f->Gmask,
				f->Bmask,
				f->Amask
			   );

		VERBOSE(BITMAP, "\tcolorkey=0x%x\n",
				f->colorkey
			   );

		VERBOSE(BITMAP, "\talpha=0x%x\n",
				f->alpha
			   );
		SDL_FreeSurface(s);
	}
	do_cleanup();

	return 0;
}

// vim:ts=4:sw=4

