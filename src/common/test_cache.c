#include <config.h>
#include <common/debug.h>
#include <common/exception.h>
#include <common/cache.h>
#include <stdio.h>
#include <assert.h>
#include <signal.h>


struct dummy_bitmap_t {
	struct cache_entry_t ce;
	int x, y, z, w;
};

init_func_t init_funcs[] = {
	__dbg_init,
	NULL,
};

cleanup_func_t cleanup_funcs[] = {
	__caches_cleanup,
	__dbg_cleanup,
	NULL,
};


static void
dummy_bitmap_destroy(struct dummy_bitmap_t * d)
{
	WARNING(SYSTEM, "Destroy bitmap %s\n", d->ce.id);
	if (d->ce.id != NULL)
		xfree(d->ce.id);
	xfree(d);
}

static struct dummy_bitmap_t *
dummy_bitmap_load(const char * str)
{
	struct dummy_bitmap_t * bitmap =
		xcalloc(1, sizeof(*bitmap));
	assert(bitmap != NULL);

	WARNING(SYSTEM, "Load bitmap %s\n", str);

	bitmap->ce.id = strdup(str);
	bitmap->ce.sz = sizeof(*bitmap);
	bitmap->ce.destroy_arg = bitmap;
	bitmap->ce.data = bitmap;
	bitmap->ce.destroy = (cache_destroy_t)(dummy_bitmap_destroy);
	bitmap->ce.cache = NULL;
	return bitmap;
}

static void
test_cache_creation(void)
{
	do_init();
	static struct cache_t c;
	VERBOSE(SYSTEM, "-------------- Testing cache create and destroy --------------\n");
	cache_init(&c, "creation cache", 2048);
	do_cleanup();
	VERBOSE(SYSTEM, "------------ END ---------------\n");
}

static void
test_cache_insertion(void)
{
	do_init();
	static struct cache_t c;
	VERBOSE(SYSTEM, "--------------- Testing cache insertion -------------\n");
	cache_init(&c, "dummy_bitmap_t's cache", 2048);

	for (int i = 0; i < 5; i++) {
		char id[5];
		snprintf(id, 5, "%d", i);
		struct dummy_bitmap_t * b = NULL;
		struct cache_entry_t * e = cache_get_entry(&c, id);
		if (e == NULL) {
			b = dummy_bitmap_load(id);
			cache_insert(&c, &(b->ce));
		} else {
			b = e->data;
		}
	}
	VERBOSE(SYSTEM, "------------ END ---------------\n");
	do_cleanup();
}

static struct dummy_bitmap_t *
get_bitmap(struct cache_t * c, int nr)
{
	char id[5];
	snprintf(id, 5, "%d", nr);

	struct dummy_bitmap_t * b = NULL;
	struct cache_entry_t * e = cache_get_entry(c, id);
	if (e == NULL) {
		b = dummy_bitmap_load(id);
		assert(b != NULL);
		cache_insert(c, &(b->ce));
	} else {
		b = e->data;
	}
	return b;
}

static void
put_bitmap(struct cache_t * c, struct dummy_bitmap_t * b)
{
}

static void
test_cache_use(void)
{
	do_init();
	static struct cache_t c;
	VERBOSE(SYSTEM, "---------------- Testing cache use ----------------\n");
	cache_init(&c, "dummy_bitmap_t's cache", 2048);


	{
		struct dummy_bitmap_t * b1 = get_bitmap(&c, 123);
		struct dummy_bitmap_t * b2 = get_bitmap(&c, 123);
		struct dummy_bitmap_t * b3 = get_bitmap(&c, 123);
		struct dummy_bitmap_t * b4 = get_bitmap(&c, 456);
		put_bitmap(&c, b4);
		put_bitmap(&c, b1);

		caches_shrink();
		put_bitmap(&c, b2);
		put_bitmap(&c, b3);
	}
	{
		struct dummy_bitmap_t * b4 = get_bitmap(&c, 456);
		put_bitmap(&c, b4);

		struct dummy_bitmap_t * b1 = get_bitmap(&c, 123);
		struct dummy_bitmap_t * b2 = get_bitmap(&c, 123);
		struct dummy_bitmap_t * b3 = get_bitmap(&c, 123);
		put_bitmap(&c, b1);
		cache_remove_entry(&c, "123");

		put_bitmap(&c, b2);
		put_bitmap(&c, b3);
		cache_remove_entry(&c, "456");

	}

	VERBOSE(SYSTEM, "------- END -----\n");
	do_cleanup();
}

static void
test_cache_replace(void)
{
	do_init();
	static struct cache_t c;
	VERBOSE(SYSTEM, "------------------ Testing cache replace -------------------\n");
	cache_init(&c, "dummy_bitmap_t's cache", 2048);

	{
		struct dummy_bitmap_t * b1 = get_bitmap(&c, 123);
		struct dummy_bitmap_t * b2 = get_bitmap(&c, 123);
		struct dummy_bitmap_t * b3 = get_bitmap(&c, 123);
		put_bitmap(&c, b1);

		/* generate new bitmap */
		struct dummy_bitmap_t * nb = dummy_bitmap_load("123");
		assert(nb != NULL);
		nb->x = 100;
		cache_insert(&c, &(nb->ce));
		put_bitmap(&c, b2);
		put_bitmap(&c, b3);
	}



	VERBOSE(SYSTEM, "------- END ---------\n");
	do_cleanup();
}

static void
test_cache_xxx(void)
{
	do_init();
	VERBOSE(SYSTEM, "--------------------- Testing cache xxx ---------------------\n");
	static struct cache_t c;
	cache_init(&c, "dummy_bitmap_t's cache", 2048);

	{
		struct dummy_bitmap_t * b1 = get_bitmap(&c, 123);
		struct dummy_bitmap_t * b2 = get_bitmap(&c, 123);
		struct dummy_bitmap_t * b3 = get_bitmap(&c, 123);
		put_bitmap(&c, b1);

		cache_insert(&c, &(b2->ce));
		cache_insert(&c, &(b2->ce));
		put_bitmap(&c, b2);
		put_bitmap(&c, b3);
	}



	VERBOSE(SYSTEM, "------- END ---------\n");
	do_cleanup();
}

static void
test_cache_size(void)
{
	do_init();
	VERBOSE(SYSTEM, "--------------------- Testing cache size ---------------------\n");

	static struct cache_t c;
	cache_init(&c, "dummy_bitmap_t's cache", sizeof(struct dummy_bitmap_t) * 3);

	get_bitmap(&c, 123);
	get_bitmap(&c, 456);
	get_bitmap(&c, 789);
	get_bitmap(&c, 123);
	get_bitmap(&c, 234);
	get_bitmap(&c, 567);
	get_bitmap(&c, 890);


	VERBOSE(SYSTEM, "------- END ---------\n");
	do_cleanup();
}


int main()
{

	test_cache_creation();
	test_cache_insertion();
	test_cache_use();
	test_cache_replace();
	test_cache_xxx();

	test_cache_size();

	return 0;
}

// vim:ts=4:sw=4

