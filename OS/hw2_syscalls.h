#ifndef _HW2_SYSCALLS_H_
#define _HW2_SYSCALLS_H_

#include <sys/types.h>
#include <errno.h>

/**	WRAPPER TEMPLATE
int my_system_call (int p1, char *p2,int p3) {
	unsigned int res;
	__asm__(
		"int $0x80;"
		: "=a" (res)
		: "0" (244) ,"b" (p1) ,"c" (p2), "d" (p3)
		: "memory"
	);
	if (res >= (unsigned long)(-125))
	{
		errno = -res;
		res = -1;
	}
	return (int) res;
}
*/

/**
 *
 *	Force process PID ​to become the youngest child process of the current process.
 *
 */
int is_short (pid_t pid) {
	unsigned int res;	// Should it be unsigned int?
	__asm__(
		"int $0x80;"
		: "=a" (res)									// Output operands.
		: "0" (243) ,"b" (pid)							// Input operands.
		: "memory"										// Clobber list.
	);
	if (res >= (unsigned long)(-125)) {
		errno = -res;
		res = -1;
	}
	return (int) res;
}

/**
 *
 *	Scans and collects the PIDs of the child processes of the current process from the youngest
 *	child to the oldest, stopping at max_length or the oldest child, whichever comes first.
 *
 */
int short_remaining_time(pid_t pid) {
	unsigned int res;
	__asm__(
		"int $0x80;"
		: "=a" (res)
		: "0" (244) ,"b" (pid)
		: "memory"
	);
	if (res >= (unsigned long)(-125)) {
		errno = -res;
		res = -1;
	}
	return (int) res;
}

/**
 *
 *	Calculates the number of direct child processes that the current process has.
 *
 */
int was_short(pid_t pid) {
	unsigned int res;
	__asm__(
		"int $0x80;"
		: "=a" (res)
		: "0" (245), "b" (pid)
		: "memory"
	);
	if (res >= (unsigned long)(-125)) {
		errno = -res;
		res = -1;
	}
	return (int) res;
}

#endif
