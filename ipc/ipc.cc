// 
// 
// 
// Nikhil Gaud
// Login - ngaud
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

/*Define Global Variables*/
pid_t   childpid;
pid_t parentProcessId;
timeval t1, t2, t3, t4, t5, t6;
int numtests,signal_numtests;
double elapsedTime;
double childMinTime, childMaxTime, childAvgTime,childTotalTime;
double parentMinTime, parentMaxTime, parentAvgTime,parentTotalTime;
double executionTimeParent, executionTimeChild;
bool child_msgReceived = false;

void sigusr1_handler(int sig) //signal from parent to child
{
	if(signal_numtests<numtests)
	{
		gettimeofday(&t4, NULL);
		executionTimeParent = (t4.tv_sec - t3.tv_sec) * 1000.0;
		executionTimeParent += (t4.tv_usec - t3.tv_usec) / 1000.0;   // us to ms
		if((executionTimeParent < parentMinTime) || parentMinTime == 0)
			{
				parentMinTime = executionTimeParent;
			}
		if((executionTimeParent > parentMaxTime))
			{
				parentMaxTime = executionTimeParent;
			}
		parentTotalTime += executionTimeParent;
		signal_numtests++;
		gettimeofday(&t3, NULL);
		kill(childpid,SIGUSR2);
		
	}
else
		{
			kill(childpid,SIGINT);
		}
}

void sigusr2_handler(int sig) //signal from child to parent
{ 
	if(child_msgReceived==false)
	{
		gettimeofday(&t5, NULL);
		kill(parentProcessId,SIGUSR1);
		child_msgReceived = true;
	}
	else
	{
		gettimeofday(&t6, NULL);
		executionTimeChild = (t6.tv_sec - t5.tv_sec) * 1000.0;
		executionTimeChild += (t6.tv_usec - t5.tv_usec) / 1000.0;   // us to ms
		if((executionTimeChild < childMinTime) || (childMinTime == 0))
		{
			childMinTime = executionTimeChild;
		}
		if((executionTimeChild > childMaxTime))
		{
			childMaxTime = executionTimeChild;
		}
		childTotalTime += executionTimeChild;
		gettimeofday(&t5, NULL);
		kill(parentProcessId,SIGUSR1);
	}	
}

void sigint_handler(int sig)
{
	if(parentProcessId==getpid())
	{
		exit(0);
	}
	else
	{
		gettimeofday(&t6, NULL);
		executionTimeChild = (t6.tv_sec - t5.tv_sec) * 1000.0;
		executionTimeChild += (t6.tv_usec - t5.tv_usec) / 1000.0;   // us to ms

		if((executionTimeChild < childMinTime) || (childMinTime == 0))
		{
			childMinTime = executionTimeChild;
		}
		if((executionTimeChild > childMaxTime))
		{
			childMaxTime = executionTimeChild;
		}
		childTotalTime += executionTimeChild;

		gettimeofday(&t2, NULL);
		elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
		elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms

		printf("Child's Results for Pipe IPC mechanisms\n");
		printf("Process ID is %d, Group ID is %d\n", getpid(),getgid());
		printf("Round trip times\n");
		printf("Average %f\n",childTotalTime/numtests);
		printf("Maximum %f\n",childMaxTime);
		printf("Minimum %f\n",childMinTime);
		printf("Elapsed Time %f\n", elapsedTime);
		exit(0);
	}
}

