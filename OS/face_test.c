/*TODO: really, should try to use RT for management, could prevent unstability*/
/*Supress warning*/
#define _SVID_SOURCE /*WTF? for shm.h*/

#include <time.h>
#include <stdio.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <sys/resource.h>
#include "hw2_syscalls.h"

/*TODO: start ask should we put it somewhere*/
#define SCHED_SHORT 5
struct my_sched_param 
{
	int sched_priority;
	int requested_time;
};
/*TODO: finish ask */

#define DO_SYS(syscall) do { \
		if ( -1 == (syscall))\
		{\
			perror(#syscall);\
			fprintf(stderr, "Occured on line %d\n", __LINE__);\
			exit(1);\
		}\
	} while (0)

#define STUCK_HEURISTIC(sec) { int i; for (i = 0; i < (sec) * 5e8; ++i); /*Could be optimized out better not to use -O*/ } 
#define STUCK(ms) { clock_t t; for (t = clock(); (clock() - t) * 1000 <= ms * CLOCKS_PER_SEC; ); }

#define ASSERT_MESSAGE(condition, message, ...) do {\
		if (!(condition))\
		{\
			fprintf(stderr, "\x1b[31m"  message "\x1b[0m", ##__VA_ARGS__);\
			exit(1);\
		}\
	} while (0)

#define ASSERT(condition) ASSERT_MESSAGE((condition), "Asseration failed \"%s\" on line %d\n", #condition, __LINE__)

#define RUN_TEST(testName) do {\
		fflush(stdout);\
		int exitCode = (testName)();\
		if (0 == exitCode)\
		{\
			printf("\x1b[32m" "%s is passed\n" "\x1b[0m", #testName);\
		}\
		else\
		{\
			printf("\x1b[31m" "%s is failed (exit code = %d)\n" "\x1b[0m", exitCode, #testName);\
		}\
		fflush(stdout);\
		memset(getSharedMemory(0), 0, sizeof(SharedMemory));\
	} while(0)

FILE *debugFile;

#ifdef DEBUG_OUTPUT

#define DEBUG(format, ...) do{\
		printf("Process %d Line %d says: " format "\n", getpid(), __LINE__, ##__VA_ARGS__);\
		fprintf(debugFile, "Process %d Line %d says: " format "\n", getpid(), __LINE__, ##__VA_ARGS__);\
		fflush(debugFile);\
	} while(0)

#define DEBUG_INIT do{\
		FILE *klogdPIDFile = fopen("/var/run/klogd.pid", "r");\
		debugFile = fopen("logWithBlackJackAndHookers.txt", "w");\
		fprintf(debugFile, "Log started\n");\
		fflush(debugFile);\
		if (klogdPIDFile)\
		{\
			long pid;\
			char commandKill[4+1+19+1];\
			fscanf(klogdPIDFile, "%d", &pid);\
			fclose(klogdPIDFile);\
			sprintf(commandKill, "kill %d", pid);\
			DEBUG("%s", commandKill);\
			DO_SYS(system(commandKill));\
		}\
		sleep(1);\
		char commandStart[5+1+2+1+256] = "klogd -f ";\
		DO_SYS((int)getcwd(commandStart + strlen(commandStart), sizeof(commandStart) - strlen(commandStart)));\
		sprintf(commandStart, "%s/kernel.log", commandStart);\
		DEBUG("%s", commandStart);\
		DO_SYS(system(commandStart));\
	}while(0)
#else
#define DEBUG(format, ...) 
#define DEBUG_INIT 
#endif

#define NUMBER_OF_CHILDREN 18

/*Suppress warning*/
int nice(int); /*TODO: why the hell it is not defined in unistd.h?*/

/*
If you see 
sharedMemoryIdentifier = shmget(key, sizeof(SharedMemory), (create ? IPC_CREAT : 0) | 0666): Invalid argument
please change it
If you can explain why the hell it happens, please tell me
*/
#define PATH_NAME "/tmp"
#define PROJECT_ID (42 + NUMBER_OF_CHILDREN)
key_t key; // TODO: why can't use ftok somehow


typedef struct {
	int startedBeingStuck[NUMBER_OF_CHILDREN], finishedBeingStuck[NUMBER_OF_CHILDREN];
	int numberOfShortProcesses;
	time_t childStartTime;
	time_t childFinishTime;
	time_t fatherStartTime;
	time_t fatherFinishTime;
} SharedMemory;

static SharedMemory* getSharedMemory(int create)
{
	//  key_t key;
	//  DO_SYS((int)(key == ftok(PATH_NAME, PROJECT_ID)));
	//  DEBUG("%d", key);

	int sharedMemoryIdentifier;
	DEBUG("create=%d, key=%x, sizeof(key)=%u", create, key, sizeof(key));
	DO_SYS(sharedMemoryIdentifier = shmget(key, sizeof(SharedMemory), (create ? IPC_CREAT : 0) | 0666));

	void *result;
	DO_SYS((int)(result = shmat(sharedMemoryIdentifier, NULL, 0)));

	if (create)
	{
		memset(result, 0, sizeof(SharedMemory));
	}

	return (SharedMemory*)result;
}

#define MAKE_SHORT(pid, time) do {\
		struct my_sched_param schedParams = { \
			.sched_priority = 42/*Todo ask must we check or must we not check: -42*/,\
			.requested_time = (time),\
		};\
		DO_SYS(sched_setscheduler((pid), SCHED_SHORT, (struct sched_param*)&schedParams));            \
	} while (0)

