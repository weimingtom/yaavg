/* 
 * cache.h
 * by WN @ Dec. 20, 2009
 */

#ifndef __CACHE_H
#define __CACHE_H

#include <common/defs.h>
#include <common/mm.h>
#include <common/debug.h>
#include <common/dict.h>
#include <common/list.h>
#include <common/init_cleanup_list.h>

#include <stdio.h>	/* ssize_t */

/* 
 * cache is originally design for store temporary store currently
 * unused bitmap.
 */

typedef void (*cache_destroy_t)(void * arg);
struct cache_t {
	bool_t locked;
	const char * name;
	struct dict_t * dict;
	struct list_head list;
	struct list_head lru_head;
	int nr;
	int total_sz;
	ssize_t limit_sz;
};

struct cache_entry_t {
	const char * id;
	void * data;
	int sz;
	void * destroy_arg;
	cache_destroy_t destroy;
	/* cache must be set to NULL when
	 * the first time this entry is created. */
	struct cache_t * cache;
	struct list_head lru_list;
	void * pprivate;
};

/* 
 * cache_entry_destroy doesn't update dict's profiling infos,
 * the caller needs to do it by itself
 */
inline static void
cache_entry_destroy(struct cache_entry_t * e)
{
	if (e == NULL)
		return;
	if (e->destroy)
		e->destroy(e->destroy_arg);
}

extern void
cache_init(struct cache_t * c, const char * name,
		int limit_sz);

extern void
cache_destroy(struct cache_t * c);

extern void
cache_insert(struct cache_t * cache,
		struct cache_entry_t * entry);

/* 
 */
extern void
cache_remove_entry(struct cache_t * cache,
		const char * id);

/* 
 * get_entry will promote the entry
 */
extern struct cache_entry_t *
cache_get_entry(struct cache_t * cache,
		const char * id);
/* 
 * put_entry will move it
 * to the head of LRU list (easier to be removed)
 */
extern void
cache_put_entry(struct cache_t * cache,
		struct cache_entry_t * entry);

/* 
 * remove all entries
 */
extern void
cache_shrink(struct cache_t * cache);

extern void
caches_shrink(void);

static inline void
lock_cache(struct cache_t * cache)
{
	if (cache->locked)
		WARNING(CACHE, "cache lock and unlock is not paired\n");
	cache->locked = TRUE;
}

static inline void
unlock_cache(struct cache_t * cache)
{
	if (!cache->locked)
		WARNING(CACHE, "cache lock and unlock is not paired\n");
	cache->locked = FALSE;

}

extern void
lock_caches(void);

extern void
unlock_caches(void);

extern bool_t
if_cache_locked(struct cache_t * cache);

/* if cache is locked, throw an EXP_CACHE_LOCKED exception */
extern void
test_cache_locked(struct cache_t * cache);
#endif

// vim:ts=4:sw=4