//
// main - The main routine 
//
int main(int argc, char **argv){
	//Initialize Constants here
	char buffer[10];

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
	signal_numtests = 0;
    //Determine the number of messages was passed in from command line arguments
    //Replace default numtests w/ the commandline argument if applicable 
    if(argc<2){
		printf("Not enough arguments\n");
		exit(0);
	}

    if(argc >= 3)
    {
		numtests = atoi(argv[2]);
    }

    printf("Number of Tests %d\n", numtests);
	childMinTime = 0.0;
	childMaxTime = 0.0;
	childAvgTime =  0.0;
	childTotalTime =  0.0;
	parentMinTime = 0.0;
	parentMaxTime = 0.0;
	parentAvgTime =  0.0;
	parentTotalTime =  0.0;
    // start timer
	gettimeofday(&t1, NULL); 
	if(strcmp(argv[1],"-p")==0)
	{
		pipe(fd1);
    	pipe(fd2);
		//code for benchmarking pipes over numtests
		if(fork() > 0) //parent
		{
			parentProcessId = getpid();
			close(fd1[0]);
			close(fd2[1]);	
			for(int count = 0;count < numtests;count++)	
			{
				gettimeofday(&t3, NULL);
				write(fd1[1], parentmsg, (strlen(parentmsg)+1));

				nbytes = read(fd2[0], buffer, sizeof(buffer));
				gettimeofday(&t4, NULL);

				executionTimeParent = (t4.tv_sec - t3.tv_sec) * 1000.0;
				executionTimeParent += (t4.tv_usec - t3.tv_usec) / 1000.0;   // us to ms
				if((executionTimeParent < parentMinTime) || (parentMinTime == 0))
				{
					parentMinTime = executionTimeParent;
				}
				else if((executionTimeParent > parentMaxTime) || (parentMaxTime == 0))
				{
					parentMaxTime = executionTimeParent;
				}
				parentTotalTime += executionTimeParent;
			}
			waitpid(childpid, NULL, 0);
			printf("Parent's Results for Pipe IPC mechanisms\n");
			printf("Process ID is %d, Group ID is %d\n", parentProcessId,getgid());
			printf("Round trip times\n");
			printf("Average %f\n",parentTotalTime/numtests);
			printf("Maximum %f\n",parentMaxTime);
			printf("Minimum %f\n",parentMinTime);
		}
		else //child
		{
			childpid = getpid();
			close(fd1[1]);
			close(fd2[0]);
			for(int count = 0;count < numtests;count++)
			{
				gettimeofday(&t5, NULL);
				nbytes = read(fd1[0], buffer, sizeof(buffer));

				write(fd2[1], childmsg, (strlen(childmsg)+1));
				gettimeofday(&t6, NULL);

				executionTimeChild = (t6.tv_sec - t5.tv_sec) * 1000.0;
				executionTimeChild += (t6.tv_usec - t5.tv_usec) / 1000.0;   // us to ms
				if((executionTimeChild < childMinTime) || (childMinTime == 0))
				{
					childMinTime = executionTimeChild;
				}
				else if((executionTimeChild > childMaxTime) || (childMaxTime == 0))
				{
					childMaxTime = executionTimeChild;
				}
				childTotalTime += executionTimeChild;
			}
			printf("Child's Results for Pipe IPC mechanisms\n");
			printf("Process ID is %d, Group ID is %d\n", childpid,getgid());
			printf("Round trip times\n");
			printf("Average %f\n",childTotalTime/numtests);
			printf("Maximum %f\n",childMaxTime);
			printf("Minimum %f\n",childMinTime);
		}

		// stop timer
		gettimeofday(&t2, NULL);

		// compute and print the elapsed time in millisec
		elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
		elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
		printf("Elapsed Time %f\n", elapsedTime);

	}
	if(strcmp(argv[1],"-s")==0){
		//code for benchmarking signals over numtests
	parentProcessId = getpid();
	if((childpid=fork()) > 0)
	{
		gettimeofday(&t3, NULL);
		kill(childpid,SIGUSR2);
	}
	else
	{
		while(1);
	}
		
		// stop timer
		if(parentProcessId==getpid())
		{
			int status;
			waitpid(childpid, &status, WUNTRACED);

			gettimeofday(&t2, NULL);

			// compute and print the elapsed time in millisec
			elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
			elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
			printf("Parent's Results for Pipe IPC mechanisms\n");
			printf("Process ID is %d, Group ID is %d\n", parentProcessId,getgid());
			printf("Round trip times\n");
			printf("Average %f\n",elapsedTime/numtests);
			printf("Maximum %f\n",parentMaxTime);
			printf("Minimum %f\n",parentMinTime);
			printf("Elapsed Time %f\n", elapsedTime);
		}
	}
}