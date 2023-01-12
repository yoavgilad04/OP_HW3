#ifndef WEBSERVER_FILES_QUEUE_H
#define WEBSERVER_FILES_QUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>


struct node{
    int data;
    struct timeval arrival_time;
    struct node* next;
};

struct request_list{
    struct node* head;
    int max_size;
    int current_size;
};

typedef struct request_list* Queue; //Define node as pointer of data type struct LinkedList
typedef struct node* Node; //Define node as pointer of data type struct LinkedList

Node createNode(int value, struct timeval arrival);
void deleteNode(Node n);
Queue createQueue(int max_size);
Node popQueue(Queue requests);
void deleteQueue(Queue q);
void pushQueue(Queue requests, int value, struct timeval arrival);
Node PopByPosition(Queue requests, int position);
int deleteByValue(Queue requests, int value);
void printQueue(Queue requests);

#endif //WEBSERVER_FILES_LIST_H