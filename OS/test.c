#include "pattach.h"

void test_get_child_process_count() {
	printf("Testing [ get_child_process_count ] ...\n");
	pid_t PID = fork();
	if (PID == 0) {
		// printf("I'm only a child!\n");
	} else {
		wait(PID);
		printf("\tChildren: %d\n", get_child_process_count() );
	}
}

void test_get_child_processes() {
	printf("Testing [ get_child_processes ] ...\n");
	pid_t result[10];
	unsigned int max_length = 2;
	int PID = fork();
	if (PID == 0) {
		// printf("Just a child!\n");
	} else {
		// wait(PID);
		int num = get_child_processes(result, max_length);
		printf("\tNum of child processes: %d\n", num);
		int i;
		for (i = 0; i < num; i++)
			printf("\tPID[%d] = %d\n", i, result[i]);
	}	
}

int main() {
	printf("Testing [ attach_proc ] ...\n");
	int father = getpid();
	printf("\n\n%d\n\n", father);
	int first = fork(), second;
	if (first != 0) {
	   second = fork();
	   if (second == 0) {
	        printf("first:\t%d\n", first);
		printf("Former parent:\t%d\n", attach_proc(first));
        	printf("Original father:\t%d\n", father);
	   } else {
	      while(1) {
		 ;
	      }
	   }
	} else {
	   printf("first_father:\t%d\n", getppid());
	   while(1) {
	      ;
	   }
	}
	//test_get_child_process_count();
	//test_get_child_processes();
	// printf("Finished\n");
	return 0;
}
