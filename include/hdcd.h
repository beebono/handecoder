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
#include <sys/types.h>
#include <sys/syscall.h>
#include <libavcodec/avcodec.h>

#define O_RDWR      00000002
#define O_CLOEXEC   02000000
#define AT_FDCWD        -100
#define MAX_POOL_SIZE      8

typedef struct {
    int fd;
    void *mapping;
    size_t size;
    bool in_use;
} display_buf_t;

typedef enum {
    NONE,
    V4L2REQ,
    V4L2,
    ROCKCHIP,
    ALLWINNER,
    SWCOMPAT
} device_type_t;

typedef struct {
    device_type_t type;
    int width, height;
    bool needs_rgb;
} hdcd_dev_t;

extern hdcd_dev_t device;

int open(const char *path, int flags, uint32_t mode);

#endif
