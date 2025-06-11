#include "hdcd.h"

#include <libavutil/hwcontext_drm.h>
#include <drm/drm.h>
#include <drm/drm_mode.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

hdcd_device_t device;

int open(const char *path, int flags, __u32 mode);

void buffer_pool_init(display_buf_t *pool, size_t size, AVCodecContext *avctx) {
    int drm_fd = open("/dev/dri/card0", O_RDWR, 0);
    for (int i = 0; i < MAX_POOL_SIZE; i++) {
        struct drm_mode_create_dumb create = {
            .width = padded_width,
            .height = padded_height,
            .bpp = 32,
        };
        if (ioctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create)) {
            perror("DRM_IOCTL_MODE_CREATE_DUMB");
        }
        struct drm_prime_handle prime = {
            .handle = create.handle,
            .flags = DRM_RDWR,
        };
        if (ioctl(drm_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime)) {
            perror("DRM_IOCTL_PRIME_HANDLE_TO_FD");
        }
        struct drm_mode_map_dumb map = {
            .handle = create.handle,
        };
        if (ioctl(drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &map)) {
            perror("DRM_IOCTL_MODE_MAP_DUMB");
        }
        pool[i].fd = prime.fd;
        pool[i].size = create.size;
        pool[i].mapping = mmap(NULL, create.size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, map.offset);
        pool[i].in_use = false;
    }
}

display_buf_t *buffer_pool_acquire(struct dma_pool *pool) {
    for (int i = 0; i < MAX_POOL_SIZE; i++) {
        if (!pool[i].in_use) {
            pool[i].in_use = true;
            return &pool[i];
        }
    }
    return NULL;
}

void buffer_used(void *opaque, uint8_t *data) {
    display_buf_t *buffer = opaque;
    AVDRMFrameDescriptor *desc = (AVDRMFrameDescriptor*)data;
    free(desc);
    buffer->in_use = false;
}

// TODO: Add a PROPER buffer/pool cleanup function!!!
//       Call at codec close or maybe just as a destructor?