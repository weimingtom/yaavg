#include <common/debug.h>
#include <common/dict.h>
#include <common/init_cleanup_list.h>
#include <common/mm.h>

#include <stdio.h>
#include <signal.h>

struct mem_cache_t * static_mem_caches[] = {
	&__dict_t_cache,
	NULL,
};

init_func_t init_funcs[] = {
	__dbg_init,
	NULL,
};


cleanup_func_t cleanup_funcs[] = {
	__dbg_cleanup,
	NULL,
};

static struct dict_entry_t s_table[8] = {
	[0] = {
		.key = "key3",
		.hash = 0x5c81d588,
		.data = {.str = "value"},
	},
	[2] = {
		.key = "key1",
		.hash = 0x5c81d58a,
		.data = {.str = "value"},
	},
	[6] = {
		.key = "key5",
		.hash = 0x5c81d58e,
		.data = {.str = "value"},
	},
	[7] = {
		.key = "key4",
		.hash = 0x5c81d58f,
		.data = {.str = "value"},
	},
};

static struct dict_t s = {
	.nr_fill = 4,
	.nr_used = 4,
	.mask = 7,
	.flags = DICT_FL_STRKEY | DICT_FL_FIXED,
	.private = STRDICT_FL_FIXED,
	.ptable = s_table,
};



int main()
{
	printf("%s\n", strdict_get(&s, "key3").str);
	printf("%s\n", strdict_get(&s, "key4").str);
	printf("%s\n", strdict_get(&s, "key1").str);
	return 0;
}

// vim:ts=4:sw=4

