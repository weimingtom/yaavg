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

#include <config.h>
#include <common/defs.h>
#include <common/debug.h>
#include <common/exception.h>
#include <common/init_cleanup_list.h>
#include <common/mm.h>
#include <common/dict.h>
#include <common/cache.h>
#include <resource/resource.h>


static pid_t resproc_pid;

static int cmd_channel[2];
static int data_channel[2];
#define C_IN	(cmd_channel[STDIN_FILENO])
#define C_OUT	(cmd_channel[STDOUT_FILENO])
#define D_IN	(data_channel[STDIN_FILENO])
#define D_OUT	(data_channel[STDOUT_FILENO])

static int
xread(int fd, void * buf, int len)
{
	int err;
	assert(buf != NULL);
	if (len == 0)
		return 0;
	assert(len > 0);

	err = read(fd, buf, len);
	if (err < 0) {
		THROW(EXP_RESOURCE_PROCESS_FAILURE,
				"read from pipe error: %d:%s\n",
				err, strerror(errno));
	}

	if (err == 0)
		THROW(EXP_RESOURCE_HOST_END,
				"seems host process has ended\n");
	return err;
}

/* 
 * write is very different from read
 */
static int
xwrite(int fd, void * buf, int len)
{
	int err;
	fd_set set;
	FD_ZERO(&set);
	FD_SET(fd, &set);
	assert(fd < FD_SETSIZE);

	assert(buf != NULL);
	if (len == 0)
		return 0;
	assert(len > 0);

	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	do {
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
				"write from pipe error: %d/%d:%s\n",
				err, len, strerror(errno));
	}

	if (err == 0)
		THROW(EXP_RESOURCE_PROCESS_FAILURE,
				"very strange: write returns 0");
	return err;
}


static void
xxwrite(int fd, void * buf, int len)
{
	int err;
	while (len > 0)
		len -= xwrite(fd, buf, len);
}

static void
xxread(int fd, void * buf, int len)
{
	int err;
	fd_set set;
	FD_ZERO(&set);
	FD_SET(fd, &set);
	assert(fd < FD_SETSIZE);

	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	while (len > 0) {
		err = TEMP_FAILURE_RETRY(
				select(FD_SETSIZE, &set,
					NULL, NULL,
					&tv));
		if (err < 0)
			THROW(EXP_RESOURCE_PROCESS_FAILURE,
					"select error: %d:%s",
					err, strerror(errno));
		if (err == 0) {
			WARNING(RESOURCE, "select timeout when read, wait for another 1s\n");
			continue;
		}
		assert(err == 1);
		err = xread(fd, buf, len);
		assert(err != 0);
		len -= err;
	}
}

static void
sigpipe_handler(int signum)
{
	WARNING(RESOURCE, "signal %d received\n", signum);
}

static void
work(void)
{
	VERBOSE(RESOURCE, "resource process start working\n");


	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(C_IN, &fdset);


	while (1) {
		int err;
		int cmd_len;
		char cmd[MAX_IDLEN];

		struct timeval tv;
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		while (1) {
			err = TEMP_FAILURE_RETRY(select(FD_SETSIZE, &fdset,
					NULL, NULL, &tv));
			if (err != 0)
				break;
			DEBUG(RESOURCE, "resource process is still alive\n");
		}

		/* select won't failure exception real error, that is,
		 * even if the other size of the pipe is closed, select still
		 * return 1, but the incoming read will return 0. */
		if (err != 1)
			THROW(EXP_RESOURCE_PROCESS_FAILURE,
					"wait for command but failed");
		/* 
		 * cmd syntax:
		 * read resource: r:[format]:[type]:[proto]:[id]
		 * exit: x
		 */

#if 0
		err = xread(C_IN, &cmd_len, sizeof(cmd_len));
		assert(err == sizeof(cmd_len));

		if ((cmd_len > MAX_IDLEN) || (cmd_len < 0))
			THROW(EXP_RESOURCE_PROCESS_FAILURE,
					"cmd_len=%d is incorrect",
					err);

		/* read the command */
		xxread(C_IN, cmd, cmd_len);
		DEBUG(RESOURCE, "command: %s\n", cmd);

#endif
	}
}

void
launch_resource_process(void)
{
	int err;
	VERBOSE(RESOURCE, "launching resource process\n");

	/* create the pipes */
	err = pipe(cmd_channel);
	assert(err == 0);
	err = pipe(data_channel);
	assert(err == 0);

	/* fork */
	resproc_pid = fork();
	assert(resproc_pid >= 0);
	if (resproc_pid != 0) {
		/* father */
		VERBOSE(RESOURCE, "resource process created: pid=%d\n",
				resproc_pid);
		/* close the unneed pipe */
		close(C_IN);
		close(D_OUT);
		return;
	}

	/* child */
	/* reinit debug */
	dbg_init(NULL);

	/* regist the SIGPIPE handler */
	signal(SIGPIPE, sigpipe_handler);

	/* close the other end */
	close(C_OUT);
	close(D_IN);

	struct exception_t exp;
	TRY(exp) {
		VERBOSE(RESOURCE, "resource process is started\n");
		/* begin the working flow */
		work();
	} FINALLY {
		WARNING(RESOURCE, "resource process cleanup exitted\n");
	} CATCH(exp) {
		switch (exp.type) {
			case EXP_RESOURCE_HOST_END:
				VERBOSE(RESOURCE, "resource process end normally\n");
				do_cleanup();
				break;
			default:
				print_exception(&exp);
				do_cleanup();
		}
	}
	return;
}

// vim:ts=4:sw=4

