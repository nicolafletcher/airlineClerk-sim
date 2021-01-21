#ifndef CQUEUE_H

#define CQUEUE_H

#include "customer_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

typedef struct Node{

    customer_info info;
    double entry_time;
    struct Node* next;

} Node;

typedef struct cQueue{
    Node *head;
    Node *tail;
    int size;
} cQueue;

void enqueue(cQueue *q, customer_info cInfo, double entry);
Node* dequeue(cQueue *q);


#endif //"CQUEUE_H"