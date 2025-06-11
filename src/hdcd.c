#include "hdcd.h"
#include <math.h>

hdcd_device_t device = {0};

static void find_approx_aspect_match(int *width, int *height) {
    // Any ratios other than these are unlikely to be supported
    // by a handheld or even laptops or tablets any time soon...
    const double ratios[][2] =  {
        {3, 2}, {4, 3}, {16, 9}, {16, 10}
    };
    int num_ratios = sizeof(ratios)/sizeof(ratios[0]);
    double input_ratio = (double)(*width) / *height;
    double min_ratio_diff = INFINITY;
    int best_width = 0, best_height = 0;
    bool is_vertical = *height > *width;

    for (int i = 0; i < num_ratios; i++) {
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
    if (resolution)
        sscanf(resolution, "%dx%d", &width, &height);

    if (access("/dev/video0", F_OK) == 0) {
        if (access("/dev/media0", F_OK) == 0) { // We've got both in this case 
            device->type = V4L2REQ; // Use newer API with standard h264 decoder
        } else { // Only basic v4l2: Use v4l2m2m instead
            device->type = V4L2;
        }
        goto finish;
    }
    
    if (access("/dev/mpp_service", F_OK) == 0) { // Only exists on legacy Rockchip kernels
        device->type = ROCKCHIP;     // Will be caught up in v4l2 check otherwise
        goto finish;
    }

    if (access("/dev/cedar_dev", F_OK) == 0) { // Again, only legacy kernels
        device->type = ALLWINNER;  // and again, v4l2 otherwise
        if (access("/dev/dri/renderD128", F_OK) == 0)
            device->needs_rgb = true; // If Cedar AND a DRM device, need RGB frames

        goto finish;
    }

    int fd = open("/dev/dri/card0", O_RDWR, 0);
    if (fd >= 0) { // We might have DRM dumb buffers here, so try it!
        device->type = SWCOMPAT;
        close(fd);
        goto finish;
    }
    // No compatibility here, just hope for the best.
    device->type = NONE;

finish:
    // Account for different screens like that wonky 31:27
    find_approx_aspect_match(&width, &height);

    if (device->type == V4L2) {
        device->width = (width + 7) & ~7;
        device->height = (height + 7) & ~7;
    } else {
        device->width = width;
        device->height = height;
    }
}

void hdcd_export_cleanup(void);

static void __attribute__((destructor)) cleanup_wrapper(void) {
    hdcd_export_cleanup();
}