/**
 *	pattach.c
 *	OS 234120 HW1
 *	Yoni Yeshanov
 */

// #include "pattach.h"

/**
 *
 *	Actual implementation.
 *
 */
int sys_attach_proc (pid_t PID) {
	printk("PID:\t%d\n", PID);
	return 0;
}

/**
 *
 *	Actual implementation.
 *
 */
int sys_get_child_processes(pid_t* result, unsigned int max_length) {
	int size = 0;
	for (int i = 0; i < max_length; i++) {
		result[i] = size++;
	}
	printk("result array size:\t%d, max_length:\t%d\n", size * sizeof(*result), max_length);
	return 0;
}

/**
 *
 *	Actual implementation.
 *
 */
int sys_get_child_process_count() {
	printk("Got in.\n");
	return 0;
}
