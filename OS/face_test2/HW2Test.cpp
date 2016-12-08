/*
 * ExampleTest.c
 *
 *  Created on: Oct 24, 2016
 *      Author: itay887
 */

#include "cutest.h"
#include "test_utils.hpp"
#include <unistd.h>
#include <sched.h>
#include "hw2_syscalls.h"
#include <errno.h>
#define INPUT_FILE "settings.txt"

using std::string;
//#define ONE_SEC_ITER 157000000*2
//#define THREE_SEC_ITER ONE_SEC_ITER*3
//#define TWO_SEC_ITER ONE_SEC_ITER*2

#define ONE_SEC_ITER 150500000
#define THREE_SEC_ITER ONE_SEC_ITER*3
#define TWO_SEC_ITER ONE_SEC_ITER*2
void funcToCompute(unsigned int iterations){
	double c=7.51,d=0;
	int i;
	for(i=0;i < iterations;++i){
		d=c*5+45/c+11;
		d=c;
	}
	
}

#define RUN_FOR_ONE_SECOND funcToCompute(ONE_SEC_ITER);
#define RUN_FOR_TWO_SECOND funcToCompute(TWO_SEC_ITER);
#define RUN_FOR_THREE_SECOND funcToCompute(THREE_SEC_ITER);
#define RUN_FOR_HUNDRED_MS(ms) funcToCompute(ms*ONE_SEC_ITER/10);
#define SHORT_NORMAL 1
#define SHORT_OVERDUE 0

//Setting test environemt with parameters
static TestUtils utils("settings-hw2.txt");
void SetParams();
void (*init_func)(void)=SetParams;


#define SCHED_SHORT 5
typedef struct my_prio_t{
	  int sched_priority; //ignored for SHORT processes
  int requested_time;
} MyPrio;

typedef std::vector<int>::iterator VecItr;


void SetParams()
{
	utils.Init();
	utils.PrintParameters("BasicTest");
}
//End of setting environment
//Tests here



void GeneralShortTest()
{
	MyPrio pri;
	int iter=utils.ParamToInt("Iterations");
	int pidson=fork();
	if(pidson==0)
	{
		pri.requested_time=2000;
		for(int i=0;i<iter;i++){
		int my_pid=getpid();
		int res=sched_getscheduler(my_pid);
		TEST_CHECK_(res==5,"Scheduling policy:Expected 5, got %d",5);
		res=is_short(my_pid);
		TEST_CHECK_(res==SHORT_NORMAL,"is_short() on normal SHORT process:\n returned %d, expected %d.\n errno:%d",res,SHORT_NORMAL,errno);
		res=short_remaining_time(my_pid);
		TEST_CHECK_(res >= 1900,"Started the process with 2000ms. Expected at least 1900, got %d",res);
		RUN_FOR_TWO_SECOND
		RUN_FOR_HUNDRED_MS(4)
		
		
		res=is_short(my_pid);
		TEST_CHECK_(res==SHORT_OVERDUE,"is_short() on overdue SHORT process:\n returned %d, expected %d.\n errno:%d",res,SHORT_OVERDUE,errno);
		
		RUN_FOR_THREE_SECOND
		RUN_FOR_TWO_SECOND
		
		res=is_short(my_pid);
		TEST_CHECK_(res == -1 && errno == EINVAL,"Overdue process shouldv'e become OTHER.\nis_short() returned %d expected %d\nerrno %d",res,-1,errno);
		res=was_short(my_pid);
		TEST_CHECK_(res == 1,"was_short() should be %d, got %d",1,res);
		res=sched_getscheduler(my_pid);
		TEST_CHECK_(res == 0,"Expected OTHER policy. expected %d got %d\nerrno %d",0,SCHED_OTHER,errno);
		
		res=sched_setscheduler(my_pid,SCHED_SHORT,(const sched_param*)&pri);
		TEST_CHECK_(res == 0,"Set scheduler for an OTHER process that was short should succeed.\n expected %d got %d\nerrno %d",0,res,errno);
		}
		exit(0);
		
	}else{
		int res=is_short(pidson);
		TEST_CHECK_((res == -1) && (errno == EINVAL),"Process wasn't declared short:\n expected %d, got %d",EINVAL,errno);
		
		pri.requested_time=0;
		res=sched_setscheduler(pidson,5,(const sched_param*)(&pri));
		TEST_CHECK_((res == -1) && (errno == EINVAL),"setscheduler() was called with 0 requested time:\nexpected errno:%d, got %d",EINVAL,errno);
		// too high
		pri.requested_time=3001;
		res=sched_setscheduler(pidson,5,(const sched_param*)(&pri));
		TEST_CHECK_((res == -1) && (errno == EINVAL),"setscheduler() was called with 3001 requested time:\nexpected errno:%d, got %d",EINVAL,errno);

		pri.requested_time=2000;
		res=sched_setscheduler(pidson,5,(const sched_param*)(&pri));
		TEST_CHECK_((res == 0),"setscheduler() was called with %d requested time:\nexpected %d, got %d\n errno:%d",pri.requested_time,0,res,errno);

		//negative check
		
		
		wait(&res);
		
	}

}

