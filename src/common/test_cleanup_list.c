#include <common/debug.h>
#include <common/exception.h>
#include <common/cleanup_list.h>
#include <common/dict.h>
#include <signal.h>
#include <stdio.h>

static void
print_destructor(uintptr_t arg)
{
	printf("XXXXX\n");
}

static struct cleanup_list_entry destructor = {
	.func = print_destructor,
	.arg = 0,
};

int main()
{
	dbg_init(NULL);
	register_cleanup_entry_hard(&destructor);
	struct exception_t exp;
	struct dict_t * dict;
	TRY(exp) {
		dict = dict_create(10, DICT_FL_STRKEY, NULL, 0);
	} FINALLY {
		dict_destroy(dict, NULL, 0);
	} CATCH(exp) {
		switch (exp.type) {
			default:
				RETHROW(exp);
		}
	}

	do_cleanup();
	raise(SIGUSR1);
	return 0;
}

// vim:ts=4:sw=4
