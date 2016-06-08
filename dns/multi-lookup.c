#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include "queue.h"
#include "util.h"
#include "multi-lookup.h"
#include <pthread.h>

int thread_count = MAX_REQUESTER_THREADS;
queue request_q;
pthread_mutex_t qMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t outMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t reqCntMutex = PTHREAD_MUTEX_INITIALIZER;

void* Requester(void* inputFileName)
{
	FILE *inFilePtr;
	char *fileName = (char *)inputFileName;
	
	/* Open Input File */
	inFilePtr = fopen(fileName, "r");
	if(!inFilePtr)
	{
	    fprintf(stderr, "Error while opening the input file: %s\n", fileName);
		MutexReqCnt();
	    return NULL;
	}

	for(;;)
	{
		char *domain = (char*)malloc(sizeof(char)*BUFF_LENGTH);
		if(fscanf(inFilePtr,INPUTFS,domain) <= 0)
		{
			free(domain);
			break;
		}
		pthread_mutex_lock(&qMutex);
		while(queue_is_full(&request_q))
		{
			pthread_mutex_unlock(&qMutex);
			usleep((rand()%101));
			pthread_mutex_lock(&qMutex);
		}
		printf("Pushing hostname into queue: %s\n", domain);
		queue_push(&request_q,domain);
		pthread_mutex_unlock(&qMutex);
	}
	fclose(inFilePtr);
	printf("Requester thread will exit now.\n");
	
	MutexReqCnt();
	return NULL;
}

void* Resolver(void* outFileName)
{
	FILE *outFilePtr;
	char* domain_name = NULL;
	char* outputFileName = (char*)outFileName;
	char** IPArr;
	int IPCount = 0;	
	pthread_mutex_lock(&reqCntMutex);
	while(thread_count > 0 || !queue_is_empty(&request_q))
	{
		IPCount = 0;
		pthread_mutex_unlock(&reqCntMutex);
		pthread_mutex_lock(&qMutex);
		if(!queue_is_empty(&request_q))
		{
			pthread_mutex_lock(&outMutex);
			outFilePtr = fopen(outputFileName,"a");
			if(!outFilePtr)
			{
				fprintf(stderr, "Error occured while opening output file %s\n", outputFileName);
				pthread_mutex_unlock(&outMutex);
				return NULL;
			}
			domain_name = queue_pop(&request_q);
			printf("Popping hostname from queue %s\n", domain_name);
			pthread_mutex_unlock(&qMutex);
		
		//#region 2
		IPArr = malloc(sizeof(char*)*255);
		if(dnsLookupMultipleIPs(&IPCount,domain_name,255,IPArr) == UTIL_FAILURE)
		{
			fprintf(stderr, "dnslookup error: %s\n", domain_name);
		}
		fprintf(outFilePtr, "%s", domain_name); 
		int iterator = 0;
		while(iterator < IPCount)
		{
			fprintf(outFilePtr,",%s", IPArr[iterator]);
			if(IPArr[iterator] != NULL)
			{
				free(IPArr[iterator]);
			}
			iterator++;
		}
		if(IPCount == 0)
		{
			fprintf(outFilePtr, ",");
		}
		fprintf(outFilePtr,"\n");
		if(IPArr != NULL)
		{
		  free(IPArr); 
		}
		//#end region 2	
		fclose(outFilePtr);
		pthread_mutex_unlock(&outMutex);
		free(domain_name);
		}
		else
		{
			pthread_mutex_unlock(&qMutex);
		}
		pthread_mutex_lock(&reqCntMutex);
	}
	pthread_mutex_unlock(&reqCntMutex);
	return NULL;
}

void MutexReqCnt()
{
	pthread_mutex_lock(&reqCntMutex);
	thread_count--;
	pthread_mutex_unlock(&reqCntMutex);
}

void InitObjs()
{
	if(queue_init(&request_q, QUEUEMAXSIZE) == QUEUE_FAILURE)
	{
		fprintf(stderr, "Error occured while queue initialization.\n");
		exit(EXIT_FAILURE);
	}
}

void DestroyObjs()
{	
	//cleanup the queue
	pthread_mutex_lock(&qMutex);
	queue_cleanup(&request_q);
	pthread_mutex_unlock(&qMutex);
	
	//destroy mutexes
	pthread_mutex_destroy(&qMutex);
	pthread_mutex_destroy(&outMutex);
	pthread_mutex_destroy(&reqCntMutex);
}

int main(int argc, char* argv[])
{
    void *exit_status = NULL;
    FILE *testOutFile = NULL;
	
	if(argc < MINARGS || argc > MAXARGS)
	{
		fprintf(stderr, "Incorrect number of arguments: %d\n",(argc-1));
		fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
		return EXIT_FAILURE;
    }
	
	testOutFile = fopen(argv[argc - 1],"w");
	if(testOutFile)
	{
		fclose(testOutFile);
	}
	else
	{
		fprintf(stderr, "Bogus output file path...exiting\n");
		fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
		return -1;
	}
	
	int proc_cores = sysconf(_SC_NPROCESSORS_ONLN)*2;
	int fileCount = argc - 2;
	thread_count = fileCount;
	pthread_t resolverPool[MAX_RESOLVER_THREADS];
    pthread_t requesterPool[MIN_RESOLVER_THREADS];
	if(proc_cores < MAX_RESOLVER_THREADS)
	{
		proc_cores = proc_cores;
	}
	else
	{
		proc_cores = MAX_RESOLVER_THREADS;
	}
	
	if(proc_cores > MIN_RESOLVER_THREADS)
	{
		proc_cores = proc_cores;
	}
	else
	{
		proc_cores = MIN_RESOLVER_THREADS;
	}
	
	printf("Number of threads(matched with number of processor cores, within constraints) on system: %d\n", proc_cores);
	
	InitObjs();
    
	int i = 0;
	while(i < fileCount)
	{
		pthread_create(&(requesterPool[i]), NULL, Requester, argv[i+1]);
		++i;
	}
	
	i = 0;
	while(i < proc_cores)
	{
		pthread_create(&(resolverPool[i]), NULL, Resolver, argv[argc-1]);
		++i;
	}
	i=0;
	while(i < fileCount)
	{
		pthread_join(requesterPool[i], exit_status);
		++i;
	}
	
	i = 0;
	while(i < proc_cores)
	{
		pthread_join(resolverPool[i], exit_status);
		++i;
	}
	
	DestroyObjs();

	return EXIT_SUCCESS;
}
