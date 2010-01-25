#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/uio.h>

#define PIN STDIN_FILENO
#define POUT STDOUT_FILENO

//#define NR 100
//#define NR 100000
#define NR 20
#define SZ ((10 << 10) * 8)
static int pp[2];

static char data_send[SZ];
static char data_recv[SZ];

int main()
{
	int err;
	struct timeval TP0, TP1;
	int ms;
	int x = 0;
	err = pipe(pp);
	assert(err == 0);

	for (int i = 0; i < SZ; i++) {
		data_send[i] = i % 7 + '0';
		if (i % 7 == 6)
			data_send[i] = '\n';
	}

	pid_t chld_pid = fork();
	assert(chld_pid >= 0);
	if (chld_pid == 0) {
		close(pp[PIN]);
		char cc[10];
		for (int i = 0; i < NR; i++) {
			memset(cc, '1', 9);
			cc[9] = '\0';

			struct iovec vec;
			vec.iov_base = cc;
			vec.iov_len = 10;
			vmsplice(pp[POUT], &vec, 1, 0);
			memset(cc, '0' + i, 9);
			sleep(2);
		}
	} else {
		close(pp[POUT]);
		for (int i = 0; i < NR; i++) {
			char cc[10];
			sleep(1);
			read(pp[PIN], cc, 10);
			printf("%s\n", cc);
		}
	}

	return 0;
}

// vim:ts=4:sw=4

