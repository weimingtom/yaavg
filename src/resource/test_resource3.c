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
write_resource(const char * resname, const char * filename)
{
	struct bitmap_t * b = NULL;
	struct io_t * writer = NULL;
	struct exception_t exp;
	TRY(exp) {
		writer = io_open_write("FILE", filename);
		b = get_resource(resname,
				(deserializer_t)bitmap_deserialize);
		bitmap_to_png(b, writer);
	} FINALLY {
		if (b != NULL)
			free_bitmap(b);
		if (writer != NULL)
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
}


int main()
{
	struct exception_t exp;
	TRY(exp) {
		do_init();
		launch_resource_process();

//		write_resource("0*XP3:01星空_thumb.tlg|FILE:/home/wn/Windows/Fate/image.xp3", "/tmp/out/2.png");
		write_resource("0*XP3:ss_保有背景真アサシン.tlg|FILE:/home/wn/Windows/Fate/image.xp3", "/tmp/out/2.png");
		write_resource("0*XP3:csセイバー炉心祈り.png|FILE:/home/wn/Windows/Fate/Realta Nua IMAGE.xp3", "/tmp/out/6.png");
		write_resource("0*XP3:25時間軸説明・セイバーc_thumb.tlg|FILE:/home/wn/Windows/Fate/image.xp3", "/tmp/out/1.png");

#if 0
		write_resource("0*FILE:no_alpha.png", "/tmp/out/1.png");
		write_resource("0*XP3:ed1.png|FILE:/home/wn/Windows/Fate/Realta Nua IMAGE.xp3", "/tmp/out/2.png");
		write_resource("0*XP3:ed2.png|FILE:/home/wn/Windows/Fate/Realta Nua IMAGE.xp3", "/tmp/out/3.png");
		write_resource("0*XP3:ed1.png|FILE:/home/wn/Windows/Fate/Realta Nua IMAGE.xp3", "/tmp/out/4.png");
		write_resource("0*XP3:allcl1.jpg|FILE:/home/wn/Windows/Fate/Realta Nua IMAGE.xp3", "/tmp/out/5.png");
		write_resource("0*XP3:csセイバー炉心祈り.png|FILE:/home/wn/Windows/Fate/Realta Nua IMAGE.xp3", "/tmp/out/6.png");
		write_resource("0*XP3:ed1.png|FILE:/home/wn/Windows/Fate/Realta Nua IMAGE.xp3", "/tmp/out/7.png");
		write_resource("0*XP3:c_cs09.jpg|FILE:/home/wn/Windows/Fate/Realta Nua IMAGE.xp3", "/tmp/out/8.png");
		write_resource("0*XP3:c_cs13b.jpg|FILE:/home/wn/Windows/Fate/Realta Nua IMAGE.xp3", "/tmp/out/9.png");
		write_resource("0*XP3:ps051.jpg|FILE:/home/wn/Windows/Fate/Realta Nua IMAGE.xp3", "/tmp/out/10.png");
		write_resource("0*XP3:ed1.png|FILE:/home/wn/Windows/Fate/Realta Nua IMAGE.xp3", "/tmp/out/11.png");
		write_resource("0*XP3:ps043.jpg|FILE:/home/wn/Windows/Fate/Realta Nua IMAGE.xp3", "/tmp/out/12.png");
		write_resource("0*XP3:C12f0.jpg|FILE:/home/wn/Windows/Fate/Realta Nua IMAGE.xp3", "/tmp/out/13.png");
		write_resource("0*XP3:C12c0.jpg|FILE:/home/wn/Windows/Fate/Realta Nua IMAGE.xp3", "/tmp/out/14.png");
#endif
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
				/* ... */
				do_cleanup();
				break;
			default:
				RETHROW(exp);
		}
	}
	return 0;
}

// vim:ts=4:sw=4

