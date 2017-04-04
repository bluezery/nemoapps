#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <libgen.h>
#include <time.h>
#include <libudev.h>
#include <sys/epoll.h>
#include <sys/mount.h>
#include <libmount.h>
#include <proc/readproc.h>

#include <uuid/uuid.h>
#include <nemotool.h>
#include <nemotimer.h>
#include <nemobus.h>

#include "nemoutil.h"
#include "widgets.h"
#include "nemoui.h"

#define TIMEOUT 1000
#define LIBMOUNT

typedef struct _Proc Proc;
struct _Proc {
    char *name;
    List *cmds;
    int tid;
    int parent_id;
    int pgroup_id;
    int tgroup_id;
    char state;

    unsigned long user_time;
    unsigned long system_time;
    unsigned long start_time;

    unsigned int priority;
    long nice;

    unsigned long vsz;
    unsigned long rss;
    unsigned long share;

};

void proc_free_procs(List *procs)
{
    Proc *proc;
    LIST_FREE(procs, proc) {
        free(proc->name);
        char *cmd;
        LIST_FREE(proc->cmds, cmd) free(cmd);
        free(proc);
    }
}

List *proc_get_procs()
{
    List *procs = NULL;
    proc_t **task = readproctab(PROC_FILLSTATUS | PROC_FILLMEM | PROC_FILLCOM |
            PROC_FILLENV | PROC_FILLUSR | PROC_FILLGRP | PROC_FILLSTAT, NULL);

    while(task) {
        proc_t *t = (*task);
        task++;

        if (!t) break;

        Proc *proc = calloc(sizeof(Proc), 1);
        proc->name = strdup(t->cmd);

        if (t->cmdline) {
            char *cmd = *(t->cmdline);
            while (cmd) {
                proc->cmds = list_append(proc->cmds, strdup(cmd));
                (t->cmdline)++;
                cmd = *(t->cmdline);
            }
        }

        proc->tid = t->tid;
        proc->parent_id = t->ppid;
        proc->pgroup_id = t->pgrp;
        proc->tgroup_id = t->tgid;
        proc->state = t->state;

        proc->user_time = t->utime;
        proc->system_time = t->stime;
        proc->start_time = t->start_time;

        proc->priority = t->priority;
        proc->nice = t->nice;

        unsigned long page_size = 4096;
        if (t->size) page_size = t->vsize/t->size;

        proc->vsz = t->size * page_size;
        proc->rss = t->resident * page_size;
        proc->share = t->share * page_size;

        procs = list_append(procs, proc);
    }

    return procs;
}

typedef struct _Disk Disk;
typedef struct _DiskDevice DiskDevice;
typedef void (*DiskCallback)(Disk *disk, const char *action, DiskDevice *dev, void *userdata);

struct _Disk {
    struct nemotool *tool;
    struct udev *udev;
    char *mount_exec;
    struct udev_monitor *udev_mon;
    int udev_mon_fd;
    DiskCallback callback;
    void *userdata;
    List *devices;
    struct nemotimer *timer;
};

struct _DiskDevice {
    char *name;
    char *path;
    long unsigned int num;
    char *type;
    char *mnt;
    char *uuid;
};

DiskDevice *disk_create_device(Disk *disk, struct udev_device *udev_dev)
{
    DiskDevice *dev = calloc(sizeof(DiskDevice), 1);

    dev->name = strdup(udev_device_get_sysname(udev_dev));
    dev->path = strdup(udev_device_get_devnode(udev_dev));
    dev->num = udev_device_get_devnum(udev_dev);
    dev->type = strdup(udev_device_get_devtype(udev_dev));
    disk->devices = list_append(disk->devices, dev);

    return dev;
}

void disk_destroy_device(Disk *disk, DiskDevice *dev)
{
    disk->devices = list_remove(disk->devices, dev);
    free(dev->name);
    free(dev->path);
    free(dev->type);
    if (dev->mnt) free(dev->mnt);
    if (dev->uuid) free(dev->uuid);
    free(dev);
}

bool disk_device_unmount(DiskDevice *dev)
{
    RET_IF(!dev, false);
    RET_IF(!dev->path, false);
    RET_IF(!dev->mnt, false);

    if (umount(dev->mnt) < 0) {
        ERR("umount (%s) failed", dev->mnt);
        return true;
    }
    ERR("umounted (%s)", dev->mnt);
    return true;
}