static int allChildrenAreNice(pid_t children[])
{
	for (int childIndex = 0; childIndex < NUMBER_OF_CHILDREN; ++childIndex)
	{
		if (getpriority(PRIO_PROCESS, children[childIndex]) == 0)
		{
			return 0;
		}
	}
	return 1;
}

static int becameForkBecameShortAndMakeChildrenShortToCheckOrder(int changeNice)
{
	const int requestedTime = 3000;
	const int runTime = requestedTime >> 1;

	pid_t child;
	int returnValue;

	DO_SYS(child = fork());

	ASSERT(-1 == is_short(getpid()));
	if (0 == child)
	{
		pid_t children[NUMBER_OF_CHILDREN];
		
		ASSERT_MESSAGE(NUMBER_OF_CHILDREN < 20, "Not so generic, give me less than 20 children"); /*TODO make cmpile time*/

		for (int childIndex = 0; childIndex < NUMBER_OF_CHILDREN; ++childIndex)
		{
			ASSERT(-1 == is_short(getpid()));	  
			DO_SYS(children[childIndex] = fork());

			if (0 == children[childIndex])
			{	  
				DEBUG("%d was born in a hurricane", getpid());

				if (changeNice)
				{
					DEBUG("before nice to %d", NUMBER_OF_CHILDREN - childIndex);
					DO_SYS(nice(NUMBER_OF_CHILDREN - childIndex));
					DEBUG("did nice: %d", getpriority(PRIO_PROCESS, getpid()));
				}
				else
				{
					DEBUG("more 'nice' that father by %d", 1);
					DO_SYS(nice(1));
					DEBUG("did nice: %d", getpriority(PRIO_PROCESS, getpid()));
				}

				SharedMemory *memory = getSharedMemory(0);	      
				while (NUMBER_OF_CHILDREN != memory->numberOfShortProcesses)
				{
					sched_yield();
				}
				
				pid_t pid = getpid();

				DEBUG("nosp: %d", memory->numberOfShortProcesses);
				ASSERT_MESSAGE(memory->numberOfShortProcesses == NUMBER_OF_CHILDREN, 
				"Seems like I have not yielded CPU while becaming less nice or interruped my father with highter priority");
				ASSERT(is_short(pid));
				if (changeNice)
				{
					ASSERT_MESSAGE(childIndex + 1 == NUMBER_OF_CHILDREN || memory->finishedBeingStuck[childIndex+1], 
					"started before more prioritirised short finished");
					ASSERT_MESSAGE(childIndex == 0 || !memory->startedBeingStuck[childIndex-1], 
					"less priored started before me");
				}
				else
				{
					ASSERT_MESSAGE(childIndex == 0 || memory->finishedBeingStuck[childIndex-1],
					"While becoming short pid %d should have entered before me but have not\n", children[childIndex-1]);		     
					ASSERT_MESSAGE(childIndex + 1 == NUMBER_OF_CHILDREN || !memory->startedBeingStuck[childIndex+1],
					"While becoming short should have entered after me but have not\n");		     
				}

				
				DEBUG("personally I have got %d ms", short_remaining_time(getpid()));
				ASSERT(is_short(getpid()));
				ASSERT(-1 == was_short(getpid()) && EINVAL == errno);
				
				DEBUG("set start");
				memory->startedBeingStuck[childIndex] = 1;
				STUCK(runTime);
				memory->finishedBeingStuck[childIndex] = 1;
				DEBUG("finished");
				ASSERT(1 == is_short(getpid()));
				ASSERT(-1 == was_short(getpid()) && EINVAL == errno);
				DEBUG("I've got another %d ms, but gonna die for my government", short_remaining_time(getpid()));
				exit(0);
			}
			
			ASSERT(-1 == is_short(children[childIndex]));
			DEBUG("I love my child: %d, he has %d, I've got %d", children[childIndex], short_remaining_time(children[childIndex]), short_remaining_time(getpid()));
		}

		ASSERT(-1 == is_short(getpid()));
		
		DEBUG("Going to wait for bastards to do nice");
		while(!allChildrenAreNice(children))
		{
			sched_yield();
		}
		DEBUG("Everyone is nice");
		
		MAKE_SHORT(getpid(), 3000);
		DEBUG("My time after forks: %d ms", short_remaining_time(getpid()));
		SharedMemory *memory = getSharedMemory(0);
		DEBUG("Making my bastards short");
		for (int childIndex = 0; childIndex < NUMBER_OF_CHILDREN; ++childIndex)
		{     
			MAKE_SHORT(children[childIndex], requestedTime);
			ASSERT(is_short(children[childIndex]));
			DEBUG("Made %d-th child %d short, got %d ms", childIndex, children[childIndex], short_remaining_time(getpid()));
			++memory->numberOfShortProcesses;
		}
		DEBUG("All my bastards are short, I am is_short: %d", is_short(getpid()));
		ASSERT_MESSAGE(1 == is_short(getpid()), "Father is not short, could cause other problems");

		for (int childIndex = 0; childIndex < NUMBER_OF_CHILDREN; ++childIndex)
		{
			int returnValue;
			
			DEBUG("So am I, still waiting, for this world to stop hating");
			ASSERT(waitpid(children[childIndex], &returnValue, 0));
			DEBUG("finished wait for child %d: exit code=%d", children[childIndex], WEXITSTATUS(returnValue));
			ASSERT(0 == WEXITSTATUS(returnValue));

			ASSERT(memory->finishedBeingStuck[childIndex]);
		}

		exit (0);
	}

	DO_SYS(waitpid(child, &returnValue, 0));
	ASSERT(0 == WEXITSTATUS(returnValue));

	return 0;
}

