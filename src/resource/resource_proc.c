/* 
 * resource.c
 * by WN @ Dec, 27, 2009
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>
#include <wait.h>

#include <config.h>
#include <common/defs.h>
#include <common/debug.h>
#include <common/exception.h>
#include <common/init_cleanup_list.h>
#include <common/mm.h>
#include <common/dict.h>
#include <common/cache.h>
#include <yconf/yconf.h>

#include <resource/resource_proc.h>
#include <resource/resource.h>


#define OK_SIG	(0x98765432)

static pid_t resproc_pid = 0;

static int __cmd_channel[2] = {-1, -1};
static int __data_channel[2] = {-1, -1};
#define C_IN	(__cmd_channel[STDIN_FILENO])
#define C_OUT	(__cmd_channel[STDOUT_FILENO])
#define D_IN	(__data_channel[STDIN_FILENO])
#define D_OUT	(__data_channel[STDOUT_FILENO])

#define xclose(x)	do {close(x); x = -1;} while(0)

/* 
 * xread never return failure or 0
 */
static int
xread(int fd, void * buf, int len)
{
	int err;
	assert(buf != NULL);
	if (len == 0)
		return 0;
	assert(len > 0);

	TRACE(RESOURCE, "try to xread %d bytes from %d\n",
			len, fd);

	err = read(fd, buf, len);
	if (err < 0) {
		THROW(EXP_RESOURCE_PROCESS_FAILURE,
				"read from pipe error: %d:%s\n",
				err, strerror(errno));
	}

	if (err == 0)
		THROW(EXP_RESOURCE_PEER_SHUTDOWN,
				"seems host peer has shutdown\n");
	TRACE(RESOURCE, "finish to xread %d bytes from %d\n",
			len, fd);
	return err;
}

/* 
 * write is very different from read
 *
 * xwrite never return failure
 */
static int
xwrite(int fd, void * buf, int len)
{
	int err;
	fd_set set;

	assert(fd < FD_SETSIZE);
	assert(buf != NULL);
	if (len == 0)
		return 0;
	assert(len > 0);

	TRACE(RESOURCE, "try to xwrite %d bytes to %d\n",
			len, fd);
	struct timeval tv;
	do {
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		FD_ZERO(&set);
		FD_SET(fd, &set);
		err = TEMP_FAILURE_RETRY(
				select(FD_SETSIZE, NULL,
					&set, NULL,
					&tv));
		if (err == 0)
			WARNING(RESOURCE, "select timeout when write, wait for another 1s\n");
	} while (err == 0);

	if (err < 0)
		THROW(EXP_RESOURCE_PROCESS_FAILURE,
				"select error: %d:%s",
				err, strerror(errno));

	assert(err == 1);

	err = write(fd, buf, len);
	if (err < 0) {
		THROW(EXP_RESOURCE_PROCESS_FAILURE,
				"write to pipe error: %d/%d:%s",
				err, len, strerror(errno));
	}

	if (err == 0)
		THROW(EXP_RESOURCE_PROCESS_FAILURE,
				"very strange: write returns 0");
	TRACE(RESOURCE, "finish to xwrite %d bytes to %d\n",
			len, fd);
	return err;
}


static void
xxwrite(int fd, void * buf, int len)
{
	TRACE(RESOURCE, "try to xxwrite %d bytes to %d\n",
			len, fd);
	while (len > 0) {
		int sz = xwrite(fd, buf, len);
		len -= sz;
		buf += sz;

	}
	TRACE(RESOURCE, "finish to xxwrite some bytes to %d\n", fd);
}

/* 
 * peri_wait_usec: periodically wait time
 * 0: wait forever
 */
