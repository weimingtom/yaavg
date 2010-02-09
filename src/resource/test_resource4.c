#include <config.h>
#include <common/debug.h>
#include <common/exception.h>
#include <resource/resource.h>
#include <resource/resource_proc.h>
#include <bitmap/bitmap.h>
#include <bitmap/bitmap_to_png.h>
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

static struct io_t *
wrap_io_open_write(const char * root_dir, const char * file_name)
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

	struct io_functionor_t * iof = get_io_handler("FILE");
	assert(iof != NULL);
//	struct io_t * io = NULL;
	FILE * fp = NULL;
//	io = NOTHROW_RET(NULL, io_open_write, "FILE", full_name);
//	if (io != NULL)
//		return io;

	fp = fopen(full_name, "wb");
	if (fp) {
		return iof_command(iof, "buildfromstdfile:write", fp);
	}

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

//	io = io_open_write("FILE", full_name);
//	assert(io != NULL);
//	return io;

	fp = fopen(full_name, "wb");
	assert(fp != NULL);
	return iof_command(iof, "buildfromstdfile:write", fp);
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
	assert(root != NULL);


	struct stat stbuf;
	int err = stat(xp3file, &stbuf);
	if ((err != 0) || (!S_ISREG(stbuf.st_mode))) {
		printf("error %s\n", xp3file);
		exit(-1);
	}

	catch_var(char *, tmp_name, NULL);
	catch_var(struct bitmap_t *, b, NULL);
	catch_var(struct package_items_t *, items, NULL);
	catch_var(struct io_t *, io, NULL);
	define_exp(exp);
	TRY(exp) {
		do_init();
		launch_resource_process();

		char * ioname = alloca(strlen(argv[1] + 6));
		sprintf(ioname, "FILE:%s", argv[1]);
		VERBOSE(SYSTEM, "ioname=%s\n", ioname);
		set_catched_var(items, get_package_items("XP3", ioname));
		assert(items != NULL);
		char ** ptr = items->table;
		while (*ptr != NULL) {
			char * prefix = _strtok(*ptr, '.');
			assert(*prefix == '.');
			if (strcmp(".tlg", prefix) == 0) {
				set_catched_var(tmp_name, xrealloc(tmp_name, strlen(ioname) + 10 + strlen(*ptr)));
				sprintf(tmp_name, "0*XP3:%s|%s", *ptr, ioname);
				VERBOSE(SYSTEM, "load bitmap %s\n", tmp_name);

				set_catched_var(b, get_resource(tmp_name,
						(deserializer_t)bitmap_deserialize));
				assert(b != NULL);

				/* append the '.png' post-fix */
				set_catched_var(tmp_name, xrealloc(tmp_name, strlen(*ptr) + 5));
				assert(tmp_name != NULL);
				sprintf(tmp_name, "%s.png", *ptr);

				set_catched_var(io, wrap_io_open_write(root, tmp_name));
				bitmap_to_png(b, io);
				io_close(io);
				set_catched_var(io, NULL);

				free_bitmap(b);
				set_catched_var(b, NULL);
			}
			ptr ++;
		}
	} FINALLY {
		get_catched_var(items);
		get_catched_var(tmp_name);
		get_catched_var(b);
		get_catched_var(io);

		xfree_null_catched(items);
		xfree_null_catched(tmp_name);
		if (b != NULL)
			free_bitmap(b);
		if (io != NULL)
			io_close(io);
		set_catched_var(b, NULL);
		set_catched_var(io, NULL);

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

