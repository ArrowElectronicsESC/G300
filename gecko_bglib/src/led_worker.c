/*******************************************************************************
 * Copyright Arrow Electronics, Inc., 2019
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *******************************************************************************/

/*******************************************************************************
 *  Author: Andrew Reisdorph
 *  Date:   2019/07/21
 *******************************************************************************/

#include "led_worker.h"
#include "log.h"

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

static LedJobQueue _led_job_queue = {
    NULL, JOB_QUEUE_SIZE, JOB_QUEUE_SIZE, {}, PTHREAD_MUTEX_INITIALIZER};

static int new_led_job_ready() {
  int job_ready;
  pthread_mutex_lock(&_led_job_queue.mutex);
  job_ready = (_led_job_queue.start->job_type != LED_JOB_NONE);
  pthread_mutex_unlock(&_led_job_queue.mutex);
  return job_ready;
}

static LedJob get_next_led_job() {
  LedJob next_job = {0};
  LedJob *new_start_ptr;

  pthread_mutex_lock(&_led_job_queue.mutex);

  if (_led_job_queue.start != NULL &&
      _led_job_queue.start->job_type != LED_JOB_NONE) {
    next_job = *_led_job_queue.start;

    _led_job_queue.start->job_type = LED_JOB_NONE;

    if (_led_job_queue.start ==
        (&_led_job_queue.queue[_led_job_queue.capacity - 1])) {
      new_start_ptr = &_led_job_queue.queue[0];
    } else {
      new_start_ptr = _led_job_queue.start + 1;
    }

    if (new_start_ptr->job_type != LED_JOB_NONE) {
      _led_job_queue.start = new_start_ptr;
    }
  } else {
    next_job.job_type = LED_JOB_NONE;
  }

  log_trace("Popping LED Job:");
  log_trace("  Job Type: %d", next_job.job_type);

  pthread_mutex_unlock(&_led_job_queue.mutex);

  return next_job;
}

int push_led_job(LedJob job) {
  pthread_mutex_lock(&_led_job_queue.mutex);

  int result = 0;
  LedJob *job_insert_ptr = NULL;

  log_trace("Pushing LED Job:");
  log_trace("  Job Type: %d", job.job_type);
  log_trace("  Job Queue Start Ptr: %p", _led_job_queue.start);
  log_trace("  Job Queue[0] Address: %p", &_led_job_queue.queue[0]);

  if (_led_job_queue.start) {
    if (_led_job_queue.start->job_type == LED_JOB_NONE) {
      job_insert_ptr = _led_job_queue.start;
    } else {
      if (_led_job_queue.start ==
          (&_led_job_queue.queue[_led_job_queue.capacity - 1])) {
        if (_led_job_queue.queue[0].job_type == LED_JOB_NONE) {
          job_insert_ptr = &_led_job_queue.queue[0];
        }
      } else if ((_led_job_queue.start + 1)->job_type == LED_JOB_NONE) {
        job_insert_ptr = _led_job_queue.start + 1;
      }
    }
  } else {
    _led_job_queue.start = &_led_job_queue.queue[0];
    job_insert_ptr = _led_job_queue.start;
  }

  if (job_insert_ptr) {
    *job_insert_ptr = job;
  } else {
    log_warn("LED Job Queue Full!");
    result = -1;
  }

  pthread_mutex_unlock(&_led_job_queue.mutex);

  return result;
}

void *led_worker() {
  int keep_working = 1;
  LedJob job = {0};
  job.job_type = LED_JOB_NONE;
  int led_on = 0;

  while (keep_working) {
    if (new_led_job_ready()) {
      job = get_next_led_job();
    }

    switch (job.job_type) {
    case LED_JOB_OFF:
      system("usockc /var/gpio_ctrl WLAN_LED_OFF");
      break;
    case LED_JOB_ON:
      switch (job.colors[0]) {
      case LED_RED:
        system("usockc /var/gpio_ctrl WLAN_LED_RED");
        break;
      case LED_GREEN:
        system("usockc /var/gpio_ctrl WLAN_LED_GREEN");
        break;
      case LED_YELLOW:
        system("usockc /var/gpio_ctrl WLAN_LED_YELLOW");
        break;
      default:
        log_warn("Unsupported LED Color: %d", job.colors[0]);
        break;
      }
      job.job_type = LED_JOB_NONE;
      break;
    case LED_JOB_ON_OFF:
      if (led_on) {
        system("usockc /var/gpio_ctrl WLAN_LED_OFF");
        led_on = 0;
      } else {
        switch (job.colors[0]) {
        case LED_RED:
          system("usockc /var/gpio_ctrl WLAN_LED_RED");
          break;
        case LED_GREEN:
          system("usockc /var/gpio_ctrl WLAN_LED_GREEN");
          break;
        case LED_YELLOW:
          system("usockc /var/gpio_ctrl WLAN_LED_YELLOW");
          break;
        default:
          log_warn("Unsupported LED Color: %d", job.colors[0]);
          break;
        }
        led_on = 1;
      }
      usleep(job.time * 1000);
      break;
    case LED_JOB_ALTERNATE:
      for (int color_index = 0; color_index < job.num_colors; color_index++) {
        switch (job.colors[color_index]) {
        case LED_RED:
          system("usockc /var/gpio_ctrl WLAN_LED_RED");
          break;
        case LED_GREEN:
          system("usockc /var/gpio_ctrl WLAN_LED_GREEN");
          break;
        case LED_YELLOW:
          system("usockc /var/gpio_ctrl WLAN_LED_YELLOW");
          break;
        default:
          log_warn("Unsupported LED Color: %d", job.colors[0]);
          break;
        }
        usleep(job.time * 1000);
      }
      break;
    case LED_JOB_QUIT:
      keep_working = 0;
      break;
    case LED_JOB_NONE:
      // Sleep half second
      usleep(500000);
      break;
    default:
      log_warn("Unhandled LED Job Type: %d", job.job_type);
      break;
    }
  }

  return NULL;
}