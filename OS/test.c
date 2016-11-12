#include "pattach.h"

int main() {
	printf("Testing [ attach_proc ] ...\n");
	if ( attach_proc(1234) ) {
		return 1;
	}
	printf("Testing [ get_child_processes ] ...\n");
	pid_t* result = malloc( sizeof(pid_t) * 10 );
	unsigned int max_length = 10;
	if ( get_child_processes(result, max_length) ) {
		return 1;
	}
	printf("Testing [ get_child_process_count ] ...\n");
	if ( get_child_process_count() ) {
		return 1;
	}
	return 0;
}