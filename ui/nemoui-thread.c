#include <stdlib.h>
#include <nemotool.h>
#include <nemotimer.h>

#include "nemowrapper.h"
#include "nemoui-thread.h"

struct _Thread {
    bool canceled;
    bool done;
    struct nemotool *tool;
    struct nemotimer *timer;
    char *id;
    pthread_t pth;
    ThreadCallback callback;
    ThreadDoneCallback callback_done;
    void *userdata;
};

static void *_thread(void *userdata)
{
    Thread *thread = (Thread *)userdata;
    void *ret = NULL;

    if (!thread->canceled)
        ret = thread->callback(thread->userdata);
    thread->done = true;
    return ret;
}

static void _thread_timer(struct nemotimer *timer, void *userdata)
{
    Thread *thread = (Thread *)userdata;
    if (!thread->done) {
        nemotimer_set_timeout(thread->timer, 100);
        return;
    }

    thread->callback_done(thread->canceled, thread->userdata);
    nemotimer_destroy(thread->timer);
    free(thread->id);
    free(thread);
}

// Should be called main thread
Thread *thread_create(struct nemotool *tool, ThreadCallback callback, ThreadDoneCallback callback_done, void *userdata)
{
    Thread *thread = (Thread *)calloc(sizeof(Thread), 1);
    thread->tool = tool;
    thread->callback = callback;
    thread->callback_done = callback_done;
    thread->userdata = userdata;
    thread->id = strdup_printf("thread:%d", get_time());

    thread->timer = TOOL_ADD_TIMER(thread->tool, 100, _thread_timer, thread);
    if (!thread->timer) {
        ERR("nemotool dispatch idle failed");
        free(thread->id);
        free(thread);
        return NULL;
    }

    pthread_attr_t attr;
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_init(&attr);

    if (pthread_create(&(thread->pth), &attr, _thread, thread) != 0) {
        ERR("pthread create failed: %s", strerror(errno));
        free(thread->id);
        free(thread);
        return NULL;
    }

    return thread;
}

// Should be called main thread
void thread_destroy(Thread *thread)
{
    thread->canceled = true;
#if 0
    // XXX: pthread cancel cause wrong behavior because anything can be aborted.
    if (pthread_cancel(thread->pth) != 0) {
        ERR("pthread cancel failed: %s", strerror(errno));
    }
    free(thread->id);
    free(thread);
#endif
}

struct _Worker {
    bool canceled;
    bool done;
    struct nemotool *tool;
    struct nemotimer *timer;
    char *id;
    Thread *thread;
    List *works;
    List *work_dones;
};

struct _Work {
    bool canceled;
    Worker *worker;
    WorkCallback doo;
    WorkDoneCallback done;
    void *userdata;
};

// Should be called main thread
void worker_stop(Worker *worker)
{
    nemotimer_set_timeout(worker->timer, 0);
    List *l;
    Work *work;
    pthread_mutex_lock(&__worker_mutex);
    LIST_FOR_EACH(worker->works, l, work) {
        work->canceled = true;
    }
    LIST_FOR_EACH(worker->work_dones, l, work) {
        work->canceled = true;
    }
    pthread_mutex_unlock(&__worker_mutex);

}

// Should be called main thread
void worker_destroy(Worker *worker)
{
    worker_stop(worker);
    if (worker->thread) thread_destroy(worker->thread);
}

static void *_worker_do(void *userdata)
{
    RET_IF(!userdata, NULL);
    ERR("worker");

    Worker *worker = (Worker *)userdata;

    Work *work;
    pthread_mutex_lock(&__worker_mutex);
    while ((work = (Work *)LIST_DATA(LIST_FIRST(worker->works)))) {
        pthread_mutex_unlock(&__worker_mutex);
        if (!work->canceled)
            work->doo(work->userdata);

        pthread_mutex_lock(&__worker_mutex);
        worker->work_dones = list_append(worker->work_dones, work);
        worker->works = list_remove(worker->works, work);
    }
    worker->done = true;
    pthread_mutex_unlock(&__worker_mutex);
    return NULL;
}

static void _worker_done(bool canceled, void *userdata)
{
    Worker *worker = (Worker *)userdata;
    worker->thread = NULL;

    worker->canceled = canceled;
}

static void _worker_timer(struct nemotimer *timer, void *userdata)
{
    Worker *worker = (Worker *)userdata;

    Work *work;
    pthread_mutex_lock(&__worker_mutex);
    while ((work = (Work *)LIST_DATA(LIST_FIRST(worker->work_dones)))) {
        pthread_mutex_unlock(&__worker_mutex);
        work->done(work->canceled, work->userdata);
        free(work);
        pthread_mutex_lock(&__worker_mutex);
        worker->work_dones = list_remove(worker->work_dones, work);
    }
    pthread_mutex_unlock(&__worker_mutex);

    if (worker->done) {
        ERR("All work is done");
        nemotimer_set_timeout(worker->timer, 0);
        worker->done = false;
    } else {
        nemotimer_set_timeout(worker->timer, 100);
    }
    if (worker->canceled) {
        ERR("All work is canceled");
        nemotimer_destroy(worker->timer);
        free(worker);
    }
}

// Should be called main thread
void worker_start(Worker *worker)
{
    nemotimer_set_timeout(worker->timer, 100);
    if (worker->thread) {
        return;
    }
    pthread_mutex_lock(&__worker_mutex);
    if (list_count(worker->works) <= 0) {
        pthread_mutex_unlock(&__worker_mutex);
        return;
    }
    pthread_mutex_unlock(&__worker_mutex);

    worker->thread = thread_create(worker->tool, _worker_do, _worker_done, worker);
}

// Should be called main thread
Work *worker_append_work(Worker *worker, WorkCallback doo, WorkDoneCallback done, void *userdata)
{
    Work *work;
    work = (Work *)calloc(sizeof(Work), 1);
    work->worker = worker;
    work->doo = doo;
    work->done = done;
    work->userdata = userdata;
    pthread_mutex_lock(&__worker_mutex);
    worker->works = list_append(worker->works, work);
    pthread_mutex_unlock(&__worker_mutex);

    return work;
}

// Should be called main thread
Worker *worker_create(struct nemotool *tool)
{
    Worker *worker = (Worker *)calloc(sizeof(Worker), 1);
    worker->tool = tool;
    worker->id = strdup_printf("worker:%d", get_time());

    worker->timer = TOOL_ADD_TIMER(worker->tool, 0, _worker_timer, worker);
    if (!worker->timer) {
        ERR("nemotool dispatch idle failed");
        free(worker->id);
        free(worker);
        return NULL;
    }

    return worker;
}