int testToCheckDifferentPriorities()
{
	return becameForkBecameShortAndMakeChildrenShortToCheckOrder(1);
}

int testToCheckFifoOrderOfTheSamePriorities()
{
	return becameForkBecameShortAndMakeChildrenShortToCheckOrder(0);
}

#define MS_EPS 3

int testToCheckThatSonStartsBefore()
{
	const int requestedTime = 3000;
	pid_t child;

	DO_SYS(child = fork());

	if (0 == child)
	{
		DEBUG("going to be short");
		MAKE_SHORT(getpid(), requestedTime);
		pid_t child;
		
		DEBUG("going to fork");
		DO_SYS(child = fork());
		
		time_t startTime = time(NULL); /*bad bad bad*/
		STUCK(requestedTime / 3);

		SharedMemory *memory = getSharedMemory(0);
		
		if (child)
		{
			int returnValue;
			ASSERT_MESSAGE(memory->childStartTime, "Seems like my son has not started yet!");
			ASSERT_MESSAGE(memory->childFinishTime, "Seems like my son has not finished yet!");
			memory->fatherStartTime = startTime;
			memory->fatherFinishTime = time(NULL);

			DO_SYS(waitpid(child, &returnValue, 0));		
			ASSERT(0 == WEXITSTATUS(returnValue));  
			DEBUG("My child is dead, see no point in living");
		}
		else
		{
			ASSERT_MESSAGE(MS_EPS > abs(short_remaining_time(getppid() - short_remaining_time(getpid()))), "Some problem with remaining time");
			ASSERT_MESSAGE(!memory->fatherStartTime, "Seems like my father already started!");
			ASSERT_MESSAGE(!memory->fatherFinishTime, "Seems like my father already finished!");
			memory->childStartTime = startTime;
			memory->childFinishTime = time(NULL);
			DEBUG("Suicide");
		}
		exit(0);
	}

	int returnValue;
	DO_SYS(waitpid(child, &returnValue, 0));
	ASSERT(0 == WEXITSTATUS(returnValue));  

	SharedMemory *memory = getSharedMemory(0); 
	ASSERT(memory->childFinishTime && memory->fatherStartTime);

	return 0;
}

