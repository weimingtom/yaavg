/* 
 * mm.h
 * by WN @ Nov. 19, 2009
 */

#ifndef YAAVG_MM_H
#define YAAVG_MM_H

#include <config.h>
#include <common/defs.h>
#include <common/debug.h>
#include <common/exception.h>
#include <common/list.h>

#define xmalloc(___sz)	({	\
		void * ___ptr;		\
		___ptr = malloc(___sz);\
		if (___ptr == NULL)	\
			THROW(EXP_OUT_OF_MEMORY, "out of memory");\
			___ptr; \
		})

#define xcalloc(___count, ___eltsize)	({	\
		void * ___ptr;		\
		___ptr = calloc((___count), (___eltsize));\
		if (___ptr == NULL)	\
			THROW(EXP_OUT_OF_MEMORY, "out of memory");\
			___ptr; \
		})

#define xfree(___ptr) do {	\
	void * ___X_ptr = (void*)(___ptr);	\
	if ((___X_ptr) != NULL)			\
		free((___X_ptr));			\
} while(0)


struct mem_cache_t {
	const char * name;
	struct list_head free_list;
	uint32_t flags;
	int nr_active;
	int nr_free;
	size_t size;
	bool_t active;
};

#define DEFINE_MEM_CACHE(n, d, s) struct mem_cache_t n = { \
	.name = (d),	\
	.free_list = LIST_HEAD_INIT((n).free_list),	\
	.flags = 0,	\
	.nr_active = 0,	\
	.nr_free = 0,	\
	.size = (s),		\
	.active = TRUE,		\
}

#define SET_INACTIVE(c)	do { ((c)->active = FALSE); } while(0)

/* 
 * flags has not been defined now, it should be 0.
 * size should no less than struct list_head.
 */
extern void
mem_cache_init(const char * name, size_t size,
		struct mem_cache_t * cache, uint32_t flags);

extern void *
mem_cache_alloc(struct mem_cache_t * cache);

extern void *
mem_cache_zalloc(struct mem_cache_t * cache);

extern void
mem_cache_free(struct mem_cache_t * cache,
		void * ptr);

/* free the whole free list */
extern size_t
mem_cache_shrink(struct mem_cache_t * cache);

/* cleanup utils */
extern struct mem_cache_t __dict_t_cache;
extern struct mem_cache_t __file_resources_cache;
extern struct mem_cache_t * static_mem_caches[];

#endif
// vim:ts=4:sw=4

