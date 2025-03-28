#define _GNU_SOURCE

#include "hdcd-common.h"

#define PATH_MAX 1024

static void *avcodec_handle = NULL;
static void *avutil_handle = NULL;
static void *avformat_handle = NULL;
static void *swresample_handle = NULL;
static int libraries_loaded = 0;

static void *(*real_dlopen)(const char *filename, int flags);

void *dlopen(const char *filename, int flags) {
    if (!real_dlopen) {
        real_dlopen = dlsym(RTLD_NEXT, "dlopen");
    }

    if (filename && (strstr(filename, "libavcodec.so") ||
                    strstr(filename, "libavutil.so") ||
                    strstr(filename, "libavformat.so") ||
                    strstr(filename, "libswresample.so"))) {
        if (!libraries_loaded) {
            return NULL;
        }
    }
    return real_dlopen(filename, flags);
}

int init_ffmpeg(const char *lib_path) {
    char path[PATH_MAX];
    
    if (libraries_loaded) {
        return 0;
    }

    snprintf(path, PATH_MAX, "%s/libavutil.so", lib_path);
    avutil_handle = real_dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!avutil_handle) {
        return -1;
    }

    snprintf(path, PATH_MAX, "%s/libavcodec.so", lib_path);
    avcodec_handle = real_dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!avcodec_handle) {
        cleanup_ffmpeg();
        return -1;
    }

    snprintf(path, PATH_MAX, "%s/libavformat.so", lib_path);
    avformat_handle = real_dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!avformat_handle) {
        cleanup_ffmpeg();
        return -1;
    }

    snprintf(path, PATH_MAX, "%s/libswresample.so", lib_path);
    swresample_handle = real_dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!swresample_handle) {
        cleanup_ffmpeg();
        return -1;
    }

    libraries_loaded = 1;
    return 0;
}

void cleanup_ffmpeg(void) {
    if (avcodec_handle) dlclose(avcodec_handle);
    if (avutil_handle) dlclose(avutil_handle);
    if (avformat_handle) dlclose(avformat_handle);
    if (swresample_handle) dlclose(swresample_handle);
    
    avcodec_handle = NULL;
    avutil_handle = NULL;
    avformat_handle = NULL;
    swresample_handle = NULL;
    libraries_loaded = 0;
}