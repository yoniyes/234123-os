#include "hw2_syscalls.h"
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
// #include <sched.h>

typedef struct sched_param {
	int sched_priority, requested_time;
} sched_param_t;

int main() {
	printf("current process is short?\n%d\n", is_short(getpid()));
	printf("errno = %d\n", errno);
	sched_param_t shorty;
	shorty.sched_priority = 0;
	shorty.requested_time = 20;
	sched_setscheduler(getpid(), 5, &shorty);
	printf("errno = %d\n", errno);
	printf("current process is short?\n%d\n", is_short(getpid()));
	// pid_t pid = fork();
	// if (pid) {

	// }
	return 0;
}
