/* 
 * cache.c
 * by WN @ Dec. 20, 2009
 */

#include <config.h>
#include <common/debug.h>
#include <common/exception.h>
#include <common/mm.h>
#include <common/defs.h>
#include <common/dict.h>
#include <common/list.h>
#include <common/cache.h>

#include <assert.h>

static LIST_HEAD(__cache_list);
static bool_t cache_global_lock = FALSE;

bool_t
is_cache_locked(struct cache_t * cache)
{
	bool_t locked = FALSE;
	if (cache != NULL)
		locked = ((cache_global_lock) || (cache->locked));
	else
		locked = cache_global_lock;
	return locked;
}

void
test_cache_locked(struct cache_t * cache)
{
	if (is_cache_locked(cache))
		THROW_FATAL(EXP_CACHE_LOCKED, "cache is locked");
}


void
lock_caches(void)
{
	if (cache_global_lock)
		WARNING(CACHE, "global cache lock and unlock is not paired\n");
	cache_global_lock = TRUE;
}

void
unlock_caches(void)
{
	if (!cache_global_lock)
		WARNING(CACHE, "global cache lock and unlock is not paired\n");
	cache_global_lock = FALSE;
}


void
cache_init(struct cache_t * c, const char * name,
		ssize_t limit_sz)
{
	test_cache_locked(NULL);
	assert(c != NULL);
	assert(limit_sz > 0);
	c->name = name;
	DEBUG(CACHE, "init cache %s, size limit is %d\n",
			c->name, limit_sz);

	/* don't dup key: doesn't need, because the
	 * key is always entry->id and it has been already dupped. */
	c->dict = strdict_create(0, 0);
	assert(c->dict != NULL);

	c->nr = 0;
	c->total_sz = 0;
	c->limit_sz = limit_sz;
	c->locked = FALSE;

	list_add_tail(&(c->list), &__cache_list);
	INIT_LIST_HEAD(&(c->lru_head));

	return;
}

static void
__cache_insert(struct cache_t * cache,
		struct cache_entry_t * entry)
{
	dict_data_t od;
	struct dict_t * dict = cache->dict;

	od = strdict_insert(dict, entry->id,
			(dict_data_t)(void*)(entry));
	entry->cache = cache;

	if (!(DICT_DATA_NULL(od))) {
		struct cache_entry_t * oe = (struct cache_entry_t *)od.ptr;

		list_del(&oe->lru_list);
		cache->total_sz -= oe->sz;
		cache->nr --;
		if (entry == oe) {
			/* they are the same entry */
			WARNING(CACHE, "insert an cache entry \"%s\" into cache \"%s\" twice, promote the old one\n",
					oe->id, cache->name);
		} else {
			DEBUG(CACHE, "replace object %s\n", oe->id);
			DEBUG(CACHE, "destroy the old object %s\n", oe->id);
			cache_entry_destroy(oe);
		}
	}

	cache->total_sz += entry->sz;
	cache->nr ++;

	list_add_tail(&entry->lru_list, &cache->lru_head);

	return;
}

void
cache_insert(struct cache_t * cache,
		struct cache_entry_t * entry)
{
	assert(cache != NULL);
	assert(cache->dict != NULL);
	assert(entry != NULL);
	assert(entry->sz > 0);

	test_cache_locked(cache);

	/* the entry is already in the cache */
	if (entry->cache != NULL) {
		assert(entry->cache == cache);
		TRACE(CACHE, "entry %s has already in cache %s, bring it on\n",
				entry->id,
				cache->name);
		list_del(&entry->lru_list);
		list_add_tail(&entry->lru_list, &cache->lru_head);
		return;
	}

	TRACE(CACHE, "insert entry %s into %s\n", entry->id, cache->name);
	TRACE(CACHE, "currently cache %s size is %d, entry_sz is %d\n", cache->name,
			cache->total_sz, entry->sz);
	/* clear cache */
	while (cache->limit_sz < cache->total_sz + entry->sz) {
		int old_total_sz = cache->total_sz;
		if (list_empty(&cache->lru_head)) {
			/* only 1 possibility: the new entry is larger than cache size */
			if (cache->total_sz != 0) {
				ERROR(CACHE, "cache %s, limit_sz=%d, total_sz=%d, nr=%d but lru list is empty\n",
						cache->name, cache->limit_sz, cache->total_sz, cache->nr);
				THROW(EXP_UNCATCHABLE, "cache %s is corrupted", cache->name);
			}
			/* then the total sz is 0, break from the loop */
			break;
		}
		cache_remove_entry(cache,
				container_of(cache->lru_head.next,
					struct cache_entry_t, lru_list)->id);
		assert(old_total_sz != cache->total_sz);
	}

