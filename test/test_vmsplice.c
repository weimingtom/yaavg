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

//#define NR 1000000
//#define NR 10000
#define NR 1
#define SZ (10 << 10)
static int pp[2];

static int data_send[SZ];
static int data_recv[SZ];

int main()
{
	int err;
	struct timeval TP0, TP1;
	int ms;
	err = pipe(pp);
	assert(err == 0);

	pid_t chld_pid = fork();
	assert(chld_pid >= 0);
	if (chld_pid == 0) {
		close(pp[PIN]);
		for (int i = 0; i < NR; i++) {
#ifdef NORMAL_WRITE
			err = write(pp[POUT], data_send, SZ);
			assert(err == SZ);
#else
			int left = SZ;
			while (left > 0) {
				struct iovec vec;
				vec.iov_base = data_send + (SZ - left);
				vec.iov_len = left;
				err = vmsplice(pp[POUT],
						&vec, 1, 0);
				assert(err >= 0);
				left -= err;
			}
#endif
		}
	} else {
		close(pp[POUT]);
		for (int i = 0; i < NR; i++) {
			int left = SZ;
			void * ptr = data_recv;
			while (left > 0) {
				err = read(pp[PIN], ptr, left);
				assert(err >= 0);
				left -= err;
				ptr += err;
			}
		}
	}

	return 0;
}

// vim:ts=4:sw=4

