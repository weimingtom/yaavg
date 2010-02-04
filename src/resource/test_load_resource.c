/* 
 * test_load_resource.c
 * by WN @ Feb. 02, 2009
 */

#include <common/init_cleanup_list.h>
#include <resource/resource_proc.h>
#include <resource/resource.h>

#include <common/debug.h>
#include <common/exception.h>
#include <bitmap/bitmap.h>

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

static const char * name[12] = {
	"0*FILE:/tmp/xxx.png",
	"0*XP3:ed1.png|FILE:/home/wn/Windows/Fate/Realta Nua IMAGE.xp3", 
	"0*XP3:ed2.png|FILE:/home/wn/Windows/Fate/Realta Nua IMAGE.xp3", 
	"0*XP3:ed1.png|FILE:/home/wn/Windows/Fate/Realta Nua IMAGE.xp3", 
	"0*XP3:allcl1.jpg|FILE:/home/wn/Windows/Fate/Realta Nua IMAGE.xp3", 
	"0*XP3:csセイバー炉心祈り.png|FILE:/home/wn/Windows/Fate/Realta Nua IMAGE.xp3", 
	"0*XP3:c_cs09.jpg|FILE:/home/wn/Windows/Fate/Realta Nua IMAGE.xp3", 
	"0*XP3:c_cs13b.jpg|FILE:/home/wn/Windows/Fate/Realta Nua IMAGE.xp3", 
	"0*XP3:ps051.jpg|FILE:/home/wn/Windows/Fate/Realta Nua IMAGE.xp3", 
	"0*XP3:ps043.jpg|FILE:/home/wn/Windows/Fate/Realta Nua IMAGE.xp3", 
	"0*XP3:C12f0.jpg|FILE:/home/wn/Windows/Fate/Realta Nua IMAGE.xp3", 
	"0*XP3:C12c0.jpg|FILE:/home/wn/Windows/Fate/Realta Nua IMAGE.xp3", 
};

int main()
{
	do_init();
	struct resource_t * res[12];
	for (int i = 0; i < 12; i++) {
		res[i] = load_resource(name[i]);
	}
	for (int i = 0; i < 12; i++) {
		res[i]->destroy(res[i]);
	}
	do_cleanup();
	return 0;
}

// vim:ts=4:sw=4
