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

#define true 1
#define false 0
#define MAX_ALG 7
// HW3: Parse the new arguments too

void getargs(int *port, int *num_of_threads, int *max_queue_size, char* policy, int argc, char *argv[])
{
    if (argc < 2) {
	fprintf(stderr, "Usage: %s <port>\n", argv[0]);
	exit(1);
    }
    if (argc >= 2)
        *port = atoi(argv[1]);
    if (argc >= 3)
        *num_of_threads = atoi(argv[2]);
    if (argc >= 4)
        *max_queue_size = atoi(argv[3]);
    if (argc >= 5)
        strcpy(policy, argv[4]);
}


pthread_cond_t cond_full; //The main thread will wait in this cond in case number of request are maximize
pthread_cond_t cond_empty; //Worker threads will wait in this cond in case the number of request is zero

pthread_mutex_t m_queues_size;


void init_cond_and_locks(){
    pthread_mutex_init(&m_queues_size, NULL);
    pthread_cond_init(&cond_full, NULL);
    pthread_cond_init(&cond_empty, NULL);
}

void* thread_routine(Queue* q_arr) {
    Stats stats;
    stats.stat_thread.count = 0;
    stats.stat_thread.count_static = 0;
    stats.stat_thread.count_dyn = 0;

    stats.stat_thread.thread_id = pthread_self();
    while(1){
        Queue q_waiting = q_arr[0];
        Queue q_handled = q_arr[1];
        pthread_mutex_lock(&m_queues_size);

        while (q_waiting->current_size == 0) {
            pthread_cond_wait(&cond_empty, &m_queues_size);
        }
        Node request = popQueue(q_waiting);
        if (request == NULL)
            continue;
        int connfd = request->data;
        stats.arrival_time = request->arrival_time;
        struct timeval handle;
        gettimeofday(&handle, NULL);
        stats.handled_time = handle;
        pushQueue(q_handled, connfd, stats.arrival_time, stats.handled_time);
        stats.stat_thread.count++;
        pthread_mutex_unlock(&m_queues_size);

        requestHandle(connfd, stats);
        Close(connfd);

        pthread_mutex_lock(&m_queues_size);
        deleteByValue(q_handled, connfd);
        pthread_mutex_unlock(&m_queues_size);
        pthread_cond_signal(&cond_full);
    }

}

pthread_t* createPool(int num_of_threads, Queue* q_arr)
{
    pthread_t *pool = (pthread_t*)malloc(sizeof(*pool)*num_of_threads);
    for(int i=0; i<num_of_threads; i++)
    {
        pthread_create(pool+i, NULL, thread_routine, (void*)q_arr);
    }
    return pool;
}

void deleteRandHalf(Queue requests) {
    int half_size = ceil(requests->current_size / 2);
    int r;
    Node request;
    for (int i = 0; i < half_size; i++) {
        r = rand() % requests->current_size;
        request = PopByPosition(requests, r);
        Close(request->data);
        deleteNode(request);
    }
}


int main(int argc, char *argv[])
{
    int listenfd, connfd, clientlen;
    struct sockaddr_in clientaddr;
    int port, num_of_threads, max_requests_size;
    char policy[MAX_ALG];

    getargs(&port,&num_of_threads, &max_requests_size, policy, argc, argv);
    // Creating queues
    Queue q_waiting = createQueue(max_requests_size);
    Queue q_handled = createQueue(max_requests_size);
    Queue q_arr[] = {q_waiting, q_handled};
    // Create pool threads
    pthread_t* pool = createPool(num_of_threads, q_arr);

    // create locks and cond_vars
    init_cond_and_locks();
    listenfd = Open_listenfd(port);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        pthread_mutex_lock(&m_queues_size);
        struct timeval arrival_time;
        gettimeofday(&arrival_time, NULL);
        if(q_waiting->current_size + q_handled->current_size == max_requests_size){
            if(q_waiting->current_size == 0){
                Close(connfd);
                pthread_mutex_unlock(&m_queues_size);
                continue;
            }
            else{
                //Todo: Throwing waited requests algorithm
                // is_block -> stop getting requests and once there is a spot for the request push the new request
                if (strcmp(policy,  'block'))
                {
                    while(q_waiting->current_size + q_handled->current_size == max_requests_size)
                        pthread_cond_wait(&cond_full, &m_queues_size);
                }
                    // drop_tail -> should close the fd, and continue to the next iteration of the while(1)
                else if (strcmp(policy,  'dt'))
                {
                    Close(connfd);
                    continue;
                }
                    // drop_head -> execute q_waiting.pop and q_wating.push(new_request)
                else if (strcmp(policy,  'dh'))
                {
                    Node request = popQueue(q_waiting);
                    Close(request->data);
                    deleteNode(request);
                }
                    // drop_random -> q_waiting.deleteRand()
                else if (strcmp(policy,  'random'))
                {
                    deleteRandHalf(q_waiting);
                }
            }
        }
        pushQueue(q_waiting, connfd, arrival_time, arrival_time);
        pthread_cond_signal(&cond_empty);
        pthread_mutex_unlock(&m_queues_size);
     }
    deleteQueue(q_waiting);
    deleteQueue(q_handled);
    free(pool);
}


    


 
