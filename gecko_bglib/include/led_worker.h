/*******************************************************************************
 * Copyright Arrow Electronics, Inc., 2019
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

/*******************************************************************************
 *  Author: Andrew Reisdorph
 *  Date:   2019/07/21
 ******************************************************************************/

#ifndef __INCLUDE_LED_WORKER_H
#define __INCLUDE_LED_WORKER_H

#include <pthread.h>

#define JOB_QUEUE_SIZE 10

typedef enum { LED_RED, LED_GREEN, LED_YELLOW } LedColor;

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
  int num_colors;
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