bool disk_device_mount(DiskDevice *dev)
{
    RET_IF(!dev, false);
    RET_IF(!dev->path, false);

#ifdef LIBMOUNT
    struct libmnt_context *mctx;
    mctx = mnt_new_context();
    struct libmnt_table *tb;

    if (mnt_context_get_mtab(mctx, &tb)) {
        ERR("failed to read mtab");
    } else {
        struct libmnt_iter *iter;
        struct libmnt_fs *fs;
        iter = mnt_new_iter(MNT_ITER_FORWARD);
        if (!iter) {
            ERR("mnt new iter failed");
        } else {
            while(mnt_table_next_fs(tb, iter, &fs) == 0) {
                const char *src = mnt_fs_get_source(fs);
                const char *target = mnt_fs_get_target(fs);
                if (src && !strcmp(src, dev->path)) {
                    if (dev->mnt) free(dev->mnt);
                    dev->mnt = strdup(target);
                    ERR("already mounted: %s --> %s", src, target);
                    mnt_free_iter(iter);
                    return true;
                }
            }
        }
        mnt_free_iter(iter);
    }
#endif

    char buf[PATH_MAX];
    snprintf(buf, PATH_MAX, "/media/%s",  dev->name);

    if (!file_mkdir_recursive(buf, 0777)) {
        ERR("failed create directory: %s", buf);
        return false;
    }

#ifdef LIBMOUNT
    int rc;
    mnt_context_set_source(mctx, dev->path);
    mnt_context_set_target(mctx, buf);
    rc = mnt_context_mount(mctx);
    if (rc != 0) {
        int rc2 = mnt_context_get_status(mctx);
        ERR("mnt_context_mount failed %s --> %s (%d)(%d)", dev->path, buf, rc, rc2);
        mnt_free_context(mctx);
        return false;
    } else {
        if (dev->mnt) free(dev->mnt);
        dev->mnt = strdup(buf);
    }
    mnt_free_context(mctx);

#else
    char *fs = NULL;
    char *magic_str = file_get_magic(dev->path, MAGIC_DEVICES);
    if (strstr(magic_str, "FAT")) {
        fs = strdup("vfat");
    }
    if (strstr(magic_str, "ext4")) {
        fs = strdup("ext4");
    }
    if (strstr(magic_str, "EXFAT")) {
        fs =strdup("exfat");
    }
    if (strstr(magic_str, "NTFS")) {
        fs =strdup("ntfs");
    }
    free(magic_str);

    if (!fs) {
        ERR("Not recognized filesystem");
        return false;
    }

    if (mount(dev->path, buf, fs, MS_RDONLY, "") < 0) {
        ERR("mount %s --> %s failed: %s", dev->path, buf, strerror(errno));
        free(fs);
        return false;
    } else {
        if (dev->mnt) free(dev->mnt);
        dev->mnt = strdup(buf);
    }
    free(fs);
#endif

    ERR("mounted: %s --> %s", dev->path, buf);
    return true;
}

static void _disk_monitor_callback(void *userdata, const char *events)
{
    RET_IF(!events);
    if (!strchr(events, 'r')) {
        ERR("Invalid events: %s", events);
        return;
    }
    Disk *disk = userdata;

    struct udev_device *udev_dev;
    udev_dev = udev_monitor_receive_device(disk->udev_mon);
    if (!udev_dev) {
        ERR("No monitoring device");
        return;
    }
    if (!disk->callback) return;

    const char *action = udev_device_get_action(udev_dev);
    const char *devtype = udev_device_get_devtype(udev_dev);
    if (!action || !devtype) return;

    const char *val = udev_device_get_sysattr_value(udev_dev, "removable");
    if (!strcmp(action, "add") && !strcmp(devtype, "disk") &&
            (val && !strcmp(val, "1"))) {

        DiskDevice *dev = disk_create_device(disk, udev_dev);
        disk->callback(disk, action, dev, disk->userdata);

        int i = 1;
        struct udev_device *_udev_dev;
        do {
            char buf[PATH_MAX];
            snprintf(buf, PATH_MAX, "%s%d", dev->name, i++);
            _udev_dev = udev_device_new_from_subsystem_sysname(disk->udev, "block", buf);
            if (!_udev_dev) break;
            DiskDevice *_dev = disk_create_device(disk, _udev_dev);
            disk->callback(disk, action, _dev, disk->userdata);
            udev_device_unref(_udev_dev);
        } while (_udev_dev);

    } else if (!strcmp(action, "remove")) {
        DiskDevice *dev;
        List *l, *ll;
        LIST_FOR_EACH_SAFE(disk->devices, l, ll, dev) {
            if (!strcmp(dev->name, udev_device_get_sysname(udev_dev)) &&
                !strcmp(dev->path, udev_device_get_devnode(udev_dev))) {
                break;
            }
        }
        if (dev) {
            disk->callback(disk, action, dev, disk->userdata);
            disk_destroy_device(disk, dev);
        } else {
            ERR("Not recognized device; %s, %s",
                    udev_device_get_sysname(udev_dev),
                    udev_device_get_devnode(udev_dev));
        }
    }

    udev_device_unref(udev_dev);
}

