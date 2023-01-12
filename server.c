#include "segel.h"
#include "request.h"
#include "queue.h"
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

typedef struct routine_args {
    Queue waiting;
    Queue handling;
    int i;
} routine_args;

//int handled_count = 0;
//int waiting_count = 0;


void getargs(int *port, int *num_of_threads, int *max_queue_size, char *policy, int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s <port> <threads> <queues_size> <schedalg>\n", argv[0]);
        exit(1);
    }
    *port = atoi(argv[1]);
    *num_of_threads = atoi(argv[2]);
    *max_queue_size = atoi(argv[3]);
    strcpy(policy, argv[4]);
}


pthread_cond_t cond_not_full; //The main thread will wait in this cond in case number of request are maximize
pthread_cond_t cond_not_empty; //Worker threads will wait in this cond in case the number of request is zero
pthread_mutex_t m_waiting;
pthread_mutex_t m_handled;


void init_cond_and_locks() {
    pthread_mutex_init(&m_waiting, NULL);
    //pthread_mutex_init(&m_handled, NULL);

    pthread_cond_init(&cond_not_full, NULL);
    pthread_cond_init(&cond_not_empty, NULL);
    return;
}

void *thread_routine(struct routine_args *args) {
    Stats stats;
    stats.stat_thread.count = 0;
    stats.stat_thread.count_static = 0;
    stats.stat_thread.count_dyn = 0;
    stats.stat_thread.thread_id = args->i;
    Queue q_waiting = args->waiting;
    Queue q_handled = args->handling;
    while (1) {
        //lock and wait for signal if queue empty
        pthread_mutex_lock(&m_waiting);
        while (q_waiting->current_size == 0) {

            pthread_cond_wait(&cond_not_empty, &m_waiting);
        }
        Node request = popQueue(q_waiting);
        int connfd = request->data;
        stats.arrival_time = request->arrival_time;
        //pthread_mutex_unlock(&m_waiting);

        //pthread_mutex_lock(&m_handled);
        pushQueue(q_handled, connfd, stats.arrival_time);

        //update handle time
        struct timeval handle;
        gettimeofday(&handle, NULL);
        stats.handled_time = handle;

        pthread_mutex_unlock(&m_waiting);

        timersub(&stats.handled_time, &stats.arrival_time, &stats.dispatch_time);

        requestHandle(connfd, &stats);
        Close(connfd);

        pthread_mutex_lock(&m_waiting);
        deleteByValue(q_handled, connfd);
        pthread_cond_signal(&cond_not_full);
        pthread_mutex_unlock(&m_waiting);
    }
}

routine_args *createArgs(int index, Queue q_waiting, Queue q_handled) {
    routine_args *args = (routine_args *) malloc(sizeof(*args));
    args->waiting = q_waiting;
    args->handling = q_handled;
    args->i = index;
    return args;
}

pthread_t *createPool(int num_of_threads, Queue q_waiting, Queue q_handled) {
    pthread_t *pool = (pthread_t *) malloc(sizeof(*pool) * num_of_threads);
    //routine_args *args;
    for (int i = 0; i < num_of_threads; i++) {
        routine_args *args = createArgs(i, q_waiting, q_handled);
        pthread_create(pool + i, NULL, (void *) thread_routine, (void *) (args));
    }
    return pool;
}

void deleteRandHalf(Queue q) {
    Node request;
    if (q->current_size == 1) {
        request = popQueue(q);
        Close(request->data);
        deleteNode(request);
        return;
    }
    int half_size = ceil(q->current_size / 2);
    int r;
    for (int i = 0; i < half_size && q->current_size > 0; i++) {
        r = rand() % q->current_size;
        request = PopByPosition(q, r);
        Close(request->data);
        deleteNode(request);
    }
}


int main(int argc, char *argv[]) {
//    printf("starting server\n");
    int listenfd, connfd, clientlen;
    struct sockaddr_in clientaddr;
    int port, num_of_threads, max_requests_size;
    char policy[MAX_ALG];

    getargs(&port, &num_of_threads, &max_requests_size, policy, argc, argv);

    // create locks and cond_vars
    init_cond_and_locks();

    // Creating queues
    Queue q_waiting = createQueue(max_requests_size);
    Queue q_handled = createQueue(max_requests_size);


    // Create pool threads
    pthread_t *pool = createPool(num_of_threads, q_waiting, q_handled);

    listenfd = Open_listenfd(port);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *) &clientaddr, (socklen_t * ) & clientlen);
        struct timeval arrival_time;
        gettimeofday(&arrival_time, NULL);

        pthread_mutex_lock(&m_waiting);
        if (q_waiting->current_size + q_handled->current_size >= max_requests_size) {

            // is_block -> stop getting requests and once there is a spot for the request push the new request
            if (strcmp(policy, "block") == 0) {
                while (q_waiting->current_size + q_handled->current_size >= max_requests_size) {
                    pthread_cond_wait(&cond_not_full, &m_waiting);
                }
            }
                // drop_tail -> should close the fd, and continue to the next iteration of the while(1)
            else if (strcmp(policy, "dt") == 0) {
                Close(connfd);
                pthread_cond_signal(&cond_not_empty);
                pthread_mutex_unlock(&m_waiting);
                continue;
            }
                // drop_head -> execute q_waiting.pop and q_wating.push(new_request)
            else if (strcmp(policy, "dh") == 0) {
                if (q_waiting->current_size == 0) {
                    Close(connfd);
                    pthread_cond_signal(&cond_not_empty);
                    pthread_mutex_unlock(&m_waiting);
                    continue;
                } else {
                    Node request = popQueue(q_waiting);
                    //waiting_count--;
                    Close(request->data);
                    //deleteNode(request);
                }
            }
                // drop_random -> q_waiting.deleteRand()
            else if (strcmp(policy, "random") == 0) {
                if (q_waiting->current_size == 0) {
                    Close(connfd);
                    pthread_mutex_unlock(&m_waiting);
                    continue;
                } else {
                    deleteRandHalf(q_waiting);
                }
            }
        }

        pushQueue(q_waiting, connfd, arrival_time);
        pthread_cond_signal(&cond_not_empty);
        pthread_mutex_unlock(&m_waiting);

    }
    free(pool);
/*
    deleteQueue(q_waiting);
    deleteQueue(q_handled);

    pthread_mutex_destroy(&m_waiting);
    pthread_mutex_destroy(&m_handled);

    pthread_cond_destroy(&cond_not_empty);
    pthread_cond_destroy(&cond_not_full);*/
}

