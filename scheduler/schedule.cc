//Name - Nikhil Gaud
//login - ngaud

#include <stdio.h>
#include <stdlib.h>
#include "schedule.h"

//queue array
queue arrQueue[4];
//queue pointer
int currQueuePtr;
/**
 * Function to initialize any global variables for the scheduler. 
 */
void init()
{
	int i = 0;
	currQueuePtr = 0;
	
	while(i < 4)
	{
		queueInit(arrQueue + i);
		i++;
	}
}

/**
 * Function to add a process to the scheduler
 * @Param pid - the ID for the process/thread to be added to the 
 *      scheduler queue
 * @Param priority - priority of the process being added
 * @return true/false response for if the addition was successful
 */
int addProcess(int pid, int priority)
{
	//if(priority >= 0 && priority < 4)
	{
		queuePush(arrQueue + (priority-1),pid);
	}
}

/**
 * Function to remove a process from the scheduler queue
 * @Param pid - the ID for the process/thread to be removed from the
 *      scheduler queue
 * @Return true/false response for if the removal was successful
 */
int removeProcess(int pid)
{
	int i = 0;
	while(i < 4)
	{
		if(queuePopSelective(arrQueue + i, pid)==1)
		{
			return 1;
		}
		i++;
	}
	return 0;
}
/**
 * Function to get the next process from the scheduler
 * @Param time - pass by reference variable to store the quanta of time
 * 		the scheduled process should run for
 * @Return returns the thread id of the next process that should be 
 *      executed, returns -1 if there are no processes
 */
int nextProcess(int &time)
{
	int pid;
	while(hasProcess())
	{
		if(!isEmpty(arrQueue + currQueuePtr))
		{
			pid = queuePop(arrQueue + currQueuePtr);
			time = 4 - currQueuePtr;
			queuePush(arrQueue + currQueuePtr, pid);
			currQueuePtr = (currQueuePtr+1)%4;
			return pid;
		}
		currQueuePtr = (currQueuePtr+1)%4;
	}
}

/**
 * Function that returns a boolean 1 True/0 False based on if there are any 
 * processes still scheduled
 * @Return 1 if there are processes still scheduled 0 if there are no more 
 *		scheduled processes
 */
int hasProcess()
{
	int i = 0;
	while(i < 4)
	{
		if(!isEmpty(arrQueue + i))
		{
			return 1;
		}
		i++;
	}
return 0;
}

int queueInit(queue* q)
{    
    //set the front and rear to NULL
    q->front = NULL;
    q->rear = NULL;
}

int isEmpty(queue* q)
{
    if((q->front == NULL) && (q->rear == NULL))
	{
		return 1;
    }
    else
	{
		return 0;
    }
}

int queuePop(queue* q)
{
    int retPid;	
    if(isEmpty(q))
	{
		return -1;
    }
	if(q->front == q->rear)
	{
		retPid = q->front->pid;
		free(q->front);
		q->front = NULL;
		q->rear = NULL;
		return retPid;
	}
	else if(q->front != q->rear)
	{
		retPid = q->front->pid;
		(q->front) = (q->front)->next;
		free(q->front->prev);
		q->front->prev = NULL;
		return retPid;
	}
}

int queuePopSelective(queue* q, int processId)
{
	if(isEmpty(q))
	{
		return 0;
    }
	int retPid;
	Node *curr,*nodeFound = NULL;
	for(curr = q->front;curr!=NULL;curr = curr->next)
	{
		if(curr->pid == processId)
		{
			nodeFound = curr;
			break;
		}
	}
	if(nodeFound == NULL)
	{
		return 0;
	}
	else
	{
		if(nodeFound == q->front)
		{
			//retPid = nodeFound->pid;
			//(q->front)->next->prev = nodeFound->prev;
			(q->front) = (q->front)->next;
			//return 1;
		}
		if(nodeFound == q->rear)
		{
			//retPid = nodeFound->pid;
			//(q->rear)->prev->next = nodeFound->next;
			(q->rear) = (q->rear)->prev;
			//return 1;
		}
		if(nodeFound->next!=NULL)
		{
			nodeFound->next->prev = nodeFound->prev;
		}
		if(nodeFound->prev!=NULL)
		{
			nodeFound->prev->next = nodeFound->next;
		}
		return 1;
	}
	return 0;
}

int queuePush(queue* q, int pid)
{    
	Node *tempNode = (Node*) malloc(sizeof(Node));
	tempNode->pid = pid;
	tempNode->prev = NULL;
	tempNode->next = NULL;
	
	//if queue is empty
	if(q->front == NULL && q->rear == NULL)
	{
		q->front = tempNode;
		q->rear = tempNode;	
	}//if it contains one node
	else if(q->front == q->rear)
	{
		tempNode->prev = q->front;		
		q->rear = tempNode;
		q->front->next = q->rear;
	}//if it contains multiple
	else 
	{
		tempNode->prev = q->rear;
		q->rear->next = tempNode;
		q->rear = tempNode;		
	}
}

