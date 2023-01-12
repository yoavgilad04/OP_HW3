#ifndef WEBSERVER_FILES_QUEUE_C
#define WEBSERVER_FILES_QUEUE_C

#include "queue.h"
#include <stdlib.h>
#include <sys/time.h>

Node createNode(int value, struct timeval arrival){
    Node temp = (Node)malloc(sizeof(*temp)); // allocate memory using malloc()
    temp->data = value;
    temp->arrival_time = arrival;
    temp->next = NULL;// make next point to NULL
    return temp;//return the new node
}

void deleteNode(Node n){
    free(n);
}

Queue createQueue(int max_size)
{
    Queue q = (Queue)malloc(sizeof(*q));
    q->current_size = 0;
    q->head = NULL;
    q->max_size = max_size;
    return q;
}

Node popQueue(Queue requests){
    Node p = NULL;
    Node new_head = NULL;
    if(requests->head == NULL){
        return NULL;
    }
    else {
        p = requests->head;
        new_head = p->next;
        requests->head = new_head;
        requests->current_size--;
        return p;
    }
}

void deleteQueue(Queue q)
{
    Node n = NULL;
    while(q->head != NULL)
    {
        n = popQueue(q);
        deleteNode(n);
    }
    free(q);
}

void pushQueue(Queue requests, int value, struct timeval arrival){
    Node temp = createNode(value, arrival);
    Node p = NULL;
    if(requests->head == NULL){
        requests->head = temp;     //when linked list is empty
    }
    else {
        p = requests->head;//assign head to p
        while (p->next != NULL) {
            p = p->next;//traverse the list until p is the last node.The last node always points to NULL.
        }
        p->next = temp;//Point the previous last node to the new node created.
    }
    requests->current_size++;
}


Node PopByPosition(Queue requests, int position)
{
    int count = 0;
    Node pre = NULL;
    Node p = NULL;
    if (requests->current_size < position)
        return NULL;
    if(requests->head == NULL){
        return NULL;    //when linked list is empty
    }
    else {
        p = requests->head;//assign head to p
        if (position == 0)
        {
            p = popQueue(requests);
            return p;
        }
        while (p->next != NULL) {
            count++;
            pre = p;
            p = p->next;//traverse the list until p is the last node.The last node always points to NULL.
            if(count == position) {
                pre->next = p->next;
                requests->current_size--;
                return p;
            }
        }
        return NULL;
    }
}

int deleteByValue(Queue requests, int value)
{
    Node pre = NULL;
    Node p = NULL;
    if(requests->head == NULL){
        return -1;    //when linked list is empty
    }
    else {
        p = requests->head;//assign head to p
        if (p->data == value)
        {
            requests->head = p->next;
            deleteNode(p);
            requests->current_size--;
            return 1;
        }
        while (p->next != NULL) {
            pre = p;
            p = p->next;//traverse the list until p is the last node.The last node always points to NULL.
            if(p->data == value) {
                pre->next = p->next;
                deleteNode(p);
                requests->current_size--;
                return 1;
            }
        }
        return -1;
    }
}

void printQueue(Queue requests)
{
    Node p = requests->head;
    while(p != NULL)
    {
        printf("%d", p->data);
        printf("->");
        p = p->next;
    }
    printf("NULL\n");
}
#endif