static int pfd[2];
void Exec(unsigned int expected_place,int pi_nice)
{
	RUN_FOR_ONE_SECOND
	RUN_FOR_HUNDRED_MS(3);
	int place;
	read(pfd[0],&place,sizeof(int));
	TEST_CHECK_(place == expected_place,"SHORT Process %d with nice %d was supposed to be in place %d, got %d",getpid(),pi_nice,expected_place,place);
	place+=1;
	write(pfd[1],&place,sizeof(int));
}
void ExecFifo(unsigned int expected_place)
{
	RUN_FOR_ONE_SECOND
	RUN_FOR_HUNDRED_MS(3);
	int place;
	read(pfd[0],&place,sizeof(int));
	TEST_CHECK_(place == expected_place,"FIFO Process %d was supposed to be in place %d, got %d",getpid(),expected_place,place);
	place+=1;
	write(pfd[1],&place,sizeof(int));
	
}
void ExecOther(unsigned int expected_place,int pi_nice)
{
	RUN_FOR_ONE_SECOND
	RUN_FOR_HUNDRED_MS(3);
	int place;
	read(pfd[0],&place,sizeof(int));
	TEST_CHECK_(place == expected_place,"OTHER Process %d with nice %d was supposed to be in place %d, got %d",getpid(),pi_nice,expected_place,place);
	place+=1;
	write(pfd[1],&place,sizeof(int));
}
void ShortArbitrationTest()
{
	
	pipe(pfd);
	
	std::vector<int> nices;
	utils.LoadToIntVec("nices",nices);
	std::sort(nices.begin(),nices.end());
	MyPrio prio;
	prio.requested_time=3000;
	prio.sched_priority=1;
	sched_setscheduler(getpid(),SCHED_FIFO,(const sched_param*)&prio);
	prio.sched_priority=0;
	unsigned int count=0;
	unsigned int place=1;
	int res;
	unsigned int size_vec=nices.size();
	write(pfd[1],&count,sizeof(int));
	RUN_FOR_HUNDRED_MS(2);
	for(VecItr itr=nices.begin();itr != nices.end(); ++itr){
		int somepid=fork();
		if(somepid==0){
			int my_pid=getpid();
			
			nice(*itr);
			Exec(place,*itr);
			close(pfd[0]);
			close(pfd[1]);
			exit(0);
		}
		res=sched_setscheduler(somepid,SCHED_OTHER,(const sched_param*)(&prio));
		res=sched_setscheduler(somepid,SCHED_SHORT,(const sched_param*)(&prio));
		TEST_CHECK_(res == 0,"setscheduler() for process %d should succeed. got errno %d",somepid,errno);
		somepid=fork();
		if(somepid==0){
			nice(*itr);
			ExecOther(place+size_vec,*itr);
			close(pfd[0]);
			close(pfd[1]);
			exit(0);
		}
		res=sched_setscheduler(somepid,SCHED_OTHER,(const sched_param*)(&prio));
		
		place++;
	}
	read(pfd[0],&count,4);
	TEST_CHECK_(count == 0,"Father should finish first since he is FIFO.expected place=%d, got %d",0,count);
	count++;
	write(pfd[1],&count,sizeof(int));
	while(wait(&place)!=-1);
	close(pfd[0]);
	close(pfd[1]);
	
}


