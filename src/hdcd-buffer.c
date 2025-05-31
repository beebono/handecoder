#define _GNU_SOURCE

#include "hdcd-common.h"
#include <libavutil/imgutils.h>
#include <libavutil/hwcontext_drm.h>
#include <drm/drm.h>
#include <drm/drm_mode.h>
#include <drm/drm_fourcc.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#define ALIGN_UP(x, a) (((x) + ((a) - 1)) & ~((a) - 1))
#define MAX_POOL_SIZE 8

extern int current_device, padded_width, padded_height;

struct dmabuffer {
    int fd;
    void *mapping;
    size_t size;
    bool in_use;
};

struct dma_pool {
    struct dmabuffer *buffers;
};

int open(const char *path, int flags, __u32 mode);

void buffer_pool_init(struct dma_pool *pool, size_t size) {
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

void yuv2drm(const AVFrame *src, AVFrame *dst) {
    AVDRMFrameDescriptor *desc = calloc(1, sizeof(*desc));

    int width = src->width;
    int height = src->height;

    size_t sizeY = src->linesize[0] * height;
    size_t sizeUV = src->linesize[0] * (height / 2);
    size_t offsetUV = ALIGN_UP(sizeY, 4096);
    size_t total_size = ALIGN_UP(offsetUV + sizeUV, 4096);

    static struct dma_pool pool;
    static bool pool_initialized = false;
    if (!pool_initialized) {
        buffer_pool_init(&pool, total_size);
        pool_initialized = true;
    }
    struct dmabuffer *buffer = buffer_pool_acquire(&pool);

    desc->nb_objects = 1;
    desc->objects[0].fd = buffer->fd;
    desc->objects[0].size = buffer->size;
    desc->objects[0].format_modifier = DRM_FORMAT_MOD_LINEAR;
    desc->nb_layers = 1;
    desc->layers[0].format = DRM_FORMAT_NV12;
    desc->layers[0].nb_planes = 2;
    desc->layers[0].planes[0].object_index = 0;
    desc->layers[0].planes[0].offset = 0;
    desc->layers[0].planes[0].pitch = src->linesize[0];
    desc->layers[0].planes[1].object_index = 0;
    desc->layers[0].planes[1].offset = offsetUV;
    desc->layers[0].planes[1].pitch = src->linesize[0];

    for (int y = 0; y < height; y++) {
        memcpy(buffer->mapping + y * src->linesize[0], src->data[0] + y * src->linesize[0], width);
    }
    uint8_t *dst_uv = buffer->mapping + offsetUV;
    for (int y = 0; y < height / 2; y++) {
        for (int x = 0; x < width / 2; x++) {
            int index = y * (src->linesize[1]) + x;
            dst_uv[y * src->linesize[0] + x * 2 + 0] = src->data[1][index];
            dst_uv[y * src->linesize[0] + x * 2 + 1] = src->data[2][index];
        }
    }

    dst->width = width;
    dst->height = height;
    dst->format = AV_PIX_FMT_OPENCL; // Required off-by-one...
    dst->crop_top = src->crop_top;
    dst->crop_bottom = src->crop_bottom;
    dst->crop_left = src->crop_left;
    dst->crop_right = src->crop_right;
    dst->data[0] = (uint8_t*)desc;
    dst->buf[0] = av_buffer_create((uint8_t*)desc, sizeof(*desc), buffer_used, buffer, 0);
}