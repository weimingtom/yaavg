/* 
 * init_cleanup_list
 * by WN @ Nov. 23, 2009
 */

#ifndef __INIT_CLEANUP_LIST_H
#define __INIT_CLEANUP_LIST_H

#include <common/defs.h>
#include <common/debug.h>
#include <common/list.h>

__BEGIN_DECLS

typedef void (*init_func_t)(void);
typedef void (*cleanup_func_t)(void);

extern init_func_t init_funcs[];
extern cleanup_func_t cleanup_funcs[];

extern void
do_init(void);

extern void
do_cleanup(void);

extern void __dbg_init(void);
extern void __dbg_cleanup(void);

extern void __yconf_init(void);
extern void __yconf_cleanup(void);

extern void __timer_init(void);
extern void __mem_cache_cleanup(void);
__END_DECLS
#endif

// vim:ts=4:sw=4

