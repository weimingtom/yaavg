
#include <config.h>
#include <common/debug.h>
#include <common/exception.h>
#include <common/init_cleanup_list.h>
#include <common/mm.h>
#include <common/functionor.h>
#include <io/io.h>


#include <sys/stat.h>
#include <errno.h>

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

static char *
_strtok(char * str, char c)
{
	while ((*str != '\0') && (*str != c))
		str ++;
	if (*str == '\0')
		return NULL;
	return str;
}

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
	while (pos != NULL) {
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

int main(int argc, char * argv[])
{
	assert(argc > 1);
	struct stat buf;
	int err;
	err = stat(argv[1], &buf);
	assert(err == 0);

	char * root_dir = NULL;
	if (argv[2] != NULL)
		root_dir = argv[2];
	assert(root_dir != NULL);

	if (root_dir != NULL) {
		errno = 0;
		err = stat(root_dir, &buf);
		if (err != 0) {
			assert(errno == ENOENT);
		} else {
			assert(S_ISDIR(buf.st_mode));
		}
	}

	char * target_fn = NULL;
	if (argc >= 4) {
		target_fn = argv[3];
		VERBOSE(SYSTEM, "target file is %s\n", target_fn);
	}

	do_init();

	char * phy_file_name = argv[1];
	int phy_file_name_sz = strlen(phy_file_name) + 1;

	char * tmp_file_name = NULL;
	char ** table = NULL;
	struct io_t * io = NULL;
	struct exception_t exp;
	TRY(exp) {
		struct io_functionor_t * io_f = NULL;
		char * readdir_cmd = alloca(strlen(phy_file_name)+sizeof("readdir:"));
		sprintf(readdir_cmd, "readdir:%s", phy_file_name);

		io_f = get_io_handler("XP3");
		assert(io_f != NULL);
		table = iof_command(io_f, readdir_cmd, NULL);
		assert(table != NULL);
		char ** ptr = table;

		while (*ptr != NULL) {
			if (target_fn != NULL) {
				if (strcmp(target_fn, *ptr) != 0) {
					ptr ++;
					continue;
				}
			}
			VERBOSE(SYSTEM, "%s\n", *ptr);

			int comp_fn_sz = phy_file_name_sz + strlen(*ptr) + 1;
			tmp_file_name = xrealloc(tmp_file_name, comp_fn_sz);
			sprintf(tmp_file_name, "%s:%s", phy_file_name, *ptr);
			if (_strtok(tmp_file_name, '$') != NULL) {
				VERBOSE(IO, "wrong file name: %s\n", tmp_file_name);
				ptr ++;
			}


			io = io_open("XP3", tmp_file_name);
			assert(io != NULL);

			/* mkdir and create the file */


			FILE * fp = wrap_fopen(root_dir, *ptr);
			assert(fp != NULL);

			void * data = io_get_internal_buffer(io);
			fwrite(data, io_get_sz(io), 1, fp);
			io_release_internal_buffer(io, data);

			fclose(fp);

			io_close(io);
			io = NULL;
			ptr ++;
		}

#if 0
		struct io_t * io = io_open("XP3", "/home/wn/Windows/Fate/Realta Nua BGM.xp3:se497.ogg");
		assert(io != NULL);
		VERBOSE(SYSTEM, "size=%Ld\n", io_get_sz(io));

		struct io_t * file = io_open_write("FILE", "/tmp/se497.ogg");
		assert(file != NULL);
		
		void * buffer = io_get_internal_buffer(io);
		assert(buffer != NULL);
		io_write_force(file, buffer, io_get_sz(io));
		io_release_internal_buffer(io, buffer);

		io_close(file);
#endif

	} FINALLY {
		if (tmp_file_name != NULL)
			xfree(tmp_file_name);
		if (table != NULL)
			xfree(table);
	}
	CATCH(exp) {
		if (io != NULL)
			io_close(io);
		RETHROW(exp);
	}
	do_cleanup();
	return 0;
}

// vim:ts=4:sw=4

