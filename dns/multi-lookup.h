#ifndef MULTI_LOOKUPH
#define MULTI_LOOKUPH

#define MAX_INPUT_FILES 10
#define MAX_NAME_LENGTH 1025
#define MAX_RESOLVER_THREADS 10
#define MAX_REQUESTER_THREADS 10
#define MIN_REQUESTER_THREADS 2
#define MAX_IP_LENGTH INET6_ADDRSTRLEN
#define MIN_RESOLVER_THREADS 2

#define INPUTFS "%1024s"
#define BUFF_LENGTH 1025
#define USAGE "<inputFilePath> <outputFilePath>"
#define MIN_INPUT_FILES 1
#define MINARGS (MIN_INPUT_FILES+2)
#define MAXARGS (MAX_INPUT_FILES+2)

void* Requester();
void* Resolver();
void InitObjs();
void DestroyObjs();
void MutexReqCnt();

#endif
