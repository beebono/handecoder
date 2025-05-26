#define _GNU_SOURCE

#include "hdcd-common.h"

enum device_type current_device;
int padded_width, padded_height;
bool is_a133;

int open(const char *path, int flags, __u32 mode);

static void __attribute__((constructor)) init_wrapper(void) {
    const char *resolution = getenv("HDCD_RESOLUTION");
    int width = 640, height = 480;
    is_a133 = false;
    if (!resolution) {
        fprintf(stderr, "Couldn't get resolution from environment. Using 640x480...\n");
    } else {
        sscanf(resolution, "%dx%d", &width, &height);
    }

    struct stat st;
    if (stat("/dev/video0", &st) == 0) {
        const char *dev_path = NULL;
        if (stat("/dev/media0", &st) == 0) {
            dev_path = "/dev/media0";
            current_device = DEVICE_TYPE_V4L2REQ;
        } else {
            dev_path = "/dev/video0";
            current_device = DEVICE_TYPE_V4L2;
        }

        int fd = open(dev_path, O_RDWR, 0);
        if (fd >= 0) {
            close(fd);
        }
    } else if (stat("/dev/mpp_service", &st) == 0) {
        int fd = open("/dev/mpp_service", O_RDWR, 0);
        if (fd >= 0) {
            current_device = DEVICE_TYPE_ROCKCHIP;
            close(fd);
        }
    } else if (stat("/dev/cedar_dev", &st) == 0) {
        int fd = open("/dev/cedar_dev", O_RDWR, 0);
        if (fd >= 0) {
            current_device = DEVICE_TYPE_ALLWINNER;
            const char *devname = getenv("HDCD_DEVNAME");
            close(fd);
            if (strcasestr(devname, "trim") != NULL) {
                is_a133 = true;
            }
        }
    } else if (stat("/dev/dri/card0", &st) == 0) {
        int fd = open("/dev/dri/card0", O_RDWR, 0);
        if (fd >= 0) {
            current_device = DEVICE_TYPE_SWCOMPAT;
            close(fd);
        }
    } else {
        current_device = DEVICE_TYPE_NONE;
    }
    int sixteen_nine_width = ceil(height * 16.0 / 9);
    int sixteen_nine_height = ceil(width * 9.0 / 16);
    if (sixteen_nine_width <= width ) {
        padded_width = sixteen_nine_width;
        padded_height = height;
    } else if (sixteen_nine_height <= height) {
        padded_width = width;
        padded_height = sixteen_nine_height;
    }
}

