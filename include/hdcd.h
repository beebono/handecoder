#ifndef HDCD_H_
#define HDCD_H_

#define _GNU_SOURCE

#include <libavcodec/avcodec.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

#endif // HDCD_H_
