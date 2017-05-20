#include <curl/curl.h>
#include <sys/epoll.h>

#include <nemotool.h>
#include <nemotimer.h>

#include "xemoutil-internal.h"
#include "xemoutil.h"
#include "xemoutil-con.h"

/*******************************************
 * Connection *
 * *****************************************/
// FIXME: Need mutex
Inlist *_mcon_lists;

typedef struct _MCon MCon;
struct  _MCon {
    Inlist __inlist;

    Inlist *con_lists;

    CURLM *curlm;

    List *fd_lists;

    struct nemotimer *timer;

    struct nemotool *tool;
};

struct _Con {
    Inlist __inlist;

    MCon *mcon;
    struct nemotool *tool;

    CURL *curl;

    char *url;

    FILE *fp;
    size_t data_size;
    void *data;

    bool end;
    Con_Data_Callback data_callback;
    void *data_userdata;
    Con_End_Callback end_callback;
    void *end_userdata;
};

#define MIN_TIMEOUT 100

static MCon *_mcon_get_current(struct nemotool *tool)
{
    MCon *mcon;
    INLIST_FOR_EACH(_mcon_lists, mcon) {
        if (mcon->tool == tool) {
            return mcon;
        }
    }
    return NULL;
}

static void
_mcon_set_timeout(MCon *mcon)
{
    CURLMcode code;
    long tout;

    code = curl_multi_timeout(mcon->curlm, &tout);
    if (code == CURLM_OK) {
        if (tout <= 0 || tout >= MIN_TIMEOUT) tout = MIN_TIMEOUT;
    } else {
        ERR("curl multi timeout failed: %s", curl_multi_strerror(code));
        tout = MIN_TIMEOUT;
    }
    nemotimer_set_timeout(mcon->timer, tout);
}

static void _mcon_set_fd(MCon *mcon);
static void _mcon_destroy(MCon *mcon);
static void _mcon_perform(MCon *mcon)
{
    CURLMcode code;
    int still_running;

    code = curl_multi_perform(mcon->curlm, &still_running);
    if (code == CURLM_CALL_MULTI_PERFORM) {
        LOG("perform again");
    } else if (code != CURLM_OK) {
        ERR("curl multi perform failed: %s", curl_multi_strerror(code));
        return;
    }

    Con *con, *tmp;
    int n_msg;
    CURLMsg *msg;
    while ((msg = curl_multi_info_read(mcon->curlm, &n_msg))) {
        if (msg->msg == CURLMSG_DONE) {
            INLIST_FOR_EACH_SAFE(mcon->con_lists, tmp, con) {
                if (con->curl == msg->easy_handle) {
                    LOG("completed: url(%s)", con->url);
                    if (con->fp) {
                        fclose(con->fp);
                        con->fp = NULL;
                    }
                    con->end = true;
                    // XXX: Should not destory any connections
                    // in the callback function.
                    if (con->end_callback) {
                        con->end_callback
                            (con, true, con->data, con->data_size,
                             con->end_userdata);
                    }
                }
            }
        }
    }

    if (!still_running) {
        LOG("Multi perform ended");
        _mcon_destroy(mcon);

        return;
    }

    _mcon_set_fd(mcon);
    _mcon_set_timeout(mcon);
}

static void
_con_fd_callback(void *userdata, const char *events)
{
    RET_IF(!events);
    if (!strchr(events, 'w')) {
        ERR("Invalid epoll events: %s", events);
        return;
    }

    MCon *mcon = userdata;
    MCon *_mcon;
    // FIXME: fd can be deleted while fd_callback loop
    // After epoll_wait() returned, fd_callback can be called again.
    // Ref: nemotool.c:nemotool_dispatch)
    INLIST_FOR_EACH(_mcon_lists, _mcon) {
        if (_mcon == mcon)  {
            _mcon_perform(mcon);
            return;
        }
    }
    ERR("something is wrong!!: %p is not valid", mcon);
}

static void
_mcon_set_fd(MCon *mcon)
{
    CURLMcode code;
    int max_fd;
    fd_set fd_read;
    fd_set fd_write;
    fd_set fd_err;

    FD_ZERO(&fd_read);
    FD_ZERO(&fd_write);
    FD_ZERO(&fd_err);

    code = curl_multi_fdset(mcon->curlm, &fd_read, &fd_write, &fd_err, &max_fd);
    if (code != CURLM_OK) {
        ERR("curl multi fdset failed: %s", curl_multi_strerror(code));
        return;
    }

    void *fdp;
    LIST_FREE(mcon->fd_lists, fdp)
        nemotool_unwatch_source(mcon->tool, (int)(intptr_t)fdp);

    int fd = 0;
    for (fd = 0 ; fd < max_fd ; fd++) {
        if (FD_ISSET(fd, &fd_write)) {
            nemotool_watch_source(mcon->tool, fd, "w", _con_fd_callback, mcon);
            mcon->fd_lists = list_append(mcon->fd_lists,
                    (void *)(intptr_t)fd);
        }
    }
}

static void
_mcon_timer_callback(struct nemotimer *timer, void *userdata)
{
    MCon *mcon = userdata;
    _mcon_perform(mcon);
}

static void _mcon_destroy(MCon *mcon)
{
    Con *con;
    INLIST_FREE(mcon->con_lists, con) {
        curl_multi_remove_handle(mcon->curlm, con->curl);
        if (con->data) {
            free(con->data);
            con->data = NULL;
            con->data_size = 0;
        }
        con->mcon = NULL;
    }

    nemotimer_destroy(mcon->timer);

    void *fdp;
    LIST_FREE(mcon->fd_lists, fdp)
        nemotool_unwatch_source(mcon->tool, (int)(intptr_t)fdp);

    curl_multi_cleanup(mcon->curlm);

    _mcon_lists = inlist_remove(_mcon_lists, INLIST(mcon));
    free(mcon);
}

