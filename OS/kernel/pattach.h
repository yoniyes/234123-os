/**
 *	pattach.h
 *	OS 234120 HW1
 *	Yoni Yeshanov
 */

#ifndef _PATTACH_H_
#define _PATTACH_H_


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
int attach_proc (pid_t PID) {
	unsigned int res;	// Should it be unsigned int?
	__asm__(
		"int $0x80;"
		: "=a" (res)									// Output operands.
		: "0" (243) ,"b" (PID)							// Input operands.
		: "memory"										// Clobber list.
	);
	// TODO: Check @res value and return.
	if (res >= (unsigned long)(-125))
	{
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
int get_child_processes(pid_t* result, unsigned int max_length) {
	unsigned int res;
	__asm__(
		"int $0x80;"
		: "=a" (res)
		: "0" (244) ,"b" (result) ,"c" (max_length)
		: "memory"
	);
	// TODO: Check @res value & @result and return.
	if (res >= (unsigned long)(-125))
	{
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
int get_child_process_count() {
	unsigned int res;
	__asm__(
		"int $0x80;"
		: "=a" (res)
		: "0" (245)
		: "memory"
	);
	return (int) res;
}

#endif