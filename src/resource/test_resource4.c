#include <config.h>
#include <common/debug.h>
#include <common/exception.h>
#include <resource/resource.h>
#include <resource/resource_proc.h>
#include <bitmap/bitmap.h>
#include <io/io.h>

#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>

init_func_t init_funcs[] = {
	__dbg_init,
	__yconf_init,
	NULL,
};

cleanup_func_t cleanup_funcs[] = {
	__yconf_cleanup,
	__io_cleanup,
	__caches_cleanup,
	__dbg_cleanup,
	NULL,
};

static FILE *
wrap_fopen(const char * root_dir, const char * file_name)
{
	assert(file_name != NULL);
	char * full_name = NULL;
	if (root_dir != NULL) {
		full_name = alloca(strlen(root_dir) + strlen(file_name) + 1);
		sprintf(full_name, "%s/%s", root_dir, file_name);
	} else {
		full_name = alloca(strlen(file_name) + 1);
		strcpy(full_name, file_name);
	}

	VERBOSE(SYSTEM, "full name is %s\n", full_name);
	struct io_t * io;
	struct exception_t exp;
	TRY()
	FILE * fp = fopen(full_name, "wb");
	if (fp)
		return fp;

	/* create all directries */
	int full_name_sz = strlen(full_name) + 1;
	char * dir_names = alloca(full_name_sz);
	memset(dir_names, '\0', full_name_sz);
	strcpy(dir_names, full_name);

	char * pdir_names = dir_names;
	if (*pdir_names == '/')
		pdir_names ++;
	assert(*pdir_names != '\0');

	char * pos;
	pos = _strtok(pdir_names, '/');
	while (*pos != '\0') {
		*pos = '\0';
		VERBOSE(SYSTEM, "checking dir %s\n", dir_names);

		struct stat buf;
		int err;
		err = stat(dir_names, &buf);
		if (err == 0) {
			assert(S_ISDIR(buf.st_mode));
		} else {
			assert(errno == ENOENT);
			err = mkdir(dir_names, 0777);
			assert(err == 0);
		}
		*pos = '/';
		pos = _strtok(pos+1, '/');
	}

	fp = fopen(full_name, "wb");
	assert (fp != NULL);
	return fp;
}

int
main(int argc, char * argv[])
{
	if (argc < 3) {
		printf("%s <xp3file> <root>\n", argv[0]);
		exit(-1);
	}

	const char * xp3file = argv[1];
	const char * root = argv[2];


	struct stat stbuf;
	int err = stat(xp3file, &stbuf);
	if ((err != 0) || (!S_ISREG(stbuf.st_mode))) {
		printf("error %s\n", xp3file);
		exit(-1);
	}

	char * resname = NULL;
	struct bitmap_t * b = NULL;
	struct package_items_t * items = NULL;
	struct exception_t exp;
	TRY(exp) {
		do_init();
		launch_resource_process();

		char * ioname = alloca(strlen(argv[1] + 6));
		sprintf(ioname, "FILE:%s", argv[1]);
		VERBOSE(SYSTEM, "ioname=%s\n", ioname);
		items = get_package_items("XP3", ioname);
		assert(items != NULL);
		char ** ptr = items->table;
		while (*ptr != NULL) {
			char * prefix = _strtok(*ptr, '.');
			assert(*prefix == '.');
			if (strcmp(".tlg", prefix) == 0) {
				resname = xrealloc(resname, strlen(ioname) + 10 + strlen(*ptr));
				sprintf(resname, "0*XP3:%s|%s", *ptr, ioname);
				VERBOSE(SYSTEM, "load bitmap %s\n", resname);

				b = get_resource(resname,
						(deserializer_t)bitmap_deserialize);
				assert(b != NULL);
				free_bitmap(b);
				b = NULL;
			}
			ptr ++;
		}

	} FINALLY {
		xfree_null(items);
		xfree_null(resname);
		if (b != NULL)
			free_bitmap(b);
		if ((exp.type != EXP_RESOURCE_PEER_SHUTDOWN)
				&& (exp.type != EXP_RESOURCE_PROCESS_FAILURE))
		{
			shutdown_resource_process();
		}

	} CATCH(exp) {
		switch (exp.type) {
			case EXP_RESOURCE_PEER_SHUTDOWN:
			case EXP_RESOURCE_PROCESS_FAILURE:
				ERROR(SYSTEM, "resource process failure before main process\n");
				if (exp.u.val == RESOURCEEXP_TIMEOUT) {
					ERROR(SYSTEM, "resource process timeout, we should kill it\n");
					shutdown_resource_process();
				}
				break;
			default:
				RETHROW(exp);
		}
	}
	do_cleanup();

	return 0;
}


// vim:ts=4:sw=4