	TRACE(CACHE, "cache cleared, start inserting %s\n", entry->id);
	__cache_insert(cache, entry);
}


void
cache_remove_entry(struct cache_t * cache,
		const char * id)
{
	assert(cache != NULL);
	assert(cache->dict != NULL);
	assert(id != NULL);
	test_cache_locked(cache);

	dict_data_t od;
	struct dict_t * dict = cache->dict;
	
	od = strdict_remove(dict, id);
	if (!(DICT_DATA_NULL(od))) {
		TRACE(CACHE, "entry \"%s\" is in cache\n", id);
		struct cache_entry_t * entry = od.ptr;

		cache->nr --;
		cache->total_sz -= entry->sz;
		
		list_del(&entry->lru_list);
		cache_entry_destroy(entry);
		return;
	}
	
	TRACE(CACHE, "entry \"%s\" is not in cache\n", id);
	return;
}

struct cache_entry_t *
cache_get_entry(struct cache_t * cache, const char * id)
{
	assert(cache != NULL);
	assert(cache->dict != NULL);
	assert(id != NULL);

	TRACE(CACHE, "trying to get \"%s\" from cache \"%s\"\n",
			id, cache->name);

	struct cache_entry_t * res = NULL;
	dict_data_t d;
	struct dict_t * dict = cache->dict;
	d = strdict_get(dict, id);
	if ((d.ptr == NULL) || (DICT_DATA_NULL(d))) {
		TRACE(CACHE, "cache \"%s\" doesn't contain \"%s\"\n",
				cache->name, id);
		return NULL;
	}

	res = d.ptr;
	TRACE(CACHE, "get entry \"%s\"\n",
			res->id);
	list_del(&res->lru_list);
	list_add_tail(&res->lru_list, &cache->lru_head);
	return res;
}

void
cache_put_entry(struct cache_t * cache,
		struct cache_entry_t * entry)
{
	assert(cache != NULL);
	assert(cache->dict != NULL);
	assert(entry != NULL);
	assert((entry->cache == NULL) || (entry->cache == cache));

	list_del(&entry->lru_list);
	list_add(&entry->lru_list, &cache->lru_head);
}

static void
cache_cleanup(struct cache_t * cache)
{
	assert(cache != NULL);

	test_cache_locked(cache);

	assert(cache->dict != NULL);
	if (cache->nr == 0)
		return;
	if (cache->total_sz == 0)
		return;

	DEBUG(CACHE, "trying to shrink cache \"%s\"\n", cache->name);

	struct dict_t * dict = cache->dict;
	for (struct dict_entry_t * de = dict_get_next(dict, NULL);
			de != NULL; de = dict_get_next(dict, de))
	{
		struct cache_entry_t * ce = de->data.ptr;
		assert(ce != NULL);

		cache->nr --;
		cache->total_sz -= ce->sz;
		list_del(&ce->lru_list);
		/* the order is important: if the dict doesn't strduped the id,
		 * we have to destroy dict entry first. */
		strdict_invalid_entry(dict, de);
		cache_entry_destroy(ce);
	}
	assert(cache->nr == 0);
	assert(cache->total_sz == 0);
}

void
cache_shrink(struct cache_t * cache)
{
	cache_cleanup(cache);
}

void
caches_shrink(void)
{
	struct cache_t * c = NULL;
	list_for_each_entry(c, &__cache_list, list)
		cache_cleanup(c);
}

void
cache_destroy(struct cache_t * c)
{
	if (c->dict != NULL) {
		cache_cleanup(c);
		strdict_destroy(c->dict);
	}
	c->dict = NULL;
	if (!list_head_deleted(&(c->list)))
		list_del(&(c->list));
}

void
__caches_cleanup(void)
{
	struct cache_t * c = NULL;
	struct cache_t * n;
	list_for_each_entry_safe(c, n, &__cache_list, list) {
		cache_destroy(c);
	}
}

// vim:ts=4:sw=4

