#ifndef __NEMOUI_THREAD_H__
#define __NEMOUI_THREAD_H__

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *(*ThreadCallback)(void *userdata);
typedef void (*ThreadDoneCallback)(bool canceled, void *userdata);
typedef struct _Thread Thread;

Thread *thread_create(struct nemotool *tool, ThreadCallback callback, ThreadDoneCallback callback_done, void *userdata);
void thread_destroy(Thread *thread);

typedef void (*WorkCallback)(void *userdata);
typedef void (*WorkDoneCallback)(bool cancel, void *userdata);

typedef struct _Work Work;
typedef struct _Worker Worker;
void worker_stop(Worker *worker);
void worker_destroy(Worker *worker);
void worker_start(Worker *worker);
Work *worker_append_work(Worker *worker, WorkCallback doo, WorkDoneCallback done, void *userdata);
Worker *worker_create(struct nemotool *tool);

#ifdef __cplusplus
}
#endif

#endif