void ShortForkTest()
{
	int sonpid;
	MyPrio prio;
	prio.requested_time=2000;//2000?
	int res=sched_setscheduler(getpid(),SCHED_SHORT,(const sched_param*)&prio);
	TEST_CHECK_(res == 0,"Setting SCHED_SHORT for parent: expected %d got %d\n errno %d",0,res,errno);
	int count = 0;
	pipe(pfd);
	sonpid=fork();
	if(sonpid == 0)
	{
		res=sched_getscheduler(getpid());
		TEST_CHECK_(res == SCHED_SHORT,"Son process expected to be SHORT. expected %d got %d",SCHED_SHORT,res);
		res=short_remaining_time(getpid());
		TEST_CHECK_(res <= prio.requested_time/2+1,
				"Parent process started with %d, so expected at most half of that:%d got %d",prio.requested_time,prio.requested_time/2+1,res);
		TEST_CHECK_(count == 0,"This process should have started before parent could increment count.\n Didn't happen! count=%d",count);
		count++;
		write(pfd[1],&count,sizeof(int));
		exit(0);
	}
	count+=4;//to check if the count above will increment after this one
	res=short_remaining_time(getpid());
	TEST_CHECK_(res <= prio.requested_time/2,
				"Parent process started with %d, so expected half of that:%d got %d",prio.requested_time,prio.requested_time/2+1,res);
	read(pfd[0],&count,sizeof(int));
	TEST_CHECK_(count == 1,"Parent should be executed after child. count=%d",count);
	wait(&res);
}

void ShortOverdueForkTest()
{
	int sonpid;
	MyPrio prio;
	prio.requested_time=1600;//2000?
	int res=sched_setscheduler(getpid(),SCHED_SHORT,(const sched_param*)&prio);
	TEST_CHECK_(res == 0,"Setting SCHED_SHORT for parent: expected %d got %d\n errno %d",0,res,errno);
	int count = 0;
	pipe(pfd);
	RUN_FOR_TWO_SECOND;
	res=is_short(getpid());
	TEST_CHECK_(res == SHORT_OVERDUE,"Expected process to be overdue. from is_short() got %d\nerrno %d",res,errno);
	int time_remain=short_remaining_time(getpid());
	write(pfd[1],&count,sizeof(int));
	sonpid=fork();
	if(sonpid == 0)
	{
		res=sched_getscheduler(getpid());
		TEST_CHECK_(res == SCHED_SHORT,"Son process expected to be SHORT. expected %d got %d",SCHED_SHORT,res);
		res=short_remaining_time(getpid());
		TEST_CHECK_(res <= time_remain,
				"Parent process became overdue with %d time remain,\nso expected at least that:%d got %d",time_remain,time_remain,res);
		read(pfd[0],&count,sizeof(int));
		TEST_CHECK_(count == 1,"This process should have started after parent.\n Didn't happen! count=%d",count);
		count++;
		write(pfd[1],&count,sizeof(int));
		exit(0);
	}
	//to check if the count above will increment after this one
	res=short_remaining_time(getpid());
	read(pfd[0],&count,sizeof(int));
	TEST_CHECK_(count == 0,"Parent should be executed before child. count=%d",count);
	count++;
	write(pfd[1],&count,sizeof(int));
	
	close(pfd[0]);
	close(pfd[1]);
	wait(&res);
	
}


