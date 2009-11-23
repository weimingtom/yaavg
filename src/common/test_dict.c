#include <common/debug.h>
#include <common/dict.h>
#include <common/cleanup_list.h>

#include <stdio.h>
#include <signal.h>

static struct dict_entry_t entries[] = {
	{"<dummy_key>", "data1", 0},
	{"key2", "data2", 0},
	{"key3", "data3", 0},
	{"key4", "data4", 0},
	{"key5", "data5", 0},
	{"key6", "data6", 0},
	{"key7", "data7", 0},
	{"key8", "data8", 0},
	{"key9", "data9", 0},
	{"key10", "data10", 0},
	{"key11", "data1", 0},
	{"key12", "data2", 0},
	{"key13", "data3", 0},
	{"key14", "data4", 0},
	{"key15", "data5", 0},
	{"key16", "data6", 0},
	{"key17", "data7", 0},
	{"key18", "data8", 0},
	{"key19", "data9", 0},
	{"key20", "data10", 0},
	{"key21", "data1", 0},
	{"key22", "data2", 0},
	{"key23", "data3", 0},
	{"key24", "data4", 0},
	{"key25", "data5", 0},
	{"key26", "data6", 0},
	{"key27", "data7", 0},
	{"key28", "data8", 0},
	{"key29", "data9", 0},
	{"key30", "data10", 0},
	{"<dummy_key>", "data100", 0},
	{NULL, NULL, 0},
};

int main()
{
	struct dict_t * dict;
	dbg_init(NULL);

	struct exception_t exp;
	TRY(exp) {
		dict = dict_create(10, DICT_FL_STRKEY, NULL, 0);

		struct dict_entry_t * ep = entries;
		while (ep->key != NULL) {
			dict_insert(dict, ep);
			struct dict_entry_t te;
			te = dict_get(dict, ep->key, 0);
			VERBOSE(SYSTEM, "check data of key %s: %s\n",
					(char*)ep->key, (char*)te.data);
			ep ++;
		}
	} FINALLY {
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
	raise(SIGUSR1);
	return 0;
}

// vim:ts=4:sw=4

