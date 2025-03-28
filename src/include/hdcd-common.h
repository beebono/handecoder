#ifndef HDCD_COMMON_H
#define HDCD_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>

enum device_type {
    DEVICE_TYPE_NONE,
    DEVICE_TYPE_V4L2,
    DEVICE_TYPE_ALLWINNER,
    DEVICE_TYPE_ROCKCHIP
};

int init_ffmpeg(const char *lib_path);
void cleanup_ffmpeg(void);

#endif // HDCD_COMMON_H