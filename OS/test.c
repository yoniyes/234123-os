#include "pattach.h"

void test_get_child_process_count() {
	printf("Testing [ get_child_process_count ] ...\n");
	pid_t PID = fork();
	if (PID == 0) {
		printf("I'm only a child!\n");
	} else {
		wait(PID);
		printf("Children: %d\n", get_child_process_count() );
	}
}

int main() {
	// printf("Testing [ attach_proc ] ...\n");
	// printf("PID:\t%d\n", attach_proc(1234));
	// test_get_child_process_count();
	printf("Testing [ get_child_processes ] ...\n");
	pid_t result[10];
	unsigned int max_length = 2;
	int PID = fork();
	if (PID == 0) {
		printf("Just a child!\n");
	} else {
		// wait(PID);
		int num = get_child_processes(result, max_length);
		printf("Num of child processes: %d\n", num);
		int i;
		for (i = 0; i < num; i++)
			printf("PID[%d] = %d\n", i, result[i]);
	}
	// printf("Finished\n");
	return 0;
}