static void
__xxread(int fd, void * buf, int len, int peri_wait_usec)
{
	int err;
	fd_set fdset;
	assert(fd < FD_SETSIZE);


	TRACE(RESOURCE, "try to __xxread %d bytes from %d\n",
			len, fd);

	assert(peri_wait_usec >= 0);
	struct timeval tv;

	int nr_warn = 0;
	while (len > 0) {
		do {
			if (peri_wait_usec == 0) {
				tv.tv_sec = 5;
				tv.tv_usec = 0;
			} else {
				tv.tv_sec = peri_wait_usec / 1000;
				tv.tv_usec = peri_wait_usec % 1000;
			}
			FD_ZERO(&fdset);
			FD_SET(fd, &fdset);

			/* periodically select */
			err = TEMP_FAILURE_RETRY(
					select(FD_SETSIZE, &fdset,
						NULL, NULL,
						&tv));

			if (err < 0)
				THROW(EXP_RESOURCE_PROCESS_FAILURE,
						"select error: %d:%s",
						err, strerror(errno));
			if (err == 0) {
				if (peri_wait_usec == 0) {
					DEBUG(RESOURCE, "i am still alive\n");
				} else {
					WARNING(RESOURCE, "select timeout, wait for another %d ms\n",
							peri_wait_usec);
					nr_warn ++;
					if (nr_warn >= 3)
						THROW(EXP_RESOURCE_PROCESS_FAILURE,
								"no response for too long time, something error");
				}
			}
		} while (err <= 0);
		assert(err == 1);

		err = xread(fd, buf, len);
		len -= err;
		buf += err;
	}
	assert(len == 0);
	TRACE(RESOURCE, "finish to __xxread some bytes from %d\n", fd);
}

static inline void
xxread(int fd, void * buf, int len)
{
	__xxread(fd, buf, len, 0);
}

static inline void
xxread_imm(int fd, void * buf, int len)
{
	__xxread(fd, buf, len, 1000);
}


static ssize_t
xxreadv(int fd, struct iovec * iovec,
		int nr)
{
	ssize_t retval;
	retval = readv(fd, iovec, nr);
	if (retval <= 0)
		THROW(EXP_RESOURCE_PROCESS_FAILURE,
				"readv failed, return %d:%s",
				retval, strerror(errno));

	size_t total_sz = 0;
	for (int i = 0; i < nr; i++)
		total_sz += iovec[i].iov_len;
	TRACE(RESOURCE, "readv: read %d bytes, return %d\n",
			total_sz, retval);
	if (retval >= total_sz)
		return retval;

	size_t total_read = retval;
	size_t s = 0;
	/* use normal read to read left data */
	for (int i = 0; i < nr; i++) {
		s += iovec[i].iov_len;
		if (s > total_read) {
			int len = s - total_read;
			void * ptr = iovec[i].iov_base + iovec[i].iov_len - len;
			xxread_imm(fd, ptr, len);
			total_read += len;
		}
	}
	return total_read;
}

static ssize_t
xxwritev(int fd, struct iovec * iovec,
		int nr)
{
	ssize_t retval = 0;
	TRACE(RESOURCE, "xxwritev %d iovecs to %d\n",
			nr, fd);
	WARNING(RESOURCE, "start writev: nr_iovec=%d\n", nr);

	for (int i = 0; i < nr; i++) {
		WARNING(RESOURCE, "iovec %d: %p:%d\n", i,
				iovec[i].iov_base,
				iovec[i].iov_len);
	}
//#ifdef HAVE_VMSPLICE
#if 1
	retval = vmsplice(fd, iovec, nr, 0);
#else
	retval = writev(fd, iovec, nr);
#endif
	WARNING(RESOURCE, "writev over, retval=%d\n", retval);

	if (retval <= 0)
		THROW(EXP_RESOURCE_PROCESS_FAILURE, "writev failed: return %d:%s",
				retval, strerror(errno));

	size_t total_sz = 0;
	for (int i = 0; i < nr; i++)
		total_sz += iovec[i].iov_len;
	TRACE(RESOURCE, "writev: write %d bytes, return %d\n",
			total_sz, retval);
	if (retval >= total_sz)
		return retval;

	size_t total_write = retval;
	size_t s = 0;
	/* use normal write to read left data */
	for (int i = 0; i < nr; i++) {
		s += iovec[i].iov_len;
		if (s > total_write) {
			int len = s - total_write;
			void * ptr = iovec[i].iov_base + iovec[i].iov_len - len;
			xxwrite(fd, ptr, len);
			total_write += len;
		}
	}
	return total_write;
}


