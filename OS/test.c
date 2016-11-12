#include "pattach.h"

int main() {
	printf("Testing [ attach_proc ] ...\n");
	printf("PID:\t%d\n", attach_proc(1234));
	printf("Testing [ get_child_processes ] ...\n");
	pid_t* result = malloc( sizeof(pid_t) * 10 );
	unsigned int max_length = 10;
	if ( get_child_processes(result, max_length) ) {
		return 1;
	}
	test_get_child_process_count();
	printf("Finished\n");
	return 0;
}

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