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
    *max_queue_size = atoi(argv[3]);
    *schedalg = argv[4];
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
        int request = popQueue(q_waiting)->data;
        pushQueue(q_handled, request);
        pthread_mutex_unlock(&m_queues_size);

        requestHandle(request);
        Close(request);

        pthread_mutex_lock(&m_queues_size);
        deleteByValue(q_handled, request);
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



int main(int argc, char *argv[])
{
    int listenfd, connfd, clientlen;
    struct sockaddr_in clientaddr;
    int port, num_of_threads, max_requests_size;
    char schedalg[MAX_ALG];

    getargs(&port,&num_of_threads, &max_requests_size, schedalg, argc, argv);
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
    while(q_waiting->current_size + q_handled->current_size == max_requests_size){
        if(q_waiting->current_size == 0){
            Close(connfd);
            pthread_cond_wait(&cond_full, &m_queues_size);
        }
        else{
            //Todo: Throwing waited requests algorithm
            continue;
        }
    }
    pushQueue(q_waiting, connfd);
    pthread_cond_signal(&cond_empty);
    pthread_mutex_unlock(&m_queues_size);

	//
	// HW3: In general, don't handle the request in the main thread.
	// Save the relevant info in a buffer and have one of the worker threads 
	// do the work. 
	//
     }
    deleteQueue(q_waiting);
    deleteQueue(q_handled);
    free(pool);
//    for(int i=0 ; i < num_of_threads; i++)
//        pthread_cancel(pool[i]);
}


    


 
