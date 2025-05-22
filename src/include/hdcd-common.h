#ifndef HDCD_COMMON_H
#define HDCD_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/stat.h>

#define O_RDWR      00000002
#define O_CLOEXEC   02000000
#define AT_FDCWD        -100

enum device_type {
    DEVICE_TYPE_NONE,
    DEVICE_TYPE_V4L2,
    DEVICE_TYPE_ROCKCHIP
};

#endif // HDCD_COMMON_H