int forkBecameShortMakeShortOverdueAndCheckOrder()
{
	const int minShortOverdueRemainingTime = 10;
	const int requestedTime = minShortOverdueRemainingTime / 2 + 1 + 1500;

	pid_t child;

	DO_SYS(child = fork());

	if (0 == child)
	{
		pid_t children[NUMBER_OF_CHILDREN];
		for (int childIndex = 0; childIndex < NUMBER_OF_CHILDREN; ++childIndex)
		{
			DO_SYS(children[childIndex] = fork());

			if (0 == children[childIndex])
			{
				DO_SYS(nice(1));
				while (-1 == is_short(getpid())) 
				{
					sched_yield();
				}
				
				ASSERT(is_short(getpid()));
				SharedMemory *memory = getSharedMemory(0);
				DEBUG("Want to be overdue");
				while(1 == is_short(getpid()));
				DEBUG("I did it! I am overdue");
				memory->startedBeingStuck[childIndex] = time(NULL);
				ASSERT_MESSAGE(childIndex == 0 || memory->finishedBeingStuck[childIndex-1], "The guy %d in fifo before me is not done yet", children[childIndex-1]);
				STUCK(short_remaining_time(getpid()) > minShortOverdueRemainingTime);
				ASSERT_MESSAGE(childIndex+1 == NUMBER_OF_CHILDREN || !memory->startedBeingStuck[childIndex+1], 
				"That bitch in fifo after me started before I was finished");
				memory->finishedBeingStuck[childIndex] = time(NULL);
				exit(0);
			}
		}

		MAKE_SHORT(getpid(), 3000);
		for (int childIndex = 0; childIndex < NUMBER_OF_CHILDREN; ++childIndex)
		{
			MAKE_SHORT(children[childIndex], requestedTime);
		}

		for (int childIndex = 0; childIndex < NUMBER_OF_CHILDREN; ++childIndex)
		{
			int returnValue, childWasShort, childIsShort;	  
			waitpid(children[childIndex], &returnValue, 0);
			ASSERT(0 == WEXITSTATUS(returnValue));
		}
		
		exit(0);
	}

	int returnValue;
	DO_SYS(waitpid(child, &returnValue, 0));
	ASSERT(0 == WEXITSTATUS(returnValue));  

	return 0;
}