void ParentShortTest(){

	int sonpid;
	MyPrio prio;
	prio.requested_time=2000;
	pipe(pfd);
	int count=0;
	write(pfd[1],&count,sizeof(int));
	int parpid=getpid();
	//std::cout<<"Parent PID="<<parpid<<"\n";
	sonpid=fork();
	if(sonpid == 0){
		read(pfd[0],&count,sizeof(int));
		TEST_CHECK_(count == 0,"Count shouldn't have changed:expected %d got %d",0,count);
		int res;
		count++;//before context_switch
		write(pfd[1],&count,sizeof(int));
		nice(8);
		res=sched_getscheduler(getpid());
		TEST_CHECK(res == SCHED_SHORT);
		res=sched_setscheduler(parpid,SCHED_SHORT,(const sched_param*)&prio);
		TEST_CHECK_(res == 0,"Setting parent scheduling to short:expected %d got %d\nerrno ",0,res,errno);
		read(pfd[0],&count,sizeof(int));
		TEST_CHECK_(count == 2,"Parent shouldv'e incremented count:expected %d got %d",2,count);
		count++;
		write(pfd[1],&count,sizeof(int));
		exit(0);
	}
	int res=sched_setscheduler(sonpid,SCHED_SHORT,(const sched_param*)&prio);
	TEST_CHECK_(res == 0,"Setting the scheduling of son to SHORT before setting parent.\n expected %d got %d\nerrno ",0,res,errno);
	//std::cout<<"Son PID="<<sonpid<<"\n";
	read(pfd[0],&count,sizeof(int));
	TEST_CHECK_(count == 1,"Son shouldv'e incremented count before setting parent to SHORT:\n expected %d got %d",1,count);
	count++;
	write(pfd[1],&count,sizeof(int));
	nice(10);//should cause context switch
	read(pfd[0],&count,sizeof(int));
	TEST_CHECK_(count == 3,"Son shouldv'e incremented count after parent increases its nice:\n expected %d got %d",3,count);
	res=sched_getscheduler(getpid());
	TEST_CHECK(res == SCHED_SHORT);
	close(pfd[0]);
	close(pfd[1]);
	wait(&res);
}

void ShortYieldTest()
{
	int sonpid,count=0;
	MyPrio prio;
	prio.requested_time=2000;
	nice(-10);
	pipe(pfd);
	write(pfd[1],&count,4);
	int res=sched_setscheduler(getpid(),SCHED_SHORT,(const sched_param*)&prio);
	TEST_CHECK_(res == 0,"Setting process to be short should succeed. expected %d got %d\nerrno %d",0,res,errno);
	sonpid=fork();
	if(sonpid==0)
	{
		read(pfd[0],&count,4);
		TEST_CHECK_(count == 0,"Son shouldv'e started before father.expected %d got %d",0,count);
		count++;
		write(pfd[1],&count,4);
		
		sched_yield();
		
		read(pfd[0],&count,4);
		TEST_CHECK_(count == 2,"Son shouldv'e continued after yield.expected %d got %d",2,count);
		count++;
		write(pfd[1],&count,4);
		exit(0);
	}
	read(pfd[0],&count,4);
	TEST_CHECK_(count == 1,"Father shouldv'e continued after son yielded.expected %d got %d",1,count);
	count++;
	write(pfd[1],&count,4);
	wait(&count);
	read(pfd[0],&count,4);
	TEST_CHECK_(count == 3,"Father shouldv'e continued after son finished.expected %d got %d",3,count);
}

