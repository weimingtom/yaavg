/* 
 * mm.c
 * by WN @ Nov. 19, 2009
 */

#include <common/debug.h>
#include <common/exception.h>
#include <common/list.h>
#include <common/mm.h>

#include <assert.h>
#include <memory.h>

void
mem_cache_init(const char * name, size_t size,
		struct mem_cache_t * new_cache, uint32_t flags)
{
	assert(flags == 0);
	assert(size >= sizeof(struct list_head));

	INIT_LIST_HEAD(&(new_cache->free_list));

	new_cache->name = name;
	new_cache->flags = flags;
	new_cache->nr_active = new_cache->nr_free = 0;
	new_cache->size = size;

	TRACE(MEMORY, "cache inited: %s(%p)\n", new_cache->name,
			new_cache);
	return;
}

void *
mem_cache_alloc(struct mem_cache_t * cache)
{
	void * ptr;
	if (cache->nr_free > 0) {
		struct list_head * h;
		h = cache->free_list.next;
		list_del(h);
		cache->nr_free --;
		ptr = (void*)h;
		TRACE(MEMORY, "grab a block from %s: %p\n",
				cache->name, ptr);
	} else {
		ptr = xmalloc(cache->size);
		TRACE(MEMORY, "no free block at %s, malloc it: %p\n",
				cache->name, ptr);
	}
	assert(ptr != NULL);
	cache->nr_active ++;
	return ptr;
}

void *
mem_cache_zalloc(struct mem_cache_t * cache)
{
	void * ptr = mem_cache_alloc(cache);
	bzero(ptr, cache->size);
	return ptr;
}

void
mem_cache_free(struct mem_cache_t * cache,
		void * ptr)
{
	struct list_head * h = (struct list_head *)ptr;
	list_add(h, &(cache->free_list));
	cache->nr_free ++;
	cache->nr_active --;
	TRACE(MEMORY, "free a block %p to %s, nr_free becomes %d\n",
			ptr, cache->name, cache->nr_free);
	return;
}

size_t
mem_cache_shrink(struct mem_cache_t * cache)
{
	size_t bytes = 0;
	struct list_head * pos, * n;
	TRACE(MEMORY, "begin shrink cache %s\n", cache->name);
	list_for_each_safe(pos, n, &(cache->free_list)) {
		list_del(pos);
		xfree(pos);
		bytes += cache->size;
		cache->nr_free --;
	}
	TRACE(MEMORY, "retrieve %d bytes from cache %s\n", bytes,
			cache->name);
	return bytes;
}

// vim:ts=4:sw=4

