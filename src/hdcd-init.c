#define _GNU_SOURCE

#include "hdcd-common.h"
#include <X11/Xlib.h>
#include <dirent.h>

enum device_type current_device;
int padded_width, padded_height;

int open(const char *path, int flags, __u32 mode);

static void __attribute__((constructor)) init_wrapper(void) {
    Display* d = XOpenDisplay(NULL);
    Screen*  s = DefaultScreenOfDisplay(d);

    struct stat st;
    if (stat("/dev/media0", &st) == 0) {
        int fd = open("/dev/media0", O_RDWR, 0);
        if (fd >= 0) {
            current_device = DEVICE_TYPE_V4L2REQ;
            padded_width = (s->width + 31) & ~31;
            padded_height = (s->height + 31) & ~31;
            close(fd);
        }
    } else if (stat("/dev/video0", &st) == 0) {
        int fd = open("/dev/video0", O_RDWR, 0);
        if (fd >= 0) {
            current_device = DEVICE_TYPE_V4L2;
            padded_width = (s->width + 31) & ~31;
            padded_height = (s->height + 31) & ~31;
            close(fd);
        }
    } else if (stat("/dev/mpp_service", &st) == 0) {
        int fd = open("/dev/mpp_service", O_RDWR, 0);
        if (fd >= 0) {
            current_device = DEVICE_TYPE_ROCKCHIP;
            padded_width = (s->width + 15) & ~15;
            padded_height = (s->height + 15) & ~15;
            close(fd);
        }
    } else {
        current_device = DEVICE_TYPE_NONE;
    }

    XCloseDisplay(d);
}
