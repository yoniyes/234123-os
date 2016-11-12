/**
 *	pattach.c
 *	OS 234120 HW1
 *	Yoni Yeshanov
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/sched.h>

/**
 *
 *	Actual implementation.
 *
 */
int sys_attach_proc (pid_t PID) {
	return (int) PID;
}

/**
 *
 *	Actual implementation.
 *
 */
int sys_get_child_processes(pid_t* result, unsigned int max_length) {
	int size = 0;
	int i;
	for (i = 0; i < max_length; i++) {
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
	//TODO: *********** list_entry(iterator, type, list head field name) ***********
	struct task_struct *t = get_current()->p_cptr;
	if (!t) {
		return 0;
	}
	printk("Got youngest child.\n");
	int res = 1;
	struct task_struct *youngest = t;
	while (t->p_osptr && (t->p_osptr != youngest)) {
		res++;
		t = t->p_osptr;
		printk("Another child.\n");
	}
	return res;
}