int testToCheckTimeAfterForkOfShort()
{
	pid_t child;
	int requestedTime = 1234;

	DO_SYS(child = fork());

	if (0 == child)
	{
		MAKE_SHORT(getpid(), requestedTime);
		int lastFatherTime;
		DO_SYS(lastFatherTime = short_remaining_time(getpid()));
		DEBUG("%d", lastFatherTime);
		ASSERT(lastFatherTime >= 0 && lastFatherTime >= requestedTime - MS_EPS);
		
		DO_SYS(child = fork());

		int myRemainingTime = short_remaining_time(getpid());
		ASSERT(abs(myRemainingTime - lastFatherTime / 2) < MS_EPS);
		ASSERT(SCHED_SHORT == sched_getscheduler(0));
		struct my_sched_param param;
		DO_SYS(sched_getparam(0, (struct sched_param*)&param));
		ASSERT(requestedTime == param.requested_time);
		
		if (0 != child)
		{
			int returnValue;
			waitpid(child, &returnValue, 0);
			ASSERT(0 == WEXITSTATUS(returnValue));
		}

		exit(0);
	}

	int returnValue;
	DO_SYS(waitpid(child, &returnValue, 0));
	ASSERT(0 == WEXITSTATUS(returnValue));  

	return 0;
}

int testToCheckNewSysCalls()
{
	pid_t child;

	DO_SYS(child = fork());

	if (0 == child)
	{
		const int runTime = 20;
		const int requestedTime = runTime * 2 + MS_EPS;

		struct my_sched_param param;

		ASSERT(-1 == is_short(getpid()) && EINVAL == errno);
		ASSERT(-1 == short_remaining_time(getpid()) && EINVAL == errno);
		ASSERT(0 == was_short(getpid()));
		ASSERT(0 == sched_getparam(getpid(), (struct sched_param*)&param)); 
		param.requested_time = 3001;
		ASSERT(-1 == sched_setscheduler(0, SCHED_SHORT, (struct sched_param*)&param) && EINVAL == errno);
		param.requested_time = 0;
		ASSERT(-1 == sched_setscheduler(0, SCHED_SHORT, (struct sched_param*)&param) && EINVAL == errno);
		param.requested_time = -1;
		ASSERT(-1 == sched_setscheduler(0, SCHED_SHORT, (struct sched_param*)&param) && EINVAL == errno);    

		MAKE_SHORT(getpid(), requestedTime);
		
		ASSERT_MESSAGE(abs(requestedTime - short_remaining_time(getpid())) < MS_EPS, 
		"%d ms were given, but just %d left. Probably conversion problem", requestedTime, short_remaining_time(getpid()));
		ASSERT(1 == is_short(getpid()));
		ASSERT(-1 == was_short(getpid()) && EINVAL == errno);
		ASSERT(0 == sched_getparam(getpid(), (struct sched_param*)&param)); 
		ASSERT(requestedTime == param.requested_time);
		param.sched_priority = 0;
		ASSERT(-1 == sched_setscheduler(0, SCHED_OTHER, (struct sched_param*)&param) && errno == EPERM);
		param.requested_time = 3001;
		ASSERT(-1 == sched_setparam(0, (struct sched_param*)&param) && EINVAL == errno);
		ASSERT(-1 == sched_setscheduler(0, SCHED_SHORT, (struct sched_param*)&param) && EINVAL == errno);
		param.requested_time = 0;
		ASSERT(-1 == sched_setparam(0, (struct sched_param*)&param) && EINVAL == errno);
		ASSERT(-1 == sched_setscheduler(0, SCHED_SHORT, (struct sched_param*)&param) && EINVAL == errno);
		param.requested_time = -1;
		ASSERT(-1 == sched_setparam(0, (struct sched_param*)&param) && EINVAL == errno);
		ASSERT(-1 == sched_setscheduler(0, SCHED_SHORT, (struct sched_param*)&param) && EINVAL == errno);
		
		while (requestedTime - short_remaining_time(getpid()) < runTime + MS_EPS);
		param.requested_time = runTime - 1;
		ASSERT_MESSAGE(-1 == sched_setparam(0, (struct sched_param*)&param) && EINVAL == errno, 
		"It is forbidden to decrease the requested time to a value smaller than the actual time the process has already ran");
		ASSERT_MESSAGE(-1 == sched_setscheduler(0, SCHED_SHORT, (struct sched_param*)&param) && EINVAL == errno, 
		"It is forbidden to decrease the requested time to a value smaller than the actual time the process has already ran");     

		while(1 == is_short(getpid()));

		ASSERT(abs(2 * requestedTime - short_remaining_time(getpid())) < MS_EPS);
		ASSERT(0 == is_short(getpid()));
		ASSERT(-1 == was_short(getpid()) && EINVAL == errno);
		ASSERT(0 == sched_getparam(getpid(), (struct sched_param*)&param)); 
		ASSERT(requestedTime == param.requested_time);
		while (0 == is_short(getpid()));

		ASSERT(-1 == is_short(getpid()) && EINVAL == errno);
		ASSERT(-1 == short_remaining_time(getpid()) && EINVAL == errno);
		ASSERT(1 == was_short(getpid()));
		
		exit(0);
	}

	int returnValue;
	DO_SYS(waitpid(child, &returnValue, 0));
	ASSERT(0 == WEXITSTATUS(returnValue));    

	return 0;
}

