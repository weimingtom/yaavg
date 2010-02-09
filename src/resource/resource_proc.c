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
#include <utils/timer.h>
#include <yconf/yconf.h>

#include <io/io.h>

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
		THROW_FATAL(EXP_RESOURCE_PROCESS_FAILURE,
				"read from pipe error: %d:%s\n",
				err, strerror(errno));
	}

	if (err == 0)
		THROW_FATAL(EXP_RESOURCE_PEER_SHUTDOWN,
				"seems host peer has shutdown first\n");
	TRACE(RESOURCE, "finish to xread %d bytes from %d\n",
			err, fd);
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
	int err = 0;
	fd_set set;

	assert(fd < FD_SETSIZE);
	assert(buf != NULL);
	if (len == 0)
		return 0;
	assert(len > 0);

	TRACE(RESOURCE, "try to xwrite %d bytes to %d\n",
			len, fd);
	struct timeval tv;
	for (int i = 0; ((i < 3) && (err == 0)); i++) {
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
	}
	if (err < 0)
		THROW_VAL_FATAL(EXP_RESOURCE_PROCESS_FAILURE, RESOURCEEXP_TIMEOUT,
				"select error: %d:%s",
				err, strerror(errno));

	assert(err == 1);

	err = write(fd, buf, len);
	if (err < 0) {
		THROW_FATAL(EXP_RESOURCE_PROCESS_FAILURE,
				"write to pipe error: %d/%d:%s",
				err, len, strerror(errno));
	}

	if (err == 0)
		THROW_FATAL(EXP_RESOURCE_PROCESS_FAILURE,
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
				THROW_FATAL(EXP_RESOURCE_PROCESS_FAILURE,
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
						THROW_VAL_FATAL(EXP_RESOURCE_PROCESS_FAILURE, RESOURCEEXP_TIMEOUT,
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
		THROW_FATAL(EXP_RESOURCE_PROCESS_FAILURE,
				"readv failed, return %d:%s",
				retval, strerror(errno));

	ssize_t total_sz = 0;
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

static inline ssize_t
xx_splicev(int fd, struct iovec * __iovec,
		int __nr, bool_t use_vmsplice, bool_t write)
{
	ssize_t retval = 0;
	size_t total_spliced = 0;
	TRACE(RESOURCE, "xx_splicev %d iovecs to %d\n",
			__nr, fd);
	struct iovec * iovec = alloca(sizeof(*iovec) * __nr);
	memcpy(iovec, __iovec, sizeof(*iovec) * __nr);
	int nr = __nr;

#ifdef HAVE_VMSPLICE
	if (use_vmsplice)
		TRACE(RESOURCE, "using vmsplice\n");
#endif
	do {
#ifdef HAVE_VMSPLICE
		if (use_vmsplice) {
			retval = vmsplice(fd, iovec, nr, 0);
		}
		else
#endif
		if (write)
			retval = writev(fd, iovec, nr);
		else
			retval = readv(fd, iovec, nr);

		if (retval < 0) {
			THROW_FATAL(EXP_RESOURCE_PROCESS_FAILURE, "readv/writev/vmsplice failed: return %d:%s",
					retval, strerror(errno));
		} else if (retval == 0) {
			WARNING(RESOURCE, "readv/writev/vmsplice return 0, very strange, sleep 0.01 sec\n");
			force_delay(10);
		}
		total_spliced += retval;

		/* compute bytes left */
		int transfered_sz = 0;
		int i;
		for (i = 0; i < nr; i++) {
			transfered_sz += iovec[i].iov_len;
			if (transfered_sz > retval) {
				/* adjust current iovec */
				size_t left = transfered_sz - retval;
				iovec[i].iov_base +=
					iovec[i].iov_len - left;
				iovec[i].iov_len = left;
				break;
			}
		}
		nr = (nr - i);
		iovec += i;
	} while (nr > 0);

	TRACE(RESOURCE, "xx_splicev over, total splice %d bytes\n",
			total_spliced);
	return total_spliced;
}

static ssize_t
xxwritev(int fd, struct iovec * iovec,
		int nr)
{
	return xx_splicev(fd, iovec, nr, FALSE, TRUE);
}

#ifdef HAVE_VMSPLICE
static ssize_t
xxvmsplice_read(int fd, struct iovec * iovec,
		int nr)
{
	return xx_splicev(fd, iovec, nr, TRUE, FALSE);
}

static ssize_t
xxvmsplice_write(int fd, struct iovec * iovec,
		int nr)
{
	return xx_splicev(fd, iovec, nr, TRUE, TRUE);
}
#endif


static void
sigpipe_handler(int signum)
{
	WARNING(RESOURCE, "signal %d received by process %d\n", signum,
			getpid());
	THROW_FATAL(EXP_RESOURCE_PROCESS_FAILURE,
			"receive SIGPIPE, write when other end closed");
}

/* ******************************** */
static struct io_t data_side_io;
static struct io_t cmd_side_io;

static inline int
__ioread(int fd, void * ptr, int size, int nr)
{
	assert(ptr != NULL);
	if (size * nr == 0)
		return 0;
	xxread_imm(fd, ptr, size * nr);
	return nr;
}

static inline int
__iowrite(int fd, void * ptr, int size, int nr)
{
	assert(ptr != NULL);
	if (size * nr == 0)
		return 0;
	xxwrite(fd, ptr, size * nr);
	return nr;
}


static inline ssize_t
__ioreadv(int fd, struct iovec * iovec, int nr)
{
	if (nr <= 0)
		return 0;
	assert(iovec != NULL);
	return xxreadv(fd, iovec, nr);
}

static inline ssize_t
__iowritev(int fd, struct iovec * iovec, int nr)
{
	if (nr <= 0)
		return 0;
	assert(iovec != NULL);
	return xxwritev(fd, iovec, nr);
}

#ifdef HAVE_VMSPLICE
static inline ssize_t
__iovmsplice_read(int fd, struct iovec * iovec, int nr)
{
	if (nr <= 0)
		return 0;
	assert(iovec != NULL);
	return xxvmsplice_read(fd, iovec, nr);
}

static inline ssize_t
__iovmsplice_write(int fd, struct iovec * iovec, int nr)
{
	if (nr <= 0)
		return 0;
	assert(iovec != NULL);
	return xxvmsplice_write(fd, iovec, nr);
}

#endif



static int
read_data(struct io_t * io, void * ptr,
		int size, int nr)
{
	assert(io == &cmd_side_io);
	return __ioread(D_IN, ptr, size, nr);
}

static int
read_cmd(struct io_t * io, void * ptr,
		int size, int nr)
{
	assert(io == &data_side_io);
	return __ioread(C_IN, ptr, size, nr);
}

static int
write_data(struct io_t * io, void * ptr,
		int size, int nr)
{
	assert(io == &data_side_io);
	return __iowrite(D_OUT, ptr, size, nr);
}

static int
write_cmd(struct io_t * io, void * ptr,
		int size, int nr)
{
	assert(io == &cmd_side_io);
	return __iowrite(C_OUT, ptr, size, nr);
}

static ssize_t
readv_data(struct io_t * io, struct iovec * iovec,
		int nr)
{
	assert(io == &cmd_side_io);
	return __ioreadv(D_IN, iovec, nr);
}

static ssize_t
readv_cmd(struct io_t * io, struct iovec * iovec,
		int nr)
{
	assert(io == &data_side_io);
	return __ioreadv(C_IN, iovec, nr);
}

static ssize_t
writev_data(struct io_t * io, struct iovec * iovec, int nr)
{
	assert(io == &data_side_io);
	return __iowritev(D_OUT, iovec, nr);
}

static ssize_t
writev_cmd(struct io_t * io, struct iovec * iovec, int nr)
{
	assert(io == &cmd_side_io);
	return __iowritev(C_OUT, iovec, nr);
}

#ifdef HAVE_VMSPLICE
static ssize_t
vmsplice_write_data(struct io_t * io, struct iovec * iovec, int nr)
{
	assert(io == &data_side_io);
	return __iovmsplice_write(D_OUT, iovec, nr);
}

static ssize_t
vmsplice_write_cmd(struct io_t * io, struct iovec * iovec, int nr)
{
	assert(io == &cmd_side_io);
	return __iovmsplice_write(C_OUT, iovec, nr);
}

static ssize_t
vmsplice_read_data(struct io_t * io, struct iovec * iovec, int nr)
{
	assert(io == &cmd_side_io);
	return __iovmsplice_read(D_IN, iovec, nr);
}

static ssize_t
vmsplice_read_cmd(struct io_t * io, struct iovec * iovec, int nr)
{
	assert(io == &data_side_io);
	return __iovmsplice_read(C_IN, iovec, nr);
}

#endif

static struct io_functionor_t data_side_io_functionor = {
	.name = "ResourceIODataSizeFunctionor",
	.fclass = FC_IO,
	.check_usable = NULL,
	.open = NULL,
	.open_write = NULL,
	.read = read_cmd,
	.write = write_data,
	.readv = readv_cmd,
	.writev = writev_data,
#ifdef HAVE_VMSPLICE
	.vmsplice_write = vmsplice_write_data,
	.vmsplice_read = vmsplice_read_cmd,
#endif
	.seek = NULL,
	.tell = NULL,
	.close = NULL,
};

static struct io_functionor_t cmd_side_io_functionor = {
	.name = "ResourceIOCmdSizeFunctionor",
	.fclass = FC_IO,
	.check_usable = NULL,
	.open = NULL,
	.open_write = NULL,
	.read = read_data,
	.write = write_cmd,
	.readv = readv_data,
	.writev = writev_cmd,
#ifdef HAVE_VMSPLICE
	.vmsplice_write = vmsplice_write_cmd,
	.vmsplice_read = vmsplice_read_data,
#endif
	.seek = NULL,
	.tell = NULL,
	.close = NULL,
};


static struct io_t data_side_io = {
	.functionor = &data_side_io_functionor,
	.rdwr = IO_READ | IO_WRITE,
	.pprivate = NULL,
};

static struct io_t cmd_side_io = {
	.functionor = &cmd_side_io_functionor,
	.rdwr = IO_READ | IO_WRITE,
	.pprivate = NULL,
};


#if 0
#endif

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

	struct resource_t * r = NULL;
	if (ce != NULL) {
		r = container_of(ce,
				struct resource_t,
				cache_entry);
	} else {
		r = load_resource(id);

		assert(r != NULL);
		/* cache it */
		struct cache_entry_t * ce = &r->cache_entry;
		ce->id = r->id;
		ce->data = r;
		ce->sz = r->res_sz;
		ce->destroy = (cache_destroy_t)(r->destroy);
		ce->destroy_arg = r;
		ce->cache = NULL;
		ce->pprivate = NULL;

		cache_insert(&res_cache, &r->cache_entry);
	}
	r->serialize(r, &data_side_io);
}


static void
delete_resource_worker(const char * __id)
{
	DEBUG(RESOURCE, "delete resource %s\n", __id);
	cache_remove_entry(&res_cache, __id);
}

static void
get_package_items_worker(const char * cmd)
{
	struct exception_t exp;
	TRY(exp) {
		/* cmd should be something like: XP3|FILE:xxx.xp3 */
		/* split the '|' first */
		char * proto = strdupa(cmd);
		assert(proto != NULL);
		char * fn = __split_token(proto, '|');
		if (fn == NULL)
			THROW_TAINTED(EXP_RESOURCE_PROCESS_FAILURE,
					"received wrong package_items_worker command %s", cmd);
		struct io_functionor_t * iof = get_io_handler(proto);
		assert(iof != NULL);

		struct package_items_t * items = iof_get_package_items(iof, fn);
		assert(items != NULL);
		serialize_and_destroy_package_items(&items, &data_side_io);
	} FINALLY {
	} CATCH(exp) {
		/* **NOTICE** this is very different from others!!! */
		if (exp.level > EXP_LV_LOWEST)
			RETHROW(exp);
		print_exception(&exp);
		/* return an empty list to peer */
		struct package_items_t head;
		head.table = NULL;
		head.nr_items = 0;
		head.total_sz = sizeof(head);
		io_write_force(&data_side_io, &head, sizeof(head));
	}
	return;
}



static char * cmd = NULL;
static int cmd_buffer_sz = 0;
static void
worker(void)
{
	VERBOSE(RESOURCE, "resource process start working\n");

	/* push the ok sig */
	uint32_t ok = OK_SIG;
	xxwrite(D_OUT, &ok, sizeof(ok));

	for(;;) {
		int cmd_len;

		/* 
		 * cmd syntax:
		 * read resource: r:REANAME, see common/defs.h
		 * exit: x
		 */
		xxread(C_IN, &cmd_len, sizeof(cmd_len));

		if ((cmd_len > MAX_IDLEN) || (cmd_len < 0))
			THROW_FATAL(EXP_RESOURCE_PROCESS_FAILURE,
					"cmd_len=%d is incorrect",
					cmd_len);

		DEBUG(RESOURCE, "cmd_len=%d\n", cmd_len);
		if (cmd_len > cmd_buffer_sz)
			cmd = xrealloc(cmd, cmd_len);

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
			case 'l':
				assert(cmd[1] == ':');
				get_package_items_worker(&cmd[2]);
				break;
			default:
				THROW_FATAL(EXP_RESOURCE_PROCESS_FAILURE,
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
			THROW_FATAL(EXP_RESOURCE_PROCESS_FAILURE,
					"seems resource process meet some error, it return 0x%x\n",
					ok);
		}
		DEBUG(RESOURCE, "got ok_sig=0x%x\n", ok);
		return resproc_pid;
	}

	/* child */
	/* reinit debug */
	dbg_init("/tmp/yaavg_resource_log");
	//dbg_init(NULL);

	/* cache, default: 10M */
	cache_init(&res_cache, "resource cache",
			conf_get_int("sys.resource.cachesz", 0xa00000));

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
		xfree_null(cmd);
	} CATCH(exp) {
		VERBOSE(RESOURCE, "resource worker end\n");
		switch (exp.type) {
			case EXP_RESOURCE_PEER_SHUTDOWN:
				if (exp.level <= EXP_LV_LOWEST) {
					VERBOSE(RESOURCE, "resource process end normally\n");
					do_cleanup();
					break;
				}
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
	struct exception_t exp;
	TRY(exp) {
		xxwrite(C_OUT, &i, sizeof(i));
		xxwrite(C_OUT, &x, sizeof(x));
	} NO_FINALLY
	CATCH(exp) {
		if (exp.level >= EXP_LV_UNCATCHABLE)
			RETHROW(exp);
		/* doesn't allow exception raise here */
		print_exception(&exp);
	}
	VERBOSE(RESOURCE, "wait for process %d finish\n", resproc_pid);

	xclose(C_OUT);
	xclose(D_IN);


	for (int i = 0; i < 2; i++) {
		int ms = 0;
		int status = 0;
		pid_t retval = waitpid(resproc_pid, &status, WNOHANG);
		while (retval != resproc_pid) {
			if (ms >= 1000)
				break;
			force_delay(100);
			ms += 100;
			retval = waitpid(resproc_pid, &status, WNOHANG);
		}
		if (retval != resproc_pid) {
			/* kill it by sig 9 */
			if (i == 0) {
				WARNING(RESOURCE, "resource process %d is not end for 1 second, send SIGTERM\n", resproc_pid);
				kill(resproc_pid, SIGTERM);
			} else {
				WARNING(RESOURCE, "resource process %d is not end for another 1 second, send SIGKILL\n", resproc_pid);
				kill(resproc_pid, SIGKILL);
			}
		} else {
			resproc_pid = -1;
			break;
		}
	}
	if (resproc_pid != -1)
		ERROR(RESOURCE, "unable to kill resource process %d\n", resproc_pid);
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

	return deserializer(&cmd_side_io);
}

struct package_items_t *
get_package_items(const char * proto, const char * name)
{
	assert(proto != NULL);
	assert(name != NULL);
	struct iovec vecs[4];
	static char l_cmd[2] = "l:";
	int lp = strlen(proto);
	int len = 2 + lp + 1 + strlen(name) + 1;

	char * x_proto = alloca(lp + 2);
	assert(x_proto != NULL);
	strcpy(x_proto, proto);
	x_proto[lp] = '|';
	x_proto[lp + 1] = '\0';
	
	vecs[0].iov_base = &len;
	vecs[0].iov_len = sizeof(len);
	vecs[1].iov_base = l_cmd;
	vecs[1].iov_len = 2;
	vecs[2].iov_base = x_proto;
	vecs[2].iov_len = lp + 1;
	vecs[3].iov_base = (char*)name;
	vecs[3].iov_len = strlen(name) + 1;

	xxwritev(C_OUT, vecs, 4);
	
	/* deserialize received package_items */
	/* it suppresses all exceptions */
	return deserialize_package_items(&cmd_side_io);
}

// vim:ts=4:sw=4

