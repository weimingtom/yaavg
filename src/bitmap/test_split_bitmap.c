#include <config.h>
#include <common/debug.h>
#include <common/exception.h>
#include <resource/resource.h>
#include <resource/resource_proc.h>
#include <resource/resource_types.h>
#include <bitmap/bitmap.h>
#include <bitmap/bitmap_to_png.h>
#include <utils/rect_mesh.h>
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
		struct bitmap_array_t * array = split_bitmap(b,
				128, 128, 1);
		if (array->original_bitmap == NULL) {
			free_bitmap(b);
		}
		struct rect_mesh_t * mesh = build_mesh_by_array(array);
		assert(mesh != NULL);

		VERBOSE(SYSTEM, "%dx%d\n", array->nr_w, array->nr_h);
		char name[128];
		for (int j = 0; j < array->nr_h; j++) {
			for (int i = 0; i < array->nr_w; i++) {
				snprintf(name, 128, "/tmp/%dx%d.png", j, i);
				define_exp(exp);
				catch_var(struct io_t *, writer, NULL);
				TRY(exp) {
					set_catched_var(writer, io_open_write("FILE", name));
					bitmap_to_png(&array->tiles[j * array->nr_w + i], writer);
				} FINALLY {
					get_catched_var(writer);
					if (writer != NULL)
						io_close(writer);
				} CATCH(exp) {
					RETHROW(exp);
				}
			}
		}

		print_rect_mesh(mesh);
		destroy_rect_mesh(mesh);

		if (array->original_bitmap != NULL) {
			free_bitmap(array->original_bitmap);
			free_bitmap_array(array);
		}

	} FINALLY {
		shutdown_resource_process();
		do_cleanup();
	} CATCH(exp) {
		
	}
	return 0;
}

// vim:ts=4:sw=4