static void
sigpipe_handler(int signum)
{
	WARNING(RESOURCE, "signal %d received by process %d\n", signum,
			getpid());
	THROW(EXP_RESOURCE_PROCESS_FAILURE,
			"receive SIGPIPE, write when other end closed");
}

/* ******************************** */
static struct io_t res_data_io;

static int
read_data(struct io_t * io, void * ptr,
		int size, int nr)
{
	assert(io == &res_data_io);
	assert(ptr != NULL);
	if (size * nr == 0)
		return 0;
	xxread_imm(D_IN, ptr, size * nr);
	return nr;
}

static int
write_data(struct io_t * io, void * ptr,
		int size, int nr)
{
	assert(io == &res_data_io);
	assert(ptr != NULL);
	if (size * nr == 0)
		return 0;
	xxwrite(D_OUT, ptr, size * nr);
	return nr;
}

static ssize_t
readv_data(struct io_t * io, struct iovec * iovec,
		int nr)
{
	assert(io == &res_data_io);
	if (nr <= 0)
		return 0;
	assert(iovec != NULL);

	return xxreadv(D_IN, iovec, nr);
}


static ssize_t
writev_data(struct io_t * io, struct iovec * iovec, int nr)
{
	assert(io == &res_data_io);
	if (nr <= 0)
		return 0;
	assert(iovec != NULL);
	return xxwritev(D_OUT, iovec, nr);
}


static struct io_functionor_t res_data_io_functionor = {
	.name = "ResourceIOFunctionor",
	.fclass = FC_IO,
	.check_usable = NULL,
	.open = NULL,
	.open_write = NULL,
	.read = read_data,
	.write = write_data,
	.readv = readv_data,
	.writev = writev_data,
	.seek = NULL,
	.close = NULL,
};

static struct io_t res_data_io = {
	.functionor = &res_data_io_functionor,
	.rdwr = IO_READ | IO_WRITE,
	.pprivate = NULL,
};

/* ******************************** */
/* CACHE!!! */

static struct cache_t res_cache;

/* ******************************** */

static void
read_resource_worker(const char * id)
{
	assert(id != NULL);
	DEBUG(RESOURCE, "read resource %s\n", id);

	struct cache_entry_t * ce = cache_get_entry(
			&res_cache, id);

	if (ce != NULL) {
		struct resource_t * r = container_of(ce,
				struct resource_t,
				cache_entry);
		r->serialize(r, &res_data_io);
	} else {
		
		struct resource_t * r = NULL;

		r = load_resource(id);

		assert(r != NULL);
		/* cache it */
		struct cache_entry_t * ce = &r->cache_entry;
		ce->id = r->id;
		ce->data = r;
		ce->sz = r->res_sz;
		ce->destroy_arg = r;
		ce->destroy = (cache_destroy_t)(r->destroy);
		ce->cache = NULL;
		ce->pprivate = NULL;

		cache_insert(&res_cache, &r->cache_entry);
		r->serialize(r, &res_data_io);
	}
}


static void
delete_resource_worker(const char * __id)
{
	DEBUG(RESOURCE, "delete resource %s\n", __id);
	cache_remove_entry(&res_cache, __id);
}

static void
worker(void)
{
	VERBOSE(RESOURCE, "resource process start working\n");

	/* push the ok sig */
	uint32_t ok = OK_SIG;
	xxwrite(D_OUT, &ok, sizeof(ok));

	for(;;) {
		int err;
		int cmd_len;
		char cmd[MAX_IDLEN];

		/* 
		 * cmd syntax:
		 * read resource: r:[format]:[type]:[proto]:[id]
		 * exit: x
		 */
		xxread(C_IN, &cmd_len, sizeof(cmd_len));

		if ((cmd_len > MAX_IDLEN) || (cmd_len < 0))
			THROW(EXP_RESOURCE_PROCESS_FAILURE,
					"cmd_len=%d is incorrect",
					err);

		/* read the command */
		xxread(C_IN, cmd, cmd_len);
		DEBUG(RESOURCE, "command: %s\n", cmd);

		switch (cmd[0]) {
			case 'x':
				THROW(EXP_RESOURCE_PEER_SHUTDOWN,
						"we are shutted down by command x\n");
				break;
			case 'r':
				assert(cmd[1] == ':');
				read_resource_worker(&cmd[2]);
				break;
			case 'd':
				assert(cmd[1] == ':');
				delete_resource_worker(&cmd[2]);
				break;
			default:
				THROW(EXP_RESOURCE_PROCESS_FAILURE,
						"got unknown command '%c'", cmd[0]);
		}
	}
}


