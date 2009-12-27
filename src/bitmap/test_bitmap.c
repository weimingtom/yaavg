#include <config.h>
#include <common/debug.h>
#include <common/exception.h>
#include <common/init_cleanup_list.h>
#include <common/mm.h>
#include <common/functionor.h>
#include <bitmap/bitmap.h>

#include <signal.h>

#if 0

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

int main()
{
	do_init();
	struct bitmap_t * bitmap = load_bitmap("xxx.jpg");
	destroy_bitmap(bitmap);
	bitmap = load_bitmap("xxx.jpg");
	destroy_bitmap(bitmap);
	do_cleanup();
	return 0;
}
#endif
int main()
{
	return 0;
}

// vim:ts=4:sw=4