static MCon *_mcon_create(struct nemotool *tool)
{
    MCon *mcon;
    mcon = calloc(sizeof(MCon), 1);
    mcon->curlm = curl_multi_init();
    if (!mcon->curlm) {
        ERR("curl multi init failed");
        free(mcon);
        return NULL;
    }

    mcon->con_lists = NULL;

    mcon->timer = nemotimer_create(tool);
    nemotimer_set_callback(mcon->timer, _mcon_timer_callback);
    nemotimer_set_userdata(mcon->timer, mcon);

    mcon->tool = tool;
    _mcon_lists = inlist_append(_mcon_lists, INLIST(mcon));

    return mcon;
}

bool con_init()
{
    CURLMcode code;
    code = curl_global_init(CURL_GLOBAL_ALL);
    if (code) {
        ERR("curl global init failed: %s", curl_easy_strerror(code));
        return false;
    }

    _mcon_lists = NULL;
    return true;
}

void con_shutdown()
{
    MCon *mcon, *tmp;
    INLIST_FOR_EACH_SAFE(_mcon_lists, tmp, mcon) {
        _mcon_destroy(mcon);
    }
    curl_global_cleanup();
}

static size_t
_con_write_function(char *ptr, size_t size, size_t nememb, void *userdata)
{
    Con *con = userdata;
    if (!ptr || (size == 0) || (nememb == 0)) return 0;
    if (con->data_callback)
        con->data_callback(con, ptr, size*nememb, con->data_userdata);

    con->data = realloc(con->data, con->data_size + size*nememb);
    memcpy(con->data + con->data_size, ptr, size*nememb);
    con->data_size += (size * nememb);
    return size * nememb;
}

void con_run(Con *con)
{
    MCon *mcon = _mcon_get_current(con->tool);
    if (!mcon) mcon = _mcon_create(con->tool);

    CURLMcode mcode;
    mcode = curl_multi_add_handle(mcon->curlm, con->curl);
    if (mcode != CURLM_OK) {
        ERR("curl multi add handle failed: %s",
                curl_multi_strerror(mcode));
        return;
    }
    con->mcon = mcon;
    mcon->con_lists = inlist_append(mcon->con_lists, INLIST(con));

    _mcon_set_fd(mcon);
    _mcon_set_timeout(mcon);
}

void con_destroy(Con *con)
{
    if (!con->end && con->end_callback)
        con->end_callback
            (con, false, NULL, 0, con->end_userdata);
    if (con->url) free(con->url);
    if (con->fp) fclose(con->fp);
    if (con->data) free(con->data);
    if (con->mcon) {
        curl_multi_remove_handle(con->mcon->curlm, con->curl);
        con->mcon->con_lists = inlist_remove(con->mcon->con_lists, INLIST(con));
    }
    if (con->curl) curl_easy_cleanup(con->curl);

    free(con);
}

void con_abort(Con *con)
{
}

Con *con_create(struct nemotool *tool)
{
    CURLMcode code;

    CURL *curl = curl_easy_init();
    if (!curl) return NULL;

    Con *con = calloc(sizeof(Con), 1);
    con->tool = tool;

    // debugging
    // if (code = curl_easy_setopt(curl, CURLOPT_VERBOSE, 1))
    //    ERR("%s", curl_easy_strerror(code));
#if 0
    code = curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, _curl_progress_func);
    code = curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, NULL);
    if (code) ERR("%s", curl_easy_strerror(code));

    code = curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, _curl_header_func);
    code = curl_easy_setopt(curl, CURLOPT_HEADERDATA, NULL);
    if (code) ERR("%s", curl_easy_strerror(code));
#endif
    code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _con_write_function);
    if (code) ERR("%s", curl_easy_strerror(code));
    code = curl_easy_setopt(curl, CURLOPT_WRITEDATA, con);
    if (code) ERR("%s", curl_easy_strerror(code));

    code = curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30);
    if (code) ERR("%s", curl_easy_strerror(code));
    code = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    if (code) ERR("%s", curl_easy_strerror(code));


    con->curl = curl;
    return con;
}

void con_set_file(Con *con, const char *path)
{
    CURLcode code;
    CURL *curl = con->curl;
    if (path) {
        con->fp = fopen(path, "w+");
        if (!con->fp) {
            ERR("%s", strerror(errno));
            return;
        }
        code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        if (code) ERR("%s", curl_easy_strerror(code));
        code = curl_easy_setopt(curl, CURLOPT_WRITEDATA, con->fp);
        if (code) ERR("%s", curl_easy_strerror(code));
    } else {
        code = curl_easy_setopt
            (curl, CURLOPT_WRITEFUNCTION, _con_write_function);
        if (code) ERR("%s", curl_easy_strerror(code));
        code = curl_easy_setopt(curl, CURLOPT_WRITEDATA, con);
        if (code) ERR("%s", curl_easy_strerror(code));
    }
}

const char *con_get_url(Con *con)
{
    RET_IF(!con, NULL);
    return con->url;
}

void con_set_url(Con *con, const char *url)
{
    CURLcode code;
    code = curl_easy_setopt(con->curl, CURLOPT_URL, url);
    if (code) ERR("%s", curl_easy_strerror(code));

    if (con->url) free(con->url);
    con->url = strdup(url);
}

void con_set_end_callback(Con *con, Con_End_Callback callback, void *userdata)
{
    con->end_callback = callback;
    con->end_userdata = userdata;
}

void con_set_data_callback(Con *con, Con_Data_Callback callback, void *userdata)
{
    con->data_callback = callback;
    con->data_userdata = userdata;
}
