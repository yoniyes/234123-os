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
#define PROJECT_ID (42 + NUMBER_OF_CHILDREN + 13)
key_t key; // TODO: why can't use ftok somehow


typedef struct {
  int startedBeingStuck[NUMBER_OF_CHILDREN], finishedBeingStuck[NUMBER_OF_CHILDREN];
  int fatherRemainingTimeBeforeFork[NUMBER_OF_CHILDREN];
  int numberOfShortProcesses;
  time_t childStartTime;
  time_t childFinishTime;
  time_t fatherStartTime;
  time_t fatherFinishTime;
  size_t queueFinishSize; /*Try to update it atomic*/
  size_t queueStartSize; /*Try to update it atomic*/
  pid_t finishQueue[3]; /*Warning: hardcoded*/
  pid_t startQueue[3]; /*Warning: hardcoded*/
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
	.sched_priority = 0/*Todo ask must we check or must we not check: -42*/,\
	.requested_time = (time),\
      };\
      DO_SYS(sched_setscheduler((pid), SCHED_SHORT, (struct sched_param*)&schedParams));            \
} while (0)

#define MAKE_RR(pid) do {\
      struct my_sched_param schedParams = { \
	.sched_priority = 50,\
	.requested_time = 42,\
      };\
      DO_SYS(sched_setscheduler((pid), SCHED_RR, (struct sched_param*)&schedParams));            \
} while (0)

