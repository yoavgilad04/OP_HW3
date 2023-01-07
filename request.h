#ifndef __REQUEST_H__

typedef struct stats Stats;
void requestHandle(int fd, Stats stats);

struct stats{
    struct timeval arrival_time;
    struct timeval handled_time;
    struct stats_thread thread;
};

struct stats_thread{
    int thread_id;
    int count;
    int count_static;
    int count_dyn;
};


#endif
