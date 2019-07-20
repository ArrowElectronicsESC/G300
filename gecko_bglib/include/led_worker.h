#ifndef __INCLUDE_LED_WORKER_H
#define __INCLUDE_LED_WORKER_H

#include <pthread.h>


#define JOB_QUEUE_SIZE 10

typedef enum {
    LED_RED,
    LED_GREEN,
    LED_YELLOW
} LedColor;

typedef enum LedJobType {
    LED_JOB_NONE,
    LED_JOB_OFF,
    LED_JOB_ON,
    LED_JOB_ON_OFF,
    LED_JOB_ALTERNATE,
    LED_JOB_QUIT
} LedJobType;

typedef struct LedJob {
    LedJobType job_type;
    int time;
    LedColor colors[3];
    int  num_colors;
} LedJob;

typedef struct LedJobQueue {
    LedJob *start;
    int size;
    int capacity;
    LedJob queue[JOB_QUEUE_SIZE];
    pthread_mutex_t mutex;
} LedJobQueue;

void *led_worker();
int push_led_job(LedJob job);

#endif //  __INCLUDE_LED_WORKER_H