#define MAKE_OTHER(pid) do {\
      struct my_sched_param schedParams = { \
	.sched_priority = 0,\
	.requested_time = 42,\
      };\
      DO_SYS(sched_setscheduler((pid), SCHED_OTHER, (struct sched_param*)&schedParams));            \
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
      
      MAKE_RR(getpid());
      DEBUG("Became rr");
      
      for (int childIndex = 0; childIndex < NUMBER_OF_CHILDREN; ++childIndex)
	{
	  ASSERT(-1 == is_short(getpid()));	  
	  DO_SYS(children[childIndex] = fork());

	  if (0 == children[childIndex])
	    {	  
	      DEBUG("%d was born in a hurricane", getpid());

	      DEBUG("Waiting to become short");
	      while (-1 == is_short(getpid()));
	      ASSERT(1 == is_short(getpid()));
	      DEBUG("All right I am short");

	      SharedMemory *memory = getSharedMemory(0);	      

	      pid_t pid = getpid();

	      DEBUG("nosp: %d", memory->numberOfShortProcesses);
	      ASSERT_MESSAGE(NUMBER_OF_CHILDREN == memory->numberOfShortProcesses, "Seems like short interruped rr father");
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
            
      SharedMemory *memory = getSharedMemory(0);
      DEBUG("Making my bastards short");
      for (int childIndex = 0; childIndex < NUMBER_OF_CHILDREN; ++childIndex)
	{     
	  MAKE_OTHER(children[childIndex]);
	  MAKE_SHORT(children[childIndex], requestedTime);
	  if (changeNice)
	    {
	      DO_SYS(setpriority(PRIO_PROCESS, children[childIndex], NUMBER_OF_CHILDREN - childIndex));
	    }
	  ASSERT(1 == is_short(children[childIndex]));
	  DEBUG("Made %d-th child %d short, got %d ms", childIndex, children[childIndex], short_remaining_time(getpid()));
	  ++memory->numberOfShortProcesses;
	}
      DEBUG("All my bastards are short, I am is_short: %d", is_short(getpid()));

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
      DEBUG("Going to be RR");
      MAKE_RR(getpid());
      DEBUG("Became RR");
      for (int childIndex = 0; childIndex < NUMBER_OF_CHILDREN; ++childIndex)
	{
	  DEBUG("Going to give birth");
	  DO_SYS(children[childIndex] = fork());

	  if (0 == children[childIndex])
	    {
	      DEBUG("Going to wait to become short");
	      while (-1 == is_short(getpid()));
	      DEBUG("Became short");
	  
	      ASSERT(1 == is_short(getpid()));
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
	  DEBUG("Did birth");
	}

      DEBUG("GOING TO MAKE BASTARDS SHORT");
      for (int childIndex = 0; childIndex < NUMBER_OF_CHILDREN; ++childIndex)
	{
	  DEBUG("Making %d short", children[childIndex]);
	  MAKE_OTHER(children[childIndex]);
	  MAKE_SHORT(children[childIndex], requestedTime);
	  DEBUG("Makde %d short", children[childIndex]);
	}

      DEBUG("Going to wait for bastards");
      for (int childIndex = 0; childIndex < NUMBER_OF_CHILDREN; ++childIndex)
	{
	  int returnValue, childWasShort, childIsShort;	  
	  DEBUG("Going to wait for %d", children[childIndex]);
	  waitpid(children[childIndex], &returnValue, 0);
	  DEBUG("Returned");
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
      ASSERT_MESSAGE(abs(myRemainingTime - lastFatherTime / 2) <= MS_EPS, "My remaining time should be about %d and not %d\n", lastFatherTime/2, myRemainingTime);
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
      
      ASSERT_MESSAGE(abs(requestedTime - short_remaining_time(getpid())) <= MS_EPS, 
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
      
      while (requestedTime - short_remaining_time(getpid()) <= runTime + MS_EPS);
      param.requested_time = runTime - 1;
      ASSERT_MESSAGE(-1 == sched_setparam(0, (struct sched_param*)&param) && EINVAL == errno, 
		"It is forbidden to decrease the requested time to a value smaller than the actual time the process has already ran");
      ASSERT_MESSAGE(-1 == sched_setscheduler(0, SCHED_SHORT, (struct sched_param*)&param) && EINVAL == errno, 
		"It is forbidden to decrease the requested time to a value smaller than the actual time the process has already ran");     

      while(1 == is_short(getpid()));

      ASSERT_MESSAGE(abs(2 * requestedTime - short_remaining_time(getpid())) <= MS_EPS, "Problem with remaining time for overdue");
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
	  ASSERT(abs(timeBeforeSleep - short_remaining_time(getpid())) <= MS_EPS);

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

int testToCheckOverdueForkOrderAndTime()
{
  pid_t child;
  
  DO_SYS(child = fork());

  if (0 == child)
    {
      const int requestedTime = 2000;
      pid_t children[NUMBER_OF_CHILDREN];

      MAKE_SHORT(getpid(), requestedTime);
      
      while (1 == is_short(getpid()));

      SharedMemory *memory = getSharedMemory(0);
      for (int childIndex = 0; childIndex < NUMBER_OF_CHILDREN; ++childIndex)
	{
	  ASSERT_MESSAGE(0 == is_short(getpid()), "Father is not overdue");
	  memory->fatherRemainingTimeBeforeFork[childIndex] = short_remaining_time(getpid());
	  children[childIndex] = fork();

	  if (0 == children[childIndex])
	    {	
	      SharedMemory *memory = getSharedMemory(0);
	      ASSERT_MESSAGE(0 == is_short(getpid()), "Son of overdue should be overdue");
	      ASSERT_MESSAGE(NUMBER_OF_CHILDREN == memory->numberOfShortProcesses, "Overdue son interruped father");
	      ASSERT_MESSAGE(MS_EPS >= abs(memory->fatherRemainingTimeBeforeFork[childIndex] - short_remaining_time(getpid())),
			     "Child should receive the same remaining time as father");
	      memory->startedBeingStuck[childIndex] = time(NULL);

	      ASSERT_MESSAGE(0 == childIndex || memory->finishedBeingStuck[childIndex-1], "Previous overdued should be done already");
	      ASSERT_MESSAGE(NUMBER_OF_CHILDREN == childIndex+1 || !memory->finishedBeingStuck[childIndex+1], 
			     "Next overdued should be done already");
	      
	      while (short_remaining_time(getpid()) > MS_EPS);

	      memory->finishedBeingStuck[childIndex] = time(NULL);

	      exit(0);
	    }
	  
	  ++memory->numberOfShortProcesses;
	}

      for (int childIndex = NUMBER_OF_CHILDREN-1; childIndex >= 0; --childIndex)
	{
	  int returnValue;
	  DO_SYS(waitpid(children[childIndex], &returnValue, 0));
	  ASSERT(0 == WEXITSTATUS(returnValue));
	}

      for (int childIndex = 0; childIndex < NUMBER_OF_CHILDREN; ++childIndex)
	{
	  ASSERT(0 == childIndex || 0 <= difftime(memory->finishedBeingStuck[childIndex-1], memory->startedBeingStuck[childIndex]));
	  ASSERT(NUMBER_OF_CHILDREN == 1+childIndex || 0 <= difftime(memory->startedBeingStuck[childIndex+1], memory->finishedBeingStuck[childIndex]));
	}

      
      exit(0);
    }
  
  
  int returnValue;
  DO_SYS(waitpid(child, &returnValue, 0));
  ASSERT(0 == WEXITSTATUS(returnValue));   
  return 0;
}

typedef enum {OTHER, SHORT_REGULAR, SHORT_OVERDUE, NUMBER_OF_TYPES} ProcessType;

pid_t makeProcess(ProcessType processType, int prio, int requestedTime, int(*functionToRun)(SharedMemory*))
{
  enum {READ = 0, WRITE = 1};
  const char SUCCESS = 0;

  ASSERT(0 <= processType && processType < NUMBER_OF_TYPES);
  int fileDescriptor[2];
  DO_SYS(pipe(fileDescriptor));  
  
  pid_t child;
  DO_SYS(child = fork());
  
  if (0 == child)
    {
      DO_SYS(close(fileDescriptor[READ]));
      SharedMemory *memory = getSharedMemory(0);
      while (SHORT_REGULAR == processType && 1 != is_short(getpid()));
      while (SHORT_OVERDUE == processType && 0 != is_short(getpid()));
      while (OTHER == processType && SCHED_OTHER != sched_getscheduler(getpid()));

      DO_SYS(write(fileDescriptor[WRITE], &SUCCESS, sizeof(SUCCESS)));
      
      exit(functionToRun(memory));
    }
  
  switch(processType)
    {
    case OTHER:
      MAKE_OTHER(child);
      break;
    case SHORT_REGULAR:
    case SHORT_OVERDUE:
      MAKE_OTHER(child);
      MAKE_SHORT(child, requestedTime);
      break;
    }

  setpriority(PRIO_PROCESS, child, prio);

  char returnValue;
  DO_SYS(close(fileDescriptor[WRITE]));
  DO_SYS(read(fileDescriptor[READ], &returnValue, sizeof(returnValue)));
  ASSERT(SUCCESS == returnValue);

  return child; 
}

int markInQueueAndStuck(SharedMemory *memory)
{
  size_t startPosition = memory->queueStartSize++; /*Need lock or better atomic*/
  memory->startQueue[startPosition] = getpid();

  STUCK(1000);

  size_t finishPosition = memory->queueFinishSize++; /*Need lock or better atomic*/
  memory->finishQueue[finishPosition] = getpid();

  return 0;
}

int testToCheckOrderBetweenOtherShortAndOverdue()
{
  pid_t child;

  DO_SYS(child = fork());

  if (0 == child)
    {
      MAKE_RR(getpid());

      pid_t shortOverdueChild = makeProcess(SHORT_OVERDUE, 0, 2000, markInQueueAndStuck);
      pid_t shortNotOverdueChild = makeProcess(SHORT_REGULAR, 0, 2000, markInQueueAndStuck);
      pid_t shortOtherChild = makeProcess(OTHER, 0, 0, markInQueueAndStuck);

      int returnValue;
      DO_SYS(waitpid(shortNotOverdueChild, &returnValue, 0));
      ASSERT(0 == WEXITSTATUS(returnValue));      
      DO_SYS(waitpid(shortOtherChild, &returnValue, 0));
      ASSERT(0 == WEXITSTATUS(returnValue));
      DO_SYS(waitpid(shortOverdueChild, &returnValue, 0));
      ASSERT(0 == WEXITSTATUS(returnValue));

      SharedMemory *memory = getSharedMemory(0);

      ASSERT_MESSAGE(3 == memory->queueFinishSize, "Some process has not marked himself or sync problem");
      ASSERT_MESSAGE(shortNotOverdueChild == memory->finishQueue[0], "Short has not finished first");
      ASSERT_MESSAGE(shortOtherChild == memory->finishQueue[1], "Other has not finished second");
      ASSERT_MESSAGE(shortOverdueChild == memory->finishQueue[2], "Overdue has not finished third");

      ASSERT_MESSAGE(3 == memory->queueStartSize, "Some process has not marked himself or sync problem");
      DEBUG("ShortNotOverdue: %d\tShortOther: %d\tShortOverdue: %d", shortNotOverdueChild, shortOtherChild, shortNotOverdueChild);
      DEBUG("1:%d\t2:%d\t3:%d", memory->startQueue[0], memory->startQueue[1], memory->startQueue[2]);
      ASSERT_MESSAGE(shortNotOverdueChild == memory->startQueue[0], "Short has not started first");
      ASSERT_MESSAGE(shortOtherChild == memory->startQueue[1], "Other has not started second");
      ASSERT_MESSAGE(shortOverdueChild == memory->startQueue[2], "Overdue has not started third");

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

#ifdef FINAL
  RUN_TEST(testToCheckOrderBetweenOtherShortAndOverdue);
#endif

  RUN_TEST(testToCheckNewSysCalls);
  RUN_TEST(testToCheckDifferentPriorities);
  RUN_TEST(testToCheckFifoOrderOfTheSamePriorities);
  RUN_TEST(testToCheckThatSonStartsBefore);
  RUN_TEST(testToCheckTimeAfterForkOfShort);
  RUN_TEST(forkBecameShortMakeShortOverdueAndCheckOrder);
  RUN_TEST(testToCheckMorePriotirisedShortWakeUp);
  RUN_TEST(testToCheckOverdueForkOrderAndTime);

 
  return 0;
}
