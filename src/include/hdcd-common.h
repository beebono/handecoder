#ifndef HDCD_COMMON_H
#define HDCD_COMMON_H

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <dlfcn.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <libavcodec/avcodec.h>

#define O_RDWR      00000002
#define O_CLOEXEC   02000000
#define AT_FDCWD        -100

enum device_type {
    DEVICE_TYPE_NONE,
    DEVICE_TYPE_V4L2REQ,
    DEVICE_TYPE_V4L2,
    DEVICE_TYPE_ROCKCHIP,
    DEVICE_TYPE_ALLWINNER,
    DEVICE_TYPE_SWCOMPAT,
};

#endif