static void _disk_check_timeout(struct nemotimer *timer, void *userdata)
{
    Disk *disk = userdata;

    int cnt = 0;
    List *l, *ll;
    DiskDevice *dev;
    LIST_FOR_EACH_SAFE(disk->devices, l, ll, dev) {
        if (!dev->mnt) continue;
        cnt++;
        bool found = false;
        List *procs = proc_get_procs();
        List *_l;
        Proc *proc;
        LIST_FOR_EACH(procs, _l, proc) {
            if (!strcmp(proc->name, "nemoexplorer")) {
                char *cmd1 = LIST_DATA(list_get_nth(proc->cmds, 1));
                char *cmd2 = LIST_DATA(list_get_nth(proc->cmds, 2));
                if (cmd1 && !strcmp(cmd1, "-f") && cmd2 && !strcmp(cmd2, dev->mnt)) {
                    found = true;
                    break;
                    //ERR("kill it (%s, %d)", proc->name, proc->tid);
                    //kill(proc->tid, SIGTERM);
                }
            }
        }
        if (!found) {
            ERR("Process was destroyed, unmount it!");
            disk_device_unmount(dev);
            if (dev->uuid) {
                free(dev->uuid);
                dev->uuid = NULL;
            }
            cnt--;
        }
    }

    if (cnt >= 1) {
        nemotimer_set_timeout(timer, TIMEOUT);
    }
#if 0
    Proc *proc;
    List *procs = proc_get_procs();
    LIST_FOR_EACH(procs, l, proc) {
        ERR("%s(%d,%d,%d, %d) %lu, %lu, %lu", proc->name, proc->tid, proc->pgroup_id, proc->tgroup_id,
                proc->parent_id, proc->vsz/1024, proc->rss/1024, proc->share/1024);
    }
#endif
}

void disk_destroy(Disk *disk)
{
    DiskDevice *dev;
    while ((dev = LIST_DATA(LIST_LAST(disk->devices)))) {
        disk_destroy_device(disk, dev);
    }
    udev_monitor_filter_remove(disk->udev_mon);
    udev_monitor_unref(disk->udev_mon);
    nemotimer_destroy(disk->timer);
    free(disk->mount_exec);
    udev_unref(disk->udev);
    free(disk);
}

void disk_search_devices(Disk *disk)
{
    struct udev_enumerate *enumerate;
    struct udev_list_entry *list_entry;
    struct udev_list_entry *list_entry_devices;
    enumerate = udev_enumerate_new(disk->udev);
    udev_enumerate_add_match_subsystem(enumerate, "block");

    udev_enumerate_scan_devices(enumerate);
    list_entry_devices = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry_foreach(list_entry, list_entry_devices) {
        const char *path = udev_list_entry_get_name(list_entry);

        struct udev_device *udev_dev;
        udev_dev = udev_device_new_from_syspath(disk->udev, path);

        const char *devtype = udev_device_get_devtype(udev_dev);
        if (!devtype) continue;

        const char *val = udev_device_get_sysattr_value(udev_dev, "removable");
        if (!strcmp(devtype, "disk") && (val && !strcmp(val, "1"))) {
            DiskDevice *dev = disk_create_device(disk, udev_dev);

            int i = 1;
            struct udev_device *_udev_dev;
            do {
                char buf[PATH_MAX];
                snprintf(buf, PATH_MAX, "%s%d", dev->name, i++);
                _udev_dev = udev_device_new_from_subsystem_sysname(disk->udev, "block", buf);
                if (!_udev_dev) break;
                disk_create_device(disk, _udev_dev);

                udev_device_unref(_udev_dev);
            } while (_udev_dev);
        }

        udev_device_unref(udev_dev);
    }
    udev_enumerate_unref(enumerate);
}

