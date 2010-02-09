#include <common/exception.h>
#include <common/debug.h>
#include <common/init_cleanup_list.h>
#include <common/mm.h>

init_func_t init_funcs[] = {
	__dbg_init,
	NULL,
};

cleanup_func_t cleanup_funcs[] = {
	__dbg_cleanup,
	NULL,
};


static void *
test(void)
{
	catch_var(void *, ptr, NULL);
	define_exp(exp);
	TRY(exp) {
		set_catched_var(ptr, xmalloc(1024));
		assert(ptr != NULL);
		VERBOSE(SYSTEM, "ptr = %p\n", ptr);
		THROW(EXP_UNCATCHABLE, "an exception");
	} FINALLY {
		VERBOSE(SYSTEM, "exitting test, ptr=%p\n", ptr);
	} CATCH(exp) {
		get_catched_var(ptr);
		VERBOSE(SYSTEM, "ptr=%p\n", ptr);
		if (ptr) {
			VERBOSE(SYSTEM, "free %p\n", ptr);
			xfree_null_catched(ptr);
		}
		RETHROW(exp);
	}
	return ptr;
}

int main()
{
	do_init();
	void * ptr ATTR_UNUSED = test();
	do_cleanup();
	return 0;
}

// vim:ts=4:sw=4

