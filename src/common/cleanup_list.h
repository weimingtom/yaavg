/* 
 * cleanup_list
 * by WN @ Nov. 23, 2009
 */

#include <common/defs.h>
#include <common/debug.h>
#include <common/list.h>

#include <stdint.h>

__BEGIN_DECLS

/* we use cleanup list, not destructor, because in some
 * serious situation, the program need to be terminate immediately,
 * call cleanups may be dangerous. */

#define NR_CLEANUP_ENTRIES	(8)

typedef void (*cleanup_func_t)(uintptr_t);

void
register_cleanup(cleanup_func_t func, uintptr_t arg);

void
do_cleanup(void);

__END_DECLS

// vim:ts=4:sw=4

