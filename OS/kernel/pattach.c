/**
 *	pattach.c
 *	OS 234120 HW1
 *	Yoni Yeshanov
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/slab.h>

#include <asm/uaccess.h>

/**
 *
 *	Actual implementation.
 *
 */
int sys_attach_proc (pid_t PID) {
	//TODO: *********** list_entry(iterator, type, list head field name) ***********
	return (int) PID;
}

/**
 *
 *	Iterates over current process children and counts them.
 *
 */
int sys_get_child_process_count() {
	struct task_struct *t = get_current()->p_cptr;
	if (!t) {	// If the process has no children.
		return 0;
	}
	int res = 1;
	struct task_struct *youngest = t;
	while (t->p_osptr && (t->p_osptr != youngest)) {
		res++;
		t = t->p_osptr;
	}
	return res;
}

/**
 *
 *	Returns the number of child processes that were inserted to the @result array.
 *
 */
int sys_get_child_processes(pid_t* result, unsigned int max_length) {
	if (!result) {
		return -EFAULT;
	}
	if (max_length == 0 || !get_current()->p_cptr) {
		return 0;
	}
	int count = sys_get_child_process_count();
	// printk("Child processes count:\t%d\n", count);
	int size;
	if (count < max_length) {
		size = count;
	} else {
		size = max_length;
	}
	// printk("Size [min(max_length, count)]:\t%d\n", size);
	pid_t* child_pids = (pid_t*) kmalloc( sizeof(pid_t) * size, GFP_KERNEL );
	if (!child_pids) {
		return -ENOMEM;
	}
	struct task_struct *t = get_current()->p_cptr;
	int i;
	for (i = 0; i < size; i++) {
		// printk("PID %d:\t%d\n", i, t->pid);
		child_pids[i] = t->pid;
		t = t->p_osptr;
	}
	int num_of_bytes_not_copied = copy_to_user(result, child_pids, size * sizeof(pid_t));
	kfree(child_pids);
	if (num_of_bytes_not_copied != 0) {
		return -EFAULT;
	}
	return size;
}
