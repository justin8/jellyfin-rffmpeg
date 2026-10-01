#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

static int (*real_unlink)(const char *) = NULL;
static int (*real_unlinkat)(int, const char *, int) = NULL;
static int (*real_rmdir)(const char *) = NULL;

__attribute__((constructor))
static void init(void) {
    real_unlink = dlsym(RTLD_NEXT, "unlink");
    real_unlinkat = dlsym(RTLD_NEXT, "unlinkat");
    real_rmdir = dlsym(RTLD_NEXT, "rmdir");
}

static inline int is_target(const char *path) {
    if (!path) return 0;
    return (strstr(path, "/cache") != NULL) || (strstr(path, ".nfs") != NULL);
}

int unlink(const char *pathname) {
    if (!real_unlink) {
        real_unlink = dlsym(RTLD_NEXT, "unlink");
    }
    int ret = real_unlink(pathname);
    if (ret != 0 && errno == EBUSY && is_target(pathname)) {
        useconds_t sleep_us = 10000; // 10ms
        for (int i = 0; i < 9; i++) {
            usleep(sleep_us);
            sleep_us = (sleep_us < 200000) ? sleep_us * 2 : 200000;
            ret = real_unlink(pathname);
            if (ret == 0 || errno == ENOENT) return 0;
        }
    }
    return ret;
}

int unlinkat(int dirfd, const char *pathname, int flags) {
    if (!real_unlinkat) {
        real_unlinkat = dlsym(RTLD_NEXT, "unlinkat");
    }
    int ret = real_unlinkat(dirfd, pathname, flags);
    if (ret != 0 && is_target(pathname)) {
        int is_busy = (errno == EBUSY) || ((flags & AT_REMOVEDIR) && errno == ENOTEMPTY);
        if (is_busy) {
            useconds_t sleep_us = 10000; // 10ms
            for (int i = 0; i < 9; i++) {
                usleep(sleep_us);
                sleep_us = (sleep_us < 200000) ? sleep_us * 2 : 200000;
                ret = real_unlinkat(dirfd, pathname, flags);
                if (ret == 0 || errno == ENOENT) return 0;
            }
        }
    }
    return ret;
}

int rmdir(const char *pathname) {
    if (!real_rmdir) {
        real_rmdir = dlsym(RTLD_NEXT, "rmdir");
    }
    int ret = real_rmdir(pathname);
    if (ret != 0 && (errno == EBUSY || errno == ENOTEMPTY) && is_target(pathname)) {
        useconds_t sleep_us = 10000; // 10ms
        for (int i = 0; i < 9; i++) {
            usleep(sleep_us);
            sleep_us = (sleep_us < 200000) ? sleep_us * 2 : 200000;
            ret = real_rmdir(pathname);
            if (ret == 0 || errno == ENOENT) return 0;
        }
    }
    return ret;
}
