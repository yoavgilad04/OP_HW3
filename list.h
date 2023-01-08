//
// Created by yoavg on 1/6/2023.
//

#ifndef WEBSERVER_FILES_LIST_H
#define WEBSERVER_FILES_LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>


typedef struct node* Node; //Define node as pointer of data type struct LinkedList

struct node{
    int data;
    struct timeval arrival_time;
    struct timeval handle_time;
    Node next;
};

struct request_list{
    Node head;
    int max_size;
    int current_size;
};

typedef struct request_list* Queue; //Define node as pointer of data type struct LinkedList

Node createNode(int value, struct timeval arrival, struct timeval handle){
    Node temp; // declare a node
    temp = (Node)malloc(sizeof(struct node)); // allocate memory using malloc()
    temp->data = value;
    temp->arrival_time = arrival;
    temp->handle_time = handle;
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

void pushQueue(Queue requests, int value, struct timeval arrival, struct timeval handle){
    Node temp = createNode(value, arrival, handle);
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
    if(requests->head == NULL){
        return NULL;    //when linked list is empty
    }
    else {
        p = requests->head;//assign head to p
        if (count == position)
        {
            p = popQueue(requests);
            requests->current_size--;
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
            deleteNode(popQueue(requests));
            requests->current_size--;
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

#endif //WEBSERVER_FILES_LIST_H
