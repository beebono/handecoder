#include "hdcd-common.h"
#include <math.h>

enum device_type current_device;
int padded_width, padded_height;
bool needs_rgb;

int open(const char *path, int flags, __u32 mode);

static void find_approx_aspect_match(int *width, int *height) {
    // Any ratios other than these are unlikely to be supported
    // by a handheld or even laptops any time soon...
    double ratios[4] =  { 
        3.0 / 2.0,
        4.0 / 3.0,
        16.0 / 9.0,
        16.0 / 10.0
    };
    double input_ratio = (double)(*width) / *height;
    double min_ratio_diff = INFINITY;
    int best_width = 0, best_height = 0;
    bool is_vertical = *height > *width;

    for (int i = 0; i < 4; i++) {
        if (is_vertical) {
            int possible_height = *height;
            int possible_width = (int)(possible_height * ratios[i] + 0.5);
            if (possible_width <= *width) {
                double diff = fabs(((double)possible_width / possible_height) - input_ratio);
                if (diff < min_ratio_diff) {
                    min_ratio_diff = diff;
                    best_width = possible_width;
                    best_height = possible_height;
                }
            }
        } else {
            int possible_width = *width;
            int possible_height = (int)(possible_width / ratios[i] + 0.5);
            if (possible_height <= *height) {
                double diff = fabs(((double)possible_width / possible_height) - input_ratio);
                if (diff < min_ratio_diff) {
                    min_ratio_diff = diff;
                    best_width = possible_width;
                    best_height = possible_height;
                }
            }
        }
    }
    *width = best_width;
    *height = best_height;
}

static void __attribute__((constructor)) init_wrapper(void) {
    // Try to get resolution from envvar
    // If not set, default to 640x480 for maximum success chance with buffers
    // 640x480 will probably crop the frame, but we'll at least see SOMETHING
    const char *resolution = getenv("HDCD_RESOLUTION");
    int width = 640, height = 480;
    if (resolution) {
        sscanf(resolution, "%dx%d", &width, &height);
    }

    struct stat st;
    if (stat("/dev/video0", &st) == 0) {
        if (stat("/dev/media0", &st) == 0) {
            int fd = open("/dev/media0", O_RDWR, 0);
            if (fd >= 0) { // We've got both in this case: Use newer API with standard h264 decoder
                current_device = DEVICE_TYPE_V4L2REQ;
                close(fd);
            }
        } else {
            int fd = open("/dev/video0", O_RDWR, 0);
            if (fd >= 0) { // Only basic v4l2: Use v4l2m2m instead
                current_device = DEVICE_TYPE_V4L2;
                close(fd);
            }
        }
    } else if (stat("/dev/mpp_service", &st) == 0) {  // Only exists on legacy Rockchip kernels
        int fd = open("/dev/mpp_service", O_RDWR, 0); // Will be caught up in v4l2 check otherwise
        if (fd >= 0) {
            current_device = DEVICE_TYPE_ROCKCHIP;
            close(fd);
        }
    } else if (stat("/dev/cedar_dev", &st) == 0) {  // Again, only legacy kernels
        int fd = open("/dev/cedar_dev", O_RDWR, 0); // and again, v4l2 otherwise
        if (fd >= 0) {
            current_device = DEVICE_TYPE_ALLWINNER;
            close(fd);
            if (stat("/dev/dri/renderD128", &st) == 0) {
                fd = open("/dev/dri/renderD128", O_RDWR, 0);
                if (fd >= 0) { // If cedar AND a DRM device, need RGB frames
                    needs_rgb = true;
                    close(fd);
                }
            }
        }
    } else if (stat("/dev/dri/card0", &st) == 0) {
        int fd = open("/dev/dri/card0", O_RDWR, 0);
        if (fd >= 0) { // We might have DRM dumb buffers here, so try it!
            current_device = DEVICE_TYPE_SWCOMPAT;
            close(fd);
        }
    } else { // No compatibility here, just hope for the best.
        current_device = DEVICE_TYPE_NONE;
    }

    // Account for different screens like that wonky 31:27
    find_approx_aspect_match(&width, &height);

    // Pad up to 8 pixels ( v4l2m2m really needs this and )
    //                    (other codecs don't seem to mind)
    padded_width = (width + 7) & ~7;
    padded_height = (height + 7) & ~7;
}

