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
/* we have 2 cleanup list, one is active when calling do_cleanup,
 * the other active when return or exit(). register_cleanup_entry install
 * cleanup entries onto the first(soft) list, register_cleanup_entry_hard
 * installs entries onto the next (hard) list. */
struct cleanup_list_entry {
	void (*func)(uintptr_t arg);
	uintptr_t arg;
	struct list_head list;
};

void
register_cleanup_entry(struct cleanup_list_entry * e);

void
register_cleanup_entry_hard(struct cleanup_list_entry * e);

void
do_cleanup(void);

__END_DECLS

// vim:ts=4:sw=4

