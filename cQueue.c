#include "cQueue.h"

void enqueue(cQueue *q, customer_info cInfo, double entry){

    Node* new_node = (Node*) malloc(sizeof(Node));
    if(new_node == NULL){
        perror("Malloc failed");
        return;
    }

    new_node->next = NULL;
    new_node->info = cInfo;
    new_node->entry_time = entry;

    //if q is empty
    if(q->tail == NULL){
        q->head = new_node;
        q->tail = new_node;
        (q->size)++;
        return;
    }
    //else add to end of queue
    q->tail->next = new_node;
    q->tail = new_node;
    (q->size)++;

}

Node* dequeue(cQueue *q) {

    //store current head node and move head pointer to one node ahead
    Node *curr = q->head;
    q->head = q->head->next;

    //if queue is now empty, change tail to null also
    if(q->head == NULL){
        q->tail = NULL;
    }

    (q->size)--;
    return curr;

}