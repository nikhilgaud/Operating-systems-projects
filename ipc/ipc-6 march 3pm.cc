// 
// 
// 
// <Put your name and ID here>
//

using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string>
#include <limits.h>
#include <sys/time.h>

#include "helper-routines.h"

#define READ_END 0
#define WRITE_END 1

/*Define Global Variables*/
pid_t   childpid;
pid_t parentProcessId;
timeval t1, t2, t3, t4, t5, t6;
int numtests;
double elapsedTime;


void sigusr1_handler(int sig) 
{	
	
}

void sigusr2_handler(int sig) 
{ 
	
}

void sigint_handler(int sig){
	
}


//
// main - The main routine 
//
int main(int argc, char **argv){
	//Initialize Constants here
	char buffer[10];

	double childMinTime, childMaxTime, childAvgTime,childTotalTime;
	double parentMinTime, parentMaxTime, parentAvgTime,parentTotalTime;
	double executionTimeParent, executionTimeChild;

    //variables for Pipe
	int fd1[2],fd2[2], nbytes;	
	//byte size messages to be passed through pipes	
	char    childmsg[] = "1";
	char    parentmsg[] = "2";
	char    quitmsg[] = "q";
    
    /*Three Signal Handlers You Might Need
     *
     *I'd recommend using one signal to signal parent from child
     *and a different SIGUSR to signal child from parent
     */
    Signal(SIGUSR1,  sigusr1_handler); //User Defined Signal 1
    Signal(SIGUSR2,  sigusr2_handler); //User Defined Signal 2
    Signal(SIGINT, sigint_handler);
    
    //Default Value of Num Tests
    numtests=10000;
    //Determine the number of messages was passed in from command line arguments
    //Replace default numtests w/ the commandline argument if applicable 
    if(argc<2){
		printf("Not enough arguments\n");
		exit(0);
	}
    pipe(fd1);
    pipe(fd2);
    printf("Number of Tests %d\n", numtests);
    // start timer
	gettimeofday(&t1, NULL); 
	if(strcmp(argv[1],"-p")==0){
		//code for benchmarking pipes over numtests
		childMinTime = 0;
		childMaxTime = 0;
		childAvgTime =  0;
		childTotalTime =  0;
		parentMinTime = 0;
		parentMaxTime = 0;
		parentAvgTime =  0;
		parentTotalTime =  0;
		if(fork() > 0)
		{
			parentProcessId = getpid();
			//int count = 0;	
			for(int count = 0;count < numtests;count++)	
			//while(count != numtests)
			{
			close(fd1[0]);
			gettimeofday(&t3, NULL);
			write(fd1[1], parentmsg, (strlen(parentmsg)+1));
			//printf("Parent sent %s for %d\n",parentmsg,count);
			close(fd1[1]);

			//waitpid(childpid, NULL, 0);

			close(fd2[1]);
			nbytes = read(fd2[0], buffer, sizeof(buffer));
			gettimeofday(&t4, NULL);

			close(fd2[0]);

		executionTimeParent = (t4.tv_sec - t3.tv_sec) * 1000.0;
		executionTimeParent += (t4.tv_usec - t3.tv_usec) / 1000.0;   // us to ms
if((count == 0))
{
	parentMinTime = executionTimeParent;
	parentMaxTime = executionTimeParent;
}
else if((count > 0) && (executionTimeParent < parentMinTime))
{
	parentMinTime = executionTimeParent;
}
else if((count > 0) && (executionTimeParent > parentMaxTime))
{
	parentMaxTime = executionTimeParent;
}
parentTotalTime += executionTimeParent;
			//printf("Parent time %f\n",executionTimeParent);
			//count++;
			}
			//if()
			waitpid(childpid, NULL, 0);
			printf("Parent's Results for Pipe IPC mechanisms\n");
			printf("Process ID is %d, Group ID is %d\n", parentProcessId,getgid());
			printf("Round trip times\n");
			printf("Average %f\n",parentTotalTime/numtests);
			printf("Maximum %f\n",parentMaxTime);
			printf("Minimum %f\n",parentMinTime);
		}
		else
		{
			childpid = getpid();
			//int count = 0;
for(int count = 0;count < numtests;count++)
			//while(count != numtests)
			{
			close(fd1[1]);
			gettimeofday(&t5, NULL);
			nbytes = read(fd1[0], buffer, sizeof(buffer));
//printf("Child received %s for %d\n",buffer,count);
			close(fd1[0]);

			close(fd2[0]);
			write(fd2[1], childmsg, (strlen(childmsg)+1));
			gettimeofday(&t6, NULL);
//printf("Child sent %s for %d\n",childmsg,count);
			close(fd2[1]);

		executionTimeChild = (t6.tv_sec - t5.tv_sec) * 1000.0;
		executionTimeChild += (t6.tv_usec - t5.tv_usec) / 1000.0;   // us to ms
if((count == 0))
{
	childMinTime = executionTimeChild;
	childMaxTime = executionTimeChild;
}
else if((count > 0) && (executionTimeChild < childMinTime))
{
	childMinTime = executionTimeChild;
}
else if((count > 0) && (executionTimeChild > childMaxTime))
{
	childMaxTime = executionTimeChild;
}
childTotalTime += executionTimeChild;
			//printf("Child time %f\n",executionTimeChild);
//printf("Min for %d: %f\n", count, executionTimeChild);
			//count++;
			}
			printf("Child's Results for Pipe IPC mechanisms\n");
			printf("Process ID is %d, Group ID is %d\n", childpid,getgid());
			printf("Round trip times\n");
			printf("Average %f\n",childTotalTime/numtests);
			printf("Maximum %f\n",childMaxTime);
			printf("Minimum %f\n",childMinTime);
		}

/*close(fd1[0]);		
close(fd1[1]);
close(fd2[0]);
close(fd2[1]);*/
		// stop timer
		gettimeofday(&t2, NULL);

		// compute and print the elapsed time in millisec
		elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
		elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
		printf("Elapsed Time %f\n", elapsedTime);

	}
	if(strcmp(argv[1],"-s")==0){
		//code for benchmarking signals over numtests
		
		
		// stop timer
		gettimeofday(&t2, NULL);

		// compute and print the elapsed time in millisec
		elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
		elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
		printf("Elapsed Time %f\n", elapsedTime);
	}
	exit(0);
}
  










