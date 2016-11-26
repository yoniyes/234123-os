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
	/**
	 * Check validity.
	 */
	struct task_struct* t = find_task_by_pid(PID);
	if (!t) {	/*	PID doesn't exist	*/
		return -ESRCH;
	}
	if (PID == 1) {	/*	If PID is INIT	*/
		return -EINVAL;
	}
	struct task_struct* iter = get_current();
	while (iter && iter->pid != 1) {
		if (iter->pid == PID) {	/*	If PID is an ancestor of current (or current itself)	*/
			return -EINVAL;
		}
		iter = iter->p_opptr;
	}
	int proc_euid = t->euid, curr_euid = get_current()->euid;
	if (curr_euid != 0 && curr_euid != proc_euid) {	/*	User is not root and not process owner	*/
		return -EPERM;
	}
	if (t->p_opptr->wait_busy == -1 || t->p_opptr->wait_busy == PID) {	/*	The father is waiting for PID [wait() or waitpid(PID)]	*/
		return -EBUSY;
	}
	/**
	 *	Start attaching PID as youngest child of current.
	 */
	int res = t->p_opptr->pid;
	REMOVE_LINKS(t);
	t->p_pptr = current;
	t->p_opptr = current;
	SET_LINKS(t);
	return res;
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
	int size;
	if (count < max_length) {
		size = count;
	} else {
		size = max_length;
	}
	pid_t* child_pids = (pid_t*) kmalloc( sizeof(pid_t) * size, GFP_KERNEL );
	if (!child_pids) {
		return -ENOMEM;
	}
	struct task_struct *t = get_current()->p_cptr;
	int i;
	for (i = 0; i < size; i++) {
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
