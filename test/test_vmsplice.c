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
#define NR 100000
//#define NR 1
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
		for (int i = 0; i < NR; i++) {
#ifdef NORMAL_WRITE
			err = write(pp[POUT], data_send, SZ);
			assert(err == SZ);
#else
			int left = SZ;
			x = 0;
			while (left > 0) {
				struct iovec vec;
				vec.iov_base = data_send + (SZ - left);
				vec.iov_len = left;
#ifdef VMSPLICE
				err = vmsplice(pp[POUT],
						&vec, 1, 0);
#else
				err = writev(pp[POUT],
						&vec, 1);
#endif
				assert(err >= 0);
				left -= err;
				x++;
			}
//			if (x > 1)
//				printf("send as %d parts, last err = %d\n", x, err);
#endif
		}
	} else {
		close(pp[POUT]);
		for (int i = 0; i < NR; i++) {
			int left = SZ;
			void * ptr = data_recv;
			x = 0;
			while (left > 0) {
				err = read(pp[PIN], ptr, left);
				assert(err >= 0);
				left -= err;
				ptr += err;
				x++;
			}

#if 0
			if (x > 1)
				printf("read for %d parts\n", x);
			err = memcmp(data_send, data_recv, SZ);
			if (err != 0) {
				printf("err=%d\n", err);
				FILE * fp = fopen("/tmp/send", "wb");
				fwrite(data_send, SZ, 1, fp);
				fclose(fp);

				fp = fopen("/tmp/recv", "wb");
				fwrite(data_recv, SZ, 1, fp);
				fclose(fp);
				assert(err == 0);
			}
#endif
		}
	}

	return 0;
}

// vim:ts=4:sw=4

