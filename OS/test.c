#include "hw2_syscalls.h"
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define SCHED_SHORT   5
#define SCHED_OTHER		0
#define SCHED_FIFO		1
#define SCHED_RR		2

typedef struct sched_param {
	int sched_priority, requested_time;
} sched_param_t;

int main() {
	//printf("current process is short?\n%d\n", is_short(getpid()));
	//printf("errno = %d\n", errno);
	//sched_setscheduler(getpid(), 5, &shorty);
	//printf("errno = %d\n", errno);
	//printf("current process is short?\n%d\n", is_short(getpid()));
	// pid_t pid = fork();
	// if (pid) {

	// }
	printf( ANSI_COLOR_YELLOW "==================HW2 Test=============" ANSI_COLOR_RESET "\n");
	sched_param_t shorty;

	printf( "check if OTHER proccess is not SHORT \n");
	if (is_short(getpid()) == -1)
	{	
		printf( ANSI_COLOR_GREEN "TEST PASS," ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
	}
	else
	{
		printf( ANSI_COLOR_RED "TEST FAIL," ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
	}

	printf( "check if wrong id proccess is not SHORT \n");
	if (is_short(123) == -1)
	{	
		printf( ANSI_COLOR_GREEN "TEST PASS," ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
	}
	else
	{
		printf( ANSI_COLOR_RED "TEST FAIL," ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
	}

	printf( "check OTHER remaining_time should fail \n");
	if (short_remaining_time(getpid()) == -1)
	{	
		printf( ANSI_COLOR_GREEN "TEST PASS," ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
	}
	else
	{
		printf( ANSI_COLOR_RED "TEST FAIL," ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
	}

	printf( "check wrong id proccess remaining_time should fail \n");
	if (short_remaining_time(123) == -1)
	{	
		printf( ANSI_COLOR_GREEN "TEST PASS," ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
	}
	else
	{
		printf( ANSI_COLOR_RED "TEST FAIL," ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
	}

	shorty.sched_priority = 0;
	shorty.requested_time = 0;

	errno = 0;
	sched_setscheduler(getpid(), SCHED_SHORT, &shorty);
	printf( "make short with requested_time less then 1 should fail. \n");
	if (errno != 22)
	{	
		printf( ANSI_COLOR_RED "TEST FAIL,"  ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
	}
	else
	{
		printf( ANSI_COLOR_GREEN "TEST PASS," ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
	}

	errno = 0;
	shorty.requested_time = 3001;
	sched_setscheduler(getpid(), SCHED_SHORT, &shorty);
	printf( "make short with requested_time more then 3000 should fail. \n");
	if (errno != 22)
	{	
		printf( ANSI_COLOR_RED "TEST FAIL," ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
	}
	else
	{
		printf( ANSI_COLOR_GREEN "TEST PASS," ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
	}

	errno = 0;
	shorty.sched_priority = 50;
	shorty.requested_time = 20;
	sched_setscheduler(getpid(), SCHED_SHORT, &shorty);
	printf( "try to make short. \n");
	if (is_short(getpid()) == 1)
	{	
		printf( ANSI_COLOR_GREEN "TEST PASS," ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
	}
	else
	{
		printf( ANSI_COLOR_RED "TEST FAIL," ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
	}

	errno = 0;
	while ( (shorty.requested_time - short_remaining_time(getpid())) < 2);
	shorty.requested_time = 1;
	sched_setscheduler(getpid(), -1, &shorty);
	printf( "change requested_time to shorter time then already run should fail. \n");
	if (errno != 22)
	{	
		printf( ANSI_COLOR_RED "TEST FAIL," ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
	}
	else
	{
		printf( ANSI_COLOR_GREEN "TEST PASS," ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
	}

	shorty.requested_time = 20;
	int time_left =  short_remaining_time(getpid());
	int flag = 0;
	
	while (time_left > 0) {
		time_left =  short_remaining_time(getpid());
		if(!flag && is_short(getpid()) == 0) {
			printf( "check that process overdue and time doubled. \n");
			int temp_time = short_remaining_time(getpid());
			if (temp_time < 2*shorty.requested_time-1 || temp_time > 2*shorty.requested_time)
			{	
				printf( ANSI_COLOR_RED "TEST FAIL," ANSI_COLOR_CYAN " new time = %d, too far from %d\n" ANSI_COLOR_RESET, temp_time, 2*shorty.requested_time);
			}
			else
			{
				printf( ANSI_COLOR_GREEN "TEST PASS," ANSI_COLOR_CYAN "  new time = %d, close to %d\n" ANSI_COLOR_RESET, temp_time, 2*shorty.requested_time);
			}
			flag = 1;
		}
	}

	printf( "check that overdue process changed to other. \n");
	if (time_left != -1)
	{	
		printf( ANSI_COLOR_RED "TEST FAIL,\n"   ANSI_COLOR_RESET );
	}
	else
	{
		printf( ANSI_COLOR_GREEN "TEST PASS,\n"   ANSI_COLOR_RESET);
	}
	

	errno = 0;
	printf( "check if  proccess is not SHORT anymore \n");
	if (is_short(getpid()) == -1)
	{	
		printf( ANSI_COLOR_GREEN "TEST PASS," ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
	}
	else
	{
		printf( ANSI_COLOR_RED "TEST FAIL," ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
	}

	errno = 0;
	printf( "check if  proccess was SHORT  \n");
	if (was_short(getpid()) == 1)
	{	
		printf( ANSI_COLOR_GREEN "TEST PASS," ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
	}
	else
	{
		printf( ANSI_COLOR_RED "TEST FAIL," ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
	}
	//need to fork and check that son wasn't short
	shorty.sched_priority = 20;
	//sched_setscheduler(getpid(), SCHED_FIFO, &shorty);
	errno = 0;
	//this should be RT process now
	int i_time;
	int son_1 = fork();
	if(son_1 == 0) {

		sched_setscheduler(getpid(), SCHED_SHORT, &shorty);
		i_time = 0; while(i_time<2000) {i_time++;}
		printf( ANSI_COLOR_CYAN "I am the 1 short"  " errno = %d\n" ANSI_COLOR_RESET, errno);
		printf( "check if 1 proccess is SHORT \n");
		if (is_short(getpid()) == 1)
		{	
			printf( ANSI_COLOR_GREEN "TEST PASS," ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
		}
		else
		{
			printf( ANSI_COLOR_RED "TEST FAIL," ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
		}

		printf( "fork first short, remain time to overdur %d\n", short_remaining_time(getpid()));
		int short_son = fork();
		i_time = short_remaining_time(getpid()); while(i_time==short_remaining_time(getpid()));
		if(short_son == 0) {
			printf( "i am short son, remain time to overdur %d\n", short_remaining_time(getpid()));
			while(is_short(getpid())==1);
		i_time = short_remaining_time(getpid()); while(i_time==short_remaining_time(getpid()));
			printf( "i am short son that overdue %d, remain time to other %d\n",is_short(getpid()), short_remaining_time(getpid()));
			while(is_short(getpid())==-1);
		i_time = short_remaining_time(getpid()); while(i_time==short_remaining_time(getpid()));
			printf( "i am 1 short son overdue that become other %d,\n",is_short(getpid()));


		}
		else {
			printf( "i am 1 short after fork %d, remain time to overdur %d\n", short_son, short_remaining_time(getpid()));
			while(is_short(getpid())==1);
		i_time = short_remaining_time(getpid()); while(i_time==short_remaining_time(getpid()));
			printf( "i am 1 short that overdue %d, remain time to other %d\n",is_short(getpid()), short_remaining_time(getpid()));
			while(is_short(getpid())==-1);
		i_time = short_remaining_time(getpid()); while(i_time==short_remaining_time(getpid()));
			printf( "i am 1 short overdue that become other %d,\n",is_short(getpid()));
		}
		exit();
	}
	errno = 0;
	int son_2 = fork();
	if(son_2 == 0) {

		sched_setscheduler(getpid(), SCHED_SHORT, &shorty);
		i_time = short_remaining_time(getpid()); while(i_time==short_remaining_time(getpid()));
		printf( ANSI_COLOR_CYAN "I am the 2 short"  " errno = %d\n" ANSI_COLOR_RESET, errno);
		printf( "check if 2 proccess is SHORT \n");
		if (is_short(getpid()) == 1)
		{	
			printf( ANSI_COLOR_GREEN "TEST PASS," ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
		}
		else
		{
			printf( ANSI_COLOR_RED "TEST FAIL," ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
		}

		exit();
	}
	errno = 0;
	int son_3 = fork();
	if(son_3 == 0) {

		shorty.sched_priority = 0;
		sched_setscheduler(getpid(), SCHED_OTHER, &shorty);
		i_time = 0; while(i_time<2000) {i_time++;}
		printf( ANSI_COLOR_CYAN "I am the 1 OTHER"  " errno = %d\n" ANSI_COLOR_RESET, errno);
		printf( "check if 1 other proccess is not SHORT \n");
		if (is_short(getpid()) == -1)
		{	
			printf( ANSI_COLOR_GREEN "TEST PASS," ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
		}
		else
		{
			printf( ANSI_COLOR_RED "TEST FAIL," ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
		}

		exit();

	}
	errno = 0;
	int son_4 = fork();
	if(son_4 == 0) {

		sched_setscheduler(getpid(), SCHED_SHORT, &shorty);
		i_time = short_remaining_time(getpid()); while(i_time==short_remaining_time(getpid()));
		printf( ANSI_COLOR_CYAN "I am the 3 short"  " errno = %d\n" ANSI_COLOR_RESET, errno);
		printf( "check if 3 proccess is SHORT  \n");
		if (is_short(getpid()) == 1)
		{	
			printf( ANSI_COLOR_GREEN "TEST PASS," ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
		}
		else
		{
			printf( ANSI_COLOR_RED "TEST FAIL," ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
		}

		exit();

	}
	errno = 0;
	int son_5 = fork();
	if(son_5 == 0) {

		sched_setscheduler(getpid(), SCHED_SHORT, &shorty);
		i_time = short_remaining_time(getpid()); while(i_time==short_remaining_time(getpid()));
		printf( ANSI_COLOR_CYAN "I am the 4 short"  " errno = %d\n" ANSI_COLOR_RESET, errno);
		printf( "check if 4 proccess is SHORT  \n");
		if (is_short(getpid()) == 1)
		{	
			printf( ANSI_COLOR_GREEN "TEST PASS," ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
		}
		else
		{
			printf( ANSI_COLOR_RED "TEST FAIL," ANSI_COLOR_CYAN " errno = %d\n" ANSI_COLOR_RESET, errno);
		}

		exit();

	}

		printf( ANSI_COLOR_GREEN "ALL sons ready: 1_%d, 2_%d, 3_%d, 4_%d, other_%d,, time to leave %d\n" ANSI_COLOR_RESET, son_1, son_2, son_4, son_5, son_3, getpid());

 
	return 0;
}
