#include <common/debug.h>
#include <common/dict.h>
#include <common/mm.h>
#include <common/init_cleanup_list.h>

#include <stdio.h>
#include <signal.h>

init_func_t init_funcs[] = {
	__dbg_init,
	NULL,
};


cleanup_func_t cleanup_funcs[] = {
	__dbg_cleanup,
	NULL,
};


static struct dict_entry_t entries[] = {
	{"<dummy_key>", 0, {"data1"},},
	{"key2", 0, {"data2"},},
	{"key3", 0, {"data3"},},
	{"key4", 0, {"data4"},},
	{"key5", 0, {"data5"},},
	{"key6", 0, {"data6"},},
	{"key7", 0, {"data7"},},
	{"key8", 0, {"data8"},},
	{"key9", 0, {"data9"},},
	{"key10", 0, {"data10"},},
	{"key11", 0, {"data1"},},
	{"key12", 0, {"data2"},},
	{"key13", 0, {"data3"},},
	{"key14", 0, {"data4"},},
	{"key15", 0, {"data5"},},
	{"key16", 0, {"data6"},},
	{"key17", 0, {"data7"},},
	{"key18", 0, {"data8"},},
	{"key19", 0, {"data9"},},
	{"key20", 0, {"data10"},},
	{"key21", 0, {"data1"},},
	{"key22", 0, {"data2"},},
	{"key23", 0, {"data3"},},
	{"key24", 0, {"data4"},},
	{"key25", 0, {"data5"},},
	{"key26", 0, {"data6"},},
	{"key27", 0, {"data7"},},
	{"key28", 0, {"data8"},},
	{"key29", 0, {"data9"},},
	{"key30", 0, {"data10"},},
	{"<dummy_key>", 0, {"data100"},},
	{NULL, 0, {NULL},},
};

int main()
{
	dbg_init(NULL);

	catch_var(struct dict_t *, dict, NULL);
	define_exp(exp);
	TRY(exp) {
		set_catched_var(dict, dict_create(10, DICT_FL_STRKEY, NULL, 0));

		struct dict_entry_t * ep = entries;
		while (ep->key != NULL) {
			struct dict_entry_t te;
			te = dict_insert(dict, ep);
			if (!DICT_ENTRY_NODATA(&te)) {
				VERBOSE(SYSTEM, "old data of %s: %s\n",
						(char*)ep->key, (char*)te.data.str);
			}
			te = dict_get(dict, ep->key, 0);
			VERBOSE(SYSTEM, "check data of key %s: %s\n",
					(char*)ep->key, (char*)te.data.str);
			ep ++;
		}
	} FINALLY {
		get_catched_var(dict);
		dict_destroy(dict, NULL, 0);
	}
	CATCH(exp) {
		switch (exp.type) {
			case EXP_DICT_FULL:
				VERBOSE(SYSTEM, "We come here!\n");
				break;
			default:
				RETHROW(exp);
		}
	}

	do_cleanup();
	return 0;
}

// vim:ts=4:sw=4

