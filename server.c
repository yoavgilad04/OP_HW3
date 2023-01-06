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

/**
 * Conditions:
 * 1) The main thread waits if the number of request is full
 * 2) The threads wait if the number of request is zero
 * 3) If one thread change queue sizes then other threads need to wait for it to finish.
 * 4) If there are no requests in the waiting queue all threads most wait for a request
 * 5)
 */
pthread_cond_t cond_full; //The main thread will wait in this cond in case number of request are maximize
pthread_cond_t cond_empty; //Worker threads will wait in this cond in case the number of request is zero
pthread_cond_t cond_read_size; //Worker threads that want to read the size of the queues will wait if a thread changes the size
pthread_cond_t cond_write_size; //Worker threads that want to change the size will wait in case a thread reads the size


p_thread_mutex_lock m_reader_inside;
p_thread_mutex_lock m_writer_inside;
p_thread_mutex_lock m_full;
p_thread_mutex_lock m_empty;


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

 */
void* thread_routine(Queue q_arr)
{

    // check if max size, push_to_handled, pop_waiting
    return;
}

p_thread* createPool(num_of_threads, Queue q_arr)
{
    p_thread *pool = (p_thread*)malloc(sizeof(*pool)*num_of_threads);
    for(int i=0; i<num_of_threads; i++)
    {
        pthread_create(pool+i, NULL, thread_routine, (void*)q_arr);
    }
    return pool;
}

void reader_lock() {
    mutex_lock(&global_lock);
    while (writers_inside > 0)
        cond_wait(&read_allowed, &global_lock);
    readers_inside++;
    mutex_unlock(&global_lock);
}

void reader_unlock() {
    mutex_lock(&global_lock);
    readers_inside--;
    if (readers_inside == 0)
        cond_signal(&write_allowed);
    mutex_unlock(&global_lock);
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
    Queue q_arr[] = {q_waiting, q_handled};
    // Create pool threads
    p_thread* pool = createPool(num_of_threads, q_arr);

    // create locks and cond_vars
    p_thread_mutex_lock m_waiting;
    p_thread_mutex_lock m_handled;
    pthread_mutex_init(&m_wating, NULL);
    pthread_mutex_init(&m_handled, NULL);



    pthread_cond_init(&cond1, NULL);
    pthread_cond_init(&cond2, NULL);


//    pthread_cond_init(&cond1, condition);
/**enquea:
 *  while(waitinh_size + handling_size == max_size)
        cond_full a

 * dque:
 while(waiting_size == 0 )
        cond_empty
 size--;
 signal(cond_full)
 * *//

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


    


 
