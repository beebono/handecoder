#include "hdcd-common.h"

#include <libavutil/hwcontext_drm.h>
#include <drm/drm.h>
#include <drm/drm_mode.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#define MAX_POOL_SIZE 8

// Externs are potentially unsafe
extern int padded_width, padded_height;

struct dmabuffer {
    int fd;
    void *mapping;
    size_t size;
    bool in_use;
};

// Maybe use a statically allocated array instead of a single entry struct like this?
struct dma_pool {
    struct dmabuffer *buffers;
};

int open(const char *path, int flags, __u32 mode);

void buffer_pool_init(struct dma_pool *pool, size_t size, AVCodecContext *avctx) {
    pool->buffers = calloc(MAX_POOL_SIZE, sizeof(struct dmabuffer));
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
        pool->buffers[i].fd = prime.fd;
        pool->buffers[i].size = create.size;
        pool->buffers[i].mapping = mmap(NULL, create.size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, map.offset);
        pool->buffers[i].in_use = false;
    }
}

struct dmabuffer *buffer_pool_acquire(struct dma_pool *pool) {
    for (int i = 0; i < MAX_POOL_SIZE; i++) {
        if (!pool->buffers[i].in_use) {
            pool->buffers[i].in_use = true;
            return &pool->buffers[i];
        }
    }
    return NULL;
}

void buffer_used(void *opaque, uint8_t *data) {
    struct dmabuffer *buffer = opaque;
    AVDRMFrameDescriptor *desc = (AVDRMFrameDescriptor*)data;
    free(desc);
    buffer->in_use = false;
}

// TODO: Add a PROPER buffer/pool cleanup function!!!
//       Call at codec close or maybe just as a destructor?