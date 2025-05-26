#define _GNU_SOURCE

#include "hdcd-common.h"

enum device_type current_device;
int padded_width, padded_height;

int open(const char *path, int flags, __u32 mode);

static void __attribute__((constructor)) init_wrapper(void) {
    const char *resolution = getenv("HDCD_RESOLUTION");
    int width = 640, height = 480;
    if (resolution == "unknown") {
        fprintf(stderr, "Couldn't get resolution from environment. Using 640x480...\n");
    } else {
        sscanf(resolution, "%dx%d", &width, &height);
        if (height > width) {
            sscanf(resolution, "%dx%d", &height, &width);
        }
    }

    struct stat st;
    if (stat("/dev/video0", &st) == 0) {
        if (stat("/dev/media0", &st) == 0) {
            int fd = open("/dev/media0", O_RDWR, 0);
            if (fd >= 0) {
                current_device = DEVICE_TYPE_V4L2REQ;
                padded_width = (width + 31) & ~31;
                padded_height = (height + 31) & ~31;
                close(fd);
            }
        }
        int fd = open("/dev/video0", O_RDWR, 0);
        if (fd >= 0) {
            current_device = DEVICE_TYPE_V4L2;
            padded_width = (width + 31) & ~31;
            padded_height = (height + 31) & ~31;
            close(fd);
        }
    } else if (stat("/dev/mpp_service", &st) == 0) {
        int fd = open("/dev/mpp_service", O_RDWR, 0);
        if (fd >= 0) {
            current_device = DEVICE_TYPE_ROCKCHIP;
            padded_width = (width + 15) & ~15;
            padded_height = (height + 15) & ~15;
            close(fd);
        }
    } else if (stat("/dev/cedar_dev", &st) == 0) {
        int fd = open("/dev/cedar_dev", O_RDWR, 0);
        if (fd >= 0) {
            current_device = DEVICE_TYPE_ALLWINNER;
            padded_width = width;
            padded_height = height;
            close(fd);
        }
    } else if (stat("/dev/dri/card0", &st) == 0) {
        int fd = open("/dev/dri/card0", O_RDWR, 0);
        if (fd >= 0) {
            current_device = DEVICE_TYPE_SWCOMPAT;
            padded_width = width;
            padded_height = height;
            close(fd);
    } else {
            current_device = DEVICE_TYPE_NONE;
        }
    }
}
