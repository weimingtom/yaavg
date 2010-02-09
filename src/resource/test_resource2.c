#include <common/init_cleanup_list.h>
#include <resource/resource_proc.h>
#include <resource/resource.h>

#include <common/debug.h>
#include <common/exception.h>
#include <bitmap/bitmap.h>
#include <bitmap/bitmap_to_png.h>

#include <unistd.h>
#include <signal.h>


init_func_t init_funcs[] = {
	__dbg_init,
	__yconf_init,
	__timer_init,
	NULL,
};

cleanup_func_t cleanup_funcs[] = {
	__yconf_cleanup,
	__io_cleanup,
	__caches_cleanup,
	__dbg_cleanup,
	NULL,
};

static void 
print_bitmap(struct bitmap_t * b)
{
	VERBOSE(SYSTEM, "get bitmap! id=%s\n", b->id);
	VERBOSE(SYSTEM, "first 4 bytes: 0x%x, 0x%x, 0x%x, 0x%x\n",
			b->pixels[0],
			b->pixels[1],
			b->pixels[2],
			b->pixels[3]);
	printf("\n");
	if ((b->w == 8) && (b->h == 8)) {
		for (int i = 0; i < 8; i++) {
			for (int j = 0; j < 8; j++) {
				printf("%x%x%x%x ",
						b->pixels[i * 8 * 4 + j],
						b->pixels[i * 8 * 4 + j + 1],
						b->pixels[i * 8 * 4 + j + 2],
						b->pixels[i * 8 * 4 + j + 3]
						);
			}
			printf("\n");
		}
	}
}

int main()
{
	/* test start and stop */

	struct exception_t exp;
	TRY(exp) {
		do_init();
		launch_resource_process();

		struct bitmap_t * b = get_resource("0*FILE:/tmp/xxx.png",
				(deserializer_t)bitmap_deserialize);
		print_bitmap(b);
		free_bitmap(b);


		b = get_resource("0*FILE:./no_alpha.png",
				(deserializer_t)bitmap_deserialize);
		print_bitmap(b);
		free_bitmap(b);
#if 0
		for (int i = 0; i < 10000; i++) {
			b = get_resource("0:file:./no_alpha.png",
					(deserializer_t)bitmap_deserialize);
			free_bitmap(b);
		}
#endif


		b = get_resource("0*FILE:./largepng.png",
				(deserializer_t)bitmap_deserialize);
		print_bitmap(b);
		free_bitmap(b);

		b = get_resource("0*FILE:./jpegtest.jpeg",
				(deserializer_t)bitmap_deserialize);
		print_bitmap(b);
		free_bitmap(b);

		b = get_resource("0*FILE:./out/RNImage/allcl1.jpg",
				(deserializer_t)bitmap_deserialize);
		print_bitmap(b);
		free_bitmap(b);

		b = get_resource("0*XP3:allcl1.jpg|FILE:/home/wn/Windows/Fate/Realta Nua IMAGE.xp3",
				(deserializer_t)bitmap_deserialize);
		print_bitmap(b);
		free_bitmap(b);

		/* write the bitmap */

		b = get_resource("0*FILE:./have_alpha.png",
				(deserializer_t)bitmap_deserialize);
		print_bitmap(b);

		struct io_t * writer = io_open_write_proto("FILE:/tmp/xxx.png");
		assert(writer != NULL);
		struct exception_t exp;
		TRY(exp) {
			bitmap_to_png(b, writer);
		} FINALLY {
			io_close(writer);
		} CATCH(exp) {
			switch (exp.type) {
				case EXP_UNSUPPORT_OPERATION:
				case EXP_LIBPNG_ERROR:
					print_exception(&exp);
					WARNING(SYSTEM, "write png failed\n");
					break;
				default:
					RETHROW(exp);
			}
		}

		free_bitmap(b);

	} FINALLY {
		if ((exp.type != EXP_RESOURCE_PEER_SHUTDOWN)
				&& (exp.type != EXP_RESOURCE_PROCESS_FAILURE)) {
			shutdown_resource_process();
			do_cleanup();
		}
	} CATCH (exp) {
		switch (exp.type) {
			case EXP_RESOURCE_PEER_SHUTDOWN:
			case EXP_RESOURCE_PROCESS_FAILURE:
				WARNING(SYSTEM, "resource process exit before main process\n");
				do_cleanup();
				break;
			default:
				RETHROW(exp);
		}
	}

	return 0;
}

// vim:ts=4:sw=4

