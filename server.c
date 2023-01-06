#include "segel.h"
#include "request.h"
#include "list.h"
//
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

#define MAX_ALG 7
// HW3: Parse the new arguments too
void getargs(int *port, int *num_of_threads, int *max_queue_size, char* schedalg, int argc, char *argv[])
{
    if (argc < 2) {
	fprintf(stderr, "Usage: %s <port>\n", argv[0]);
	exit(1);
    }
    *port = atoi(argv[1]);
    *num_of_threads = atoi(argv[2]);
    *max_queue_size = atio(argv[3]);
    *schedalg = argv[4];
}

void* thread_routine(Queue q_waiting, Queue q_handled)
{
    // check if max size, push_to_handled, pop_waiting
    return;
}

p_thread* createPool(num_of_threads, Queue q_waiting, Queue q_handled)
{
    p_thread *pool = (p_thread*)malloc(sizeof(*pool)*num_of_threads);
    for(int i=0; i<num_of_threads; i++)
    {
        pthread_create(pool+i, NULL, thread_routine, (void*)q_waiting, (void*)q_handled)
    }
}
int main(int argc, char *argv[])
{
    int listenfd, connfd, clientlen;
    struct sockaddr_in clientaddr;
    int port, num_of_threads, max_queue_size;
    char schedalg[MAX_ALG];

    getargs(&port,&num_of_threads, &max_queue_size, schedalg, argc, argv);
    // Creating queues
    Queue q_waiting = createQueue(max_queue_size);
    Queue q_handled = createQueue(max_queue_size);

    // Create pool threads


    listenfd = Open_listenfd(port);
    while (1) {
	clientlen = sizeof(clientaddr);
	connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);

	// 
	// HW3: In general, don't handle the request in the main thread.
	// Save the relevant info in a buffer and have one of the worker threads 
	// do the work. 
	// 
	requestHandle(connfd);

	Close(connfd);
    }

}


    


 
