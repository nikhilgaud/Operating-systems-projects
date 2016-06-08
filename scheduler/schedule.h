//Name - Nikhil Gaud
//login - ngaud

#ifndef _schedule_h_
#define _schedule_h_

typedef struct Node
{
    Node* next;
    Node* prev;
    int pid;
} Node;

typedef struct queue
{
    Node* front;
    Node* rear;
} queue;

void init();
int addProcess(int pid, int priority);
int removeProcess(int pid);
int nextProcess(int &time);
int hasProcess();
int isEmpty(queue* q);
int queueInit(queue* q);
int queuePush(queue* q, int priority);
int queuePop(queue* q);
int queuePopSelective(queue* q, int processId);
#endif