Disk *disk_create(struct nemotool *tool, const char *mount_exec, DiskCallback callback, void *userdata)
{
    Disk *disk = calloc(sizeof(Disk), 1);
    disk->tool = tool;
    disk->udev = udev_new();
    disk->mount_exec = strdup(mount_exec);
    disk->callback = callback;
    disk->userdata = userdata;
    disk->timer = TOOL_ADD_TIMER(tool, 0, _disk_check_timeout, disk);

    disk->udev_mon = udev_monitor_new_from_netlink(disk->udev, "udev");
    if (!disk->udev_mon) {
        ERR("udev_monitor_new failed");
        nemotimer_destroy(disk->timer);
        free(disk->mount_exec);
        udev_unref(disk->udev);
        free(disk);
        return NULL;
    }

    if (udev_monitor_filter_add_match_subsystem_devtype(disk->udev_mon, "block", NULL)) {
        ERR("udev monitor filter add failed");
        udev_monitor_unref(disk->udev_mon);
        nemotimer_destroy(disk->timer);
        free(disk->mount_exec);
        udev_unref(disk->udev);
        free(disk);
        return NULL;
    }

    if (udev_monitor_enable_receiving(disk->udev_mon) != 0) {
        ERR("udev monitor enable failed");
        udev_monitor_filter_remove(disk->udev_mon);
        udev_monitor_unref(disk->udev_mon);
        nemotimer_destroy(disk->timer);
        free(disk->mount_exec);
        udev_unref(disk->udev);
        free(disk);
        return NULL;
    }
    disk->udev_mon_fd = udev_monitor_get_fd(disk->udev_mon);
    if (nemotool_watch_source(tool, disk->udev_mon_fd, "r", _disk_monitor_callback, disk) < 0) {
        ERR("nemotool_watch_source failed");
    }

    disk_search_devices(disk);

    return disk;
}

List *disk_get_devices(Disk *disk)
{
    return disk->devices;
}

static char *_execute(const char *exec, const char *file)
{
    char path[PATH_MAX];
    char args[PATH_MAX];

    char *buf;
    char *tok;
    buf = strdup(exec);
    tok = strtok(buf, ";");
    snprintf(path, PATH_MAX, "%s", tok);
    tok = strtok(NULL, ";");
    snprintf(args, PATH_MAX, "%s;%s", tok, file);
    free(buf);

    ERR("Execute: %s, %s", path, args);
    return nemo_execute(NULL, "app", path, args, NULL, 0, 0, 0, 1, 1);
}

static void _disk_callback(Disk *disk, const char *action, DiskDevice *dev, void *userdata)
{
    ERR("[%s] %s %s %lu %s", action, dev->name, dev->path, dev->num, dev->type);
    RET_IF(!action);
    if (!strcmp(action, "add")) {
        if (disk_device_mount(dev)) {
            if (dev->uuid) free(dev->uuid);
            dev->uuid = _execute(disk->mount_exec, dev->mnt);
            nemotimer_set_timeout(disk->timer, TIMEOUT);
        }
    } else if (!strcmp(action, "remove") && dev->uuid) {
        nemo_close(dev->uuid);
        disk_device_unmount(dev);
    }
}

typedef struct _ConfigApp ConfigApp;
struct _ConfigApp {
    Config *config;
    char *mount_exec;
};

static ConfigApp *_config_load(const char *domain, const char *filename, int argc, char *argv[])
{
    ConfigApp *app = calloc(sizeof(ConfigApp), 1);
    app->config = config_load(domain, filename, argc, argv);

    Xml *xml;
    if (app->config->path) {
        xml = xml_load_from_path(app->config->path);
        if (!xml) ERR("Load configuration failed: %s", app->config->path);
    } else {
        xml = xml_load_from_domain(domain, filename);
        if (!xml) ERR("Load configuration failed: %s:%s", domain, filename);
    }
    if (!xml) {
        config_unload(app->config);
        free(app);
        return NULL;
    }

    const char *temp;

    temp = xml_get_value(xml, "config/mount", "exec");
    if (temp && strlen(temp) > 0) {
        app->mount_exec = strdup(temp);
    } else {
        ERR("No mount exec in configuration file");
    }
    xml_unload(xml);

    return app;
}

static void _config_unload(ConfigApp *app)
{
    config_unload(app->config);
    if (app->mount_exec) free(app->mount_exec);
    free(app);
}


int main(int argc, char *argv[])
{
    ConfigApp *app = _config_load(PROJECT_NAME, CONFXML, argc, argv);
    RET_IF(!app, -1);
    RET_IF(!app->mount_exec, -1);

    struct nemotool *tool = TOOL_CREATE();

    List *l;
    Disk *disk = disk_create(tool, app->mount_exec, _disk_callback, NULL);
    List *devices = disk_get_devices(disk);
    DiskDevice *dev;
    LIST_FOR_EACH(devices, l, dev) {
        if (disk_device_mount(dev)) {
            if (dev->uuid) free(dev->uuid);
            dev->uuid = _execute(disk->mount_exec, dev->mnt);
            nemotimer_set_timeout(disk->timer, TIMEOUT);
        }
    }

    nemotool_run(tool);

    disk_destroy(disk);
    TOOL_DESTROY(tool);

    _config_unload(app);

    return 0;
}