int testToCheckMorePriotirisedShortWakeUp()
{
	pid_t child;

	DO_SYS(child = fork());

	if (0 == child)
	{
		const int requestedTime = 3000;

		pid_t child;

		DO_SYS(child = fork());
		
		if (0 == child)
		{
			MAKE_SHORT(getpid(), requestedTime);
			DEBUG("I will sleep a little bit, but afterwards I will wake up and to interrupt my child");
			int timeBeforeSleep = short_remaining_time(getpid());
			sleep((requestedTime/1000) >> 1);
			ASSERT(abs(timeBeforeSleep - short_remaining_time(getpid())) < MS_EPS);

			SharedMemory *memory = getSharedMemory(0);
			ASSERT(memory->fatherStartTime);
			ASSERT(!memory->fatherFinishTime);
			memory->childFinishTime = time(NULL);
			exit(0);
		}
		DO_SYS(nice(1));
		MAKE_SHORT(getpid(), requestedTime);
		SharedMemory *memory = getSharedMemory(0);
		memory->fatherStartTime = time(NULL);
		ASSERT(!memory->childFinishTime);
		DEBUG("Going to wait for child to wake up");
		while (!memory->childFinishTime);
		ASSERT(1 == is_short(getpid()));
		DEBUG("Child woken up and interrupted me");
		memory->fatherFinishTime = time(NULL);
		exit(0);
	}

	int returnValue;
	DO_SYS(waitpid(child, &returnValue, 0));
	ASSERT(0 == WEXITSTATUS(returnValue)); 
	
	return 0;
}

int main(int argc, char *argv[])
{  
	DEBUG_INIT;

	DO_SYS(key = ftok(PATH_NAME, PROJECT_ID));
	getSharedMemory(1);

	RUN_TEST(testToCheckNewSysCalls);
	RUN_TEST(testToCheckDifferentPriorities);
	RUN_TEST(testToCheckFifoOrderOfTheSamePriorities);
	RUN_TEST(testToCheckThatSonStartsBefore);
	RUN_TEST(testToCheckTimeAfterForkOfShort);
	RUN_TEST(forkBecameShortMakeShortOverdueAndCheckOrder);
	RUN_TEST(testToCheckMorePriotirisedShortWakeUp);

	return 0;
}
