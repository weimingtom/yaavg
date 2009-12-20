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

void
cache_init(struct cache_t * c, const char * name)
{
	assert(c != NULL);
	c->name = name;
	DEBUG(CACHE, "init cache %s\n", c->name);

	/* don't dup key: doesn't need, because the
	 * key is always entry->id and it has been dupped. */
	c->dict = strdict_create(0, 0);
	assert(c->dict != NULL);

	c->nr = 0;
	c->total_sz = 0;

	list_add(&(c->list), &__cache_list);
	return;
}

void
cache_insert(struct cache_t * cache,
		struct cache_entry_t * entry)
{
	assert(cache != NULL);
	assert(cache->dict != NULL);
	assert(entry != NULL);

	/* the entry is already in the cache */
	if (entry->cache != NULL) {
		assert(entry->cache == cache);
		return;
	}

	dict_data_t od;
	struct dict_t * dict = cache->dict;

	od = strdict_insert(dict, entry->id,
			(dict_data_t)(void*)(entry));
	entry->cache = cache;
	entry->ref_count ++;

	if (!(GET_DICT_DATA_FLAGS(od) & DICT_DATA_FL_VANISHED)) {
		struct cache_entry_t * oe = (struct cache_entry_t *)od.ptr;

		/* they are the same entry */
		if (entry == oe) {
			WARNING(CACHE, "insert an cache entry \"%s\" into cache \"%s\" twice\n",
					oe->id, cache->name);
			entry->ref_count --;
			return;
		}

		oe->ref_count --;
		DEBUG(CACHE, "replace object %s, ref_count = %d\n", oe->id,
				oe->ref_count);
		/* omit cache->nr: it doesn't change in this situation */
		cache->total_sz -= oe->sz;

		if (oe->ref_count != 0) {
			WARNING(CACHE, "replacing a currently using object \"%s\"\n",
					oe->id);
			oe->cache = NULL;
		} else {
			DEBUG(CACHE, "destroy the old object %s\n", oe->id);
			cache_entry_destroy(oe);
		}
	} else
		cache->nr ++;

	cache->total_sz += entry->sz;

	TRACE(CACHE, "\"%s\"'s ref_count become %d\n", entry->id,
			entry->ref_count);

	return;
}

void
cache_remove_entry(struct cache_t * cache,
		const char * id)
{
	assert(cache != NULL);
	assert(cache->dict != NULL);
	assert(id != NULL);

	dict_data_t od;
	struct dict_t * dict = cache->dict;
	
	od = strdict_remove(dict, id);
	if (!(GET_DICT_DATA_FLAGS(od) & DICT_DATA_FL_VANISHED)) {
		DEBUG(CACHE, "entry \"%s\" is in cache\n", id);
		struct cache_entry_t * entry = od.ptr;

		entry->ref_count --;
		cache->nr --;
		cache->total_sz -= entry->sz;
		
		if (entry->ref_count != 0) {
			DEBUG(CACHE, "entry \"%s\" is still in use, ref_count is %d\n",
					entry->id, entry->ref_count);
			entry->cache = NULL;
		} else {
			cache_entry_destroy(entry);
		}
		return;
	}
	
	DEBUG(CACHE, "entry \"%s\" is not in cache\n", id);
	return;
}

struct cache_entry_t *
cache_get_entry(struct cache_t * cache, const char * id)
{
	assert(cache != NULL);
	assert(cache->dict != NULL);
	assert(id != NULL);

	DEBUG(CACHE, "trying to get \"%s\" from cache \"%s\"\n",
			id, cache->name);

	struct cache_entry_t * res = NULL;
	dict_data_t d;
	struct dict_t * dict = cache->dict;
	d = strdict_get(dict, id);
	if ((d.ptr == NULL) || (GET_DICT_DATA_FLAGS(d) & DICT_DATA_FL_VANISHED)) {
		TRACE(CACHE, "cache \"%s\" doesn't contain \"%s\"\n",
				cache->name, id);
		return NULL;
	}

	res = d.ptr;
	assert(res->ref_count > 0);
	res->ref_count ++;
	TRACE(CACHE, "get entry \"%s\", ref_count become %d\n",
			res->id, res->ref_count);
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

	assert(entry->ref_count >= 1);
	entry->ref_count --;
	if (entry->ref_count == 0) {
		assert(entry->cache == NULL);
		DEBUG(CACHE, "destroy no used entry \"%s\" when put\n", entry->id);
		cache_entry_destroy(entry);
		return;
	}

	if (entry->cache == cache) {
		DEBUG(CACHE, "put entry \"%s\", ref_count become %d\n",
				entry->id, entry->ref_count);
	} else {
		DEBUG(CACHE, "put freed entry \"%s\", ref_count become %d\n",
				entry->id, entry->ref_count);
	}
}

static void
cache_cleanup(struct cache_t * cache, bool_t destroy_all)
{
	assert(cache != NULL);
	assert(cache->dict != NULL);

	DEBUG(CACHE, "tring to shink cache %s\n", cache->name);
	if (destroy_all)
		DEBUG(CACHE, "enable destroy_all\n");

	struct dict_t * dict = cache->dict;
	for (struct dict_entry_t * de = dict_get_next(dict, NULL);
			de != NULL; de = dict_get_next(dict, de))
	{
		struct cache_entry_t * ce = de->data.ptr;
		assert(ce != NULL);
		assert(ce->ref_count >= 1);
		if ((destroy_all) || (ce->ref_count == 1)) {
			if (ce->ref_count == 1)
				TRACE(CACHE, "cache entry \"%s\" can be freed\n",
						ce->id);
			else
				WARNING(CACHE, "cache entry \"%s\" is being used: ref_count = %d\n",
						ce->id, ce->ref_count);
			cache->nr --;
			cache->total_sz -= ce->sz;
			/* the order is important: if the dict doesn't strduped the id,
			 * we have to destroy dict entry first. */
			strdict_invalid_entry(dict, de);
			cache_entry_destroy(ce);
		}
	}
}

void
cache_shrink(struct cache_t * cache)
{
	cache_cleanup(cache, FALSE);
}

void
caches_shrink(void)
{
	struct cache_t * c = NULL;
	list_for_each_entry(c, &__cache_list, list)
		cache_cleanup(c, FALSE);
}

void
__caches_cleanup(void)
{
	struct cache_t * c = NULL;
	struct cache_t * n;
	list_for_each_entry_safe(c, n, &__cache_list, list) {
		if (c->dict != NULL) {
			cache_cleanup(c, TRUE);
			strdict_destroy(c->dict);
		}
		c->dict = NULL;
		list_del(&(c->list));
	}
}

// vim:ts=4:sw=4