/* *************************************************************8 */

int
launch_resource_process(void)
{
	int err;
	VERBOSE(RESOURCE, "launching resource process\n");

	/* create the pipes */
	err = pipe(__cmd_channel);
	assert(err == 0);
	err = pipe(__data_channel);
	assert(err == 0);

	/* regist the SIGPIPE handler */
	signal(SIGPIPE, sigpipe_handler);

	/* fork */
	resproc_pid = fork();
	assert(resproc_pid >= 0);
	if (resproc_pid != 0) {
		/* father */
		VERBOSE(RESOURCE, "resource process created: pid=%d\n",
				resproc_pid);
		/* close the unneed pipe */
		xclose(C_IN);
		xclose(D_OUT);
		/* wait for the first byte and return */
		uint32_t ok = 0;
		xxread(D_IN, &ok, sizeof(ok));
		if (ok != OK_SIG) {
			xclose(C_OUT);
			xclose(D_IN);
			THROW(EXP_RESOURCE_PROCESS_FAILURE,
					"seems resource process meet some error, it return 0x%x\n",
					ok);
		}
		DEBUG(RESOURCE, "got ok_sig=0x%x\n", ok);
		return resproc_pid;
	}

	/* child */
	/* reinit debug */
	dbg_init("/tmp/yaavg_resource_log");

	/* cache, default: 10M */
	cache_init(&res_cache, "resource cache",
			conf_get_int("sys.mem.threshold", 0xa00000));

	/* close the other end */
	xclose(C_OUT);
	xclose(D_IN);

	struct exception_t exp;
	TRY(exp) {
		VERBOSE(RESOURCE, "resource process is started\n");
		/* begin the working flow */
		worker();
	} FINALLY {
		xclose(C_IN);
		xclose(D_OUT);
	} CATCH(exp) {
		switch (exp.type) {
			case EXP_RESOURCE_PEER_SHUTDOWN:
				VERBOSE(RESOURCE, "resource process end normally\n");
				do_cleanup();
				break;
			default:
				print_exception(&exp);
				do_cleanup();
		}
	}

	exit(0);
	return 0;
}

void
shutdown_resource_process(void)
{
	VERBOSE(RESOURCE, "shutting down resource process %d, C_OUT=%d\n", resproc_pid,
			C_OUT);
	if (C_OUT == -1)
		return;
	if (resproc_pid == 0)
		return;

	char x[2] = {'x', '\0'};
	int i = 2;
	xxwrite(C_OUT, &i, sizeof(i));
	xxwrite(C_OUT, &x, sizeof(x));
	VERBOSE(RESOURCE, "wait for process %d finish\n", resproc_pid);
	waitpid(resproc_pid, NULL, 0);

	resproc_pid = -1;
	C_OUT = -1;
	return;
}

void
delete_resource(const char * name)
{
	struct iovec vecs[3];
	static char d_cmd[2] = "d:";

	int len = 3 + strlen(name);

	vecs[0].iov_base = &len;
	vecs[0].iov_len = sizeof(len);

	vecs[1].iov_base = d_cmd;
	vecs[1].iov_len = 2;

	vecs[2].iov_base = (void*)name;
	vecs[2].iov_len = len - 2;

	xxwritev(C_OUT, vecs, 3);
}


void *
get_resource(const char * name,
		deserializer_t deserializer)
{
	struct iovec vecs[3];
	static char d_cmd[2] = "r:";

	int len = 3 + strlen(name);

	vecs[0].iov_base = &len;
	vecs[0].iov_len = sizeof(len);

	vecs[1].iov_base = d_cmd;
	vecs[1].iov_len = 2;

	vecs[2].iov_base = (void*)name;
	vecs[2].iov_len = len - 2;

	xxwritev(C_OUT, vecs, 3);

	return deserializer(&res_data_io);
}

// vim:ts=4:sw=4

