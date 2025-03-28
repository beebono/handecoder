#define _GNU_SOURCE

#include "hdcd-common.h"
#include <dirent.h>

enum device_type current_device;

static char* get_library_path(void) {
    Dl_info dl_info;
    if (dladdr((void*)get_library_path, &dl_info) == 0) {
        return NULL;
    }

    char* ffmpeg_path = malloc(strlen(dl_info.dli_fname) + 16);
    return ffmpeg_path;
}

static void __attribute__((constructor)) init_wrapper(void) {
    char *ffmpeg_path = getenv("FFMPEG_PATH");
    char *default_path = NULL;
    
    if (!ffmpeg_path) {
        default_path = get_library_path();
        ffmpeg_path = default_path ? default_path : "/usr/lib";
    }
    
    free(default_path);
    init_ffmpeg(ffmpeg_path);

    struct stat st;
    if (stat("/dev/video0", &st) == 0) {
        int fd = open("/dev/video0", O_RDWR);
        if (fd >= 0) {
            current_device = DEVICE_TYPE_V4L2;
            close(fd);
        }
    } else if (stat("/dev/dev_cedar", &st) == 0) {
        int fd = open("/dev/dev_cedar", O_RDWR);
        if (fd >= 0) {
            current_device = DEVICE_TYPE_ALLWINNER;
            close(fd);
        }
    } else if (stat("/dev/mpp_service", &st) == 0) {
        int fd = open("/dev/mpp_service", O_RDWR);
        if (fd >= 0) {
            current_device = DEVICE_TYPE_ROCKCHIP;
            close(fd);
        }
    } else {
        current_device = DEVICE_TYPE_NONE;
    }
}

static void __attribute__((destructor)) cleanup_wrapper(void) {
    cleanup_ffmpeg();
}