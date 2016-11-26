#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/errno.h>

#include <asm/uaccess.h>

int sys_is_short(pid_t pid) {
	struct task_struct *t = find_task_by_pid(pid);
	if (!t) {
		return -ESRCH;
	}
	if (t->policy != SCHED_SHORT) {
		return -EINVAL;
	} else if (t->is_short == -1) {
			return 0;
	}
	return 1;
}

int sys_short_remaining_time(pid_t pid) {
	struct task_struct *t = find_task_by_pid(pid);
	if (!t) {
		return -ESRCH;
	}
	if (t->policy != SCHED_SHORT) {
		return -EINVAL;
	}
	return t->time_slice * 1000 / HZ;
}

int sys_was_short(pid_t pid) {
	struct task_struct *t = find_task_by_pid(pid);
	if (!t) {
		return -ESRCH;
	}
	if (t->policy == SCHED_SHORT) {
		return -EINVAL;
	}
	return -t->is_short;
}
