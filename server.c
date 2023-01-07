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

/**
 * Conditions:
 * 1) The main thread waits if the number of request is full
 * 2) The threads wait if the number of request is zero
 * 3) If one thread change queue sizes then other threads need to wait for it to finish.
 * 4) If there are no requests in the waiting queue all threads most wait for a request
 */
pthread_cond_t cond_full; //The main thread will wait in this cond in case number of request are maximize
pthread_cond_t cond_empty; //Worker threads will wait in this cond in case the number of request is zero

pthread_mutex_t m_queues_size;


/**
 * נעילת mutex:
int pthread_mutex_lock(pthread_mutex_t *mutex);
הפעולה חוסמת עד שה-mutex מתפנה ואז נועלת אותו.
ניסיון לנעילת mutex:
int pthread_mutex_trylock(pthread_mutex_t *mutex);
הפעולה נכשלת אם ה-mutex כבר נעול, אחרת נועלת אותו.
שחרור mutex נעול:
int pthread_mutex_unlock(pthread_mutex_t *mutex);
ניסיון לשחרר מנעול שאינו נעול תביא להתנהגות לא מוגדרת.
פינוי mutex בתום השימוש:
int pthread_mutex_destroy(pthread_mutex_t *mutex);
הפעולה נכשלת אם ה-mutex מאותחל אבל נעול.
 */

/**
 * int pthread_cond_signal(pthread_cond_t *cond);
משחררת את אחד החוטים הממתינים (הגינות לא מובטחת).

 int pthread_cond_wait(pthread_cond_t *cond,
		pthread_mutex_t *mutex);
פעולה:
משחררת את המנעול ומעבירה את החוט להמתין על משתנה התנאי באופן אטומי (ראינו קודם מדוע זה הכרחי).
החוט הממתין חייב להחזיק במנעול mutex לפני הקריאה.
בחזרה מהמתנה על משתנה התנאי, החוט עובר להמתין על המנעול. החוט יחזור מהקריאה ל-pthread_cond_wait() רק לאחר שינעל מחדש את ה-mutex.

ערך מוחזר: הפעולה תמיד מצליחה ומחזירה 0.

 int pthread_cond_broadcast(pthread_cond_t *cond);
משחררת את כל החוטים הממתינים.
כל החוטים מפסיקים להמתין על משתנה התנאי ועוברים להמתין על המנעול. החוטים יחזרו לפעילות בזה אחר זה (בסדר כלשהו, לאו דווקא הוגן) לאחר שינעלו מחדש את ה-mutex.


 int pthread_cond_init(pthread_cond_t *cond,
		pthread_condattr_t *cond_attr);
ערך מוחזר: הפעולה תמיד מצליחה ומחזירה 0.

int pthread_cond_destroy(pthread_cond_t *cond);
ערך מוחזר: 0 בהצלחה, ערך שונה מ-0 בכישלון (למשל, אם יש עדיין חוטים הממתינים על משתנה התנאי).

 */

void init_cond_and_locks(){
    pthread_mutex_init(&m_queues_size, NULL);
    pthread_cond_init(&cond_full, NULL);
    pthread_cond_init(&cond_empty, NULL);
}

void* thread_routine(Queue* q_arr) {
    while(1){
        Queue q_waiting = q_arr[0];
        Queue q_handled = q_arr[1];
        pthread_mutex_lock(&m_queues_size);

        while (q_waiting->current_size == 0) {
            pthread_cond_wait(&cond_empty, &m_queues_size);
        }
        Node request = popQueue(q_waiting);
        int request_num = request->data;
        struct timeval arrival = request->arrival_time;
        struct timeval handle;
        gettimeofday(&handle, NULL);
        pushQueue(q_handled, request_num, arrival, handle);
        pthread_mutex_unlock(&m_queues_size);

        requestHandle(request, arrival, handle);
        Close(request_num);

        pthread_mutex_lock(&m_queues_size);
        deleteByValue(q_handled, request_num);
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


    


 
