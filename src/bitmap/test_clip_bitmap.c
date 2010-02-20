#include <config.h>
#include <common/debug.h>
#include <common/exception.h>
#include <resource/resource.h>
#include <resource/resource_proc.h>
#include <resource/resource_types.h>
#include <bitmap/bitmap.h>
#include <bitmap/bitmap_to_png.h>
#include <io/io.h>

#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>

init_func_t init_funcs[] = {
	__dbg_init,
	__yconf_init,
	NULL,
};

cleanup_func_t cleanup_funcs[] = {
	__yconf_cleanup,
	__io_cleanup,
	__caches_cleanup,
	__dbg_cleanup,
	NULL,
};

int main()
{
	define_exp(exp);
	TRY(exp) {
		do_init();
		launch_resource_process();

		struct bitmap_t * b = load_bitmap("0*XP3:ss_保有背景真アサシン.tlg|FILE:/home/wn/Windows/Fate/image.xp3",
				NULL);
		struct rect_t rect = {
			.x = 100,
			.y = 100,
			.w = 200,
			.h = 250,
		};
		struct bitmap_t * b2 = clip_bitmap(b, rect, 0);

		define_exp(exp);
		catch_var(struct io_t *, writer1, NULL);
		catch_var(struct io_t *, writer2, NULL);
		TRY(exp) {
			set_catched_var(writer1, io_open_write("FILE", "/tmp/xxx.png"));
			bitmap_to_png(b2, writer1);
			set_catched_var(writer2, io_open_write("FILE", "/tmp/ori.png"));
			bitmap_to_png(b, writer2);
		} FINALLY {
			get_catched_var(writer1);
			get_catched_var(writer2);
			if (writer1 != NULL)
				io_close(writer1);
			if (writer2 != NULL)
				io_close(writer2);
		} CATCH(exp) {
			RETHROW(exp);
		}

		free_bitmap(b);
		free_bitmap(b2);
	} FINALLY {
		shutdown_resource_process();
		do_cleanup();
	} CATCH(exp) {
		
	}
	return 0;
}

// vim:ts=4:sw=4