void ShortVsFIFOTest()
{
	pipe(pfd);
	
	std::vector<int> nices;
	utils.LoadToIntVec("nices",nices);
	std::sort(nices.begin(),nices.end());
	MyPrio prio,son_prio;
	prio.requested_time=3000;
	prio.sched_priority=1;
	son_prio=prio;
	unsigned int count=0;
	unsigned int place=0;
	unsigned int size_vec=nices.size();
	sched_setscheduler(getpid(),SCHED_FIFO,(const sched_param*)(&prio));
	prio.sched_priority=0;
	write(pfd[1],&count,sizeof(int));
	RUN_FOR_HUNDRED_MS(2);
	for(VecItr itr=nices.begin();itr != nices.end(); ++itr){
		int somepid=fork();
		if(somepid==0){
			
			sched_yield();
			nice(*itr);
			Exec(size_vec+place+1,*itr);
			close(pfd[0]);
			close(pfd[1]);
			exit(0);
		}
		int res=sched_setscheduler(somepid,SCHED_OTHER,(const sched_param*)(&prio));//setting son to be SHORT
		res=sched_setscheduler(somepid,SCHED_SHORT,(const sched_param*)(&prio));//setting son to be SHORT
		TEST_CHECK_(res == 0,"setscheduler() for process %d should succeed. got errno %d",somepid,errno);
		somepid=fork();//this one is fifo
		if(somepid==0){
			sched_yield();
			ExecFifo(place+1);
			close(pfd[0]);
			close(pfd[1]);
			exit(0);
		}
		place++;
	}
	//std::cout<<"Father should reach here first...\nWaiting for sons...\n";
	read(pfd[0],&count,4);
	TEST_CHECK_(count == 0,"Father is FIFO so should finish first. expected %d got %d",0,count);
	count++;
	write(pfd[1],&count,4);
	
	while(wait(&place)!=-1);
	
	close(pfd[0]);
	close(pfd[1]);
	while(wait(&count)!=-1);
	
}
/*
void ShortGeneralTest()
{
	//int iter=utils.ParamToInt("Iterations");
	int mypid=getpid();
	int res=0;
	MyPrio prio;
	prio.requested_time=2001;
	res = sched_setscheduler(getpid(),SCHED_SHORT,(const sched_param*)(&prio));
	TEST_CHECK_(res == 0,"Set process to short.\nexpected %d got %d",SCHED_SHORT,res);
	int lower_time=100;
	for(int i=0; i < 1;++i){
		std::cout<<"iteration number "<<i<<std::endl;
		res=sched_getscheduler(mypid);
		TEST_CHECK_(res != SCHED_SHORT,"Process at start of loop shouldn't be short. got %d",res);
		
		
		res=is_short(mypid);
		TEST_CHECK_(res == SHORT_NORMAL,"Should be normal SHORT process.\n is_short() returned %d,expected %d\nerrno %d",res,SHORT_NORMAL,errno);
		RUN_FOR_ONE_SECOND
		std::cout<<"Before setparam with error..."<<std::endl;
		res = sched_setparam(mypid,(const sched_param*)&prio);
		TEST_CHECK_(res == -1 && errno == EINVAL,"Tried to set param to higher time then the time the process run\nexpected res=%d got %d\n errno %d",-1,res,errno);
		
		RUN_FOR_ONE_SECOND;
		RUN_FOR_HUNDRED_MS(4);
		res=is_short(mypid);
		TEST_CHECK_(res == SHORT_OVERDUE,"Should be overdue SHORT process - ran for over 2 seconds.\n is_short() returned %d,expected %d\nerrno %d",res,SHORT_OVERDUE,errno);
		std::cout<<"Before succesfull setparam..."<<std::endl;
		prio.requested_time = 100;
		res = sched_setparam(mypid,(const sched_param*)&prio);
		TEST_CHECK_(res == 0,"Setting sched param for overdue short process with lower time. expected res=%d got %d\nerrno %d",0,res,errno);
		
		RUN_FOR_HUNDRED_MS(2);
		res=sched_getscheduler(mypid);
		TEST_CHECK_(res == SCHED_OTHER,"Process at end of loop should be OTHER. got %d",res);
		
		res=was_short(mypid);
		TEST_CHECK_(res == 1,"Process was short.was_short() returned %d expected %d",res,1);
		
		res=is_short(mypid);
		TEST_CHECK_(res == -1 && errno==EINVAL,"Should be OTHER process.\n is_short() returned %d,expected %d\nerrno %d",res,-1,errno);
		
		prio.requested_time=2000;
	}
}*/
TEST_LIST ={
		{"GeneralShortTest",GeneralShortTest},
		{"ShortArbitrationTest",ShortArbitrationTest},
		{"ShortForkTest",ShortForkTest},
		{"ShortOverdueForkTest",ShortOverdueForkTest},
		{"ParentShortTest",ParentShortTest},
		{"ShortYieldTest",ShortYieldTest},
		{"ShortVsFIFOTest",ShortVsFIFOTest},
		//{"ShortGeneralTest",ShortGeneralTest},
		{0}
};
