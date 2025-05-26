#define _GNU_SOURCE

#include "hdcd-common.h"
#include "allwinner/ion.h"
#include <libavutil/hwcontext_drm.h>
#include <drm/drm_fourcc.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <gbm.h>

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

    if (current_device == DEVICE_TYPE_ALLWINNER) {
        int ion_fd = open("/dev/ion", O_RDWR, 0);
        struct ion_allocation_data allocation_data = {
		    .len = size,
		    .align = 4096,
		    .heap_id_mask = ION_HEAP_TYPE_DMA_MASK,
		    .flags = ION_FLAG_CACHED
	    };
        for (int i = 0; i < MAX_POOL_SIZE; i++) {
            ioctl(ion_fd, ION_IOC_ALLOC, &allocation_data);
            struct ion_fd_data fd_data = {
		        .handle = allocation_data.handle
	        };
            ioctl(ion_fd, ION_IOC_MAP, &fd_data);
            pool->buffers[i].fd = fd_data.fd;
            pool->buffers[i].size = size;
            pool->buffers[i].mapping = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_data.fd, 0);
            pool->buffers[i].in_use = false;
        }
    } else if (current_device == DEVICE_TYPE_SWCOMPAT) {
        int drm_fd = open("/dev/dri/card0", O_RDWR, 0);
        struct gbm_device *gbm_dev = gbm_create_device(drm_fd);

        for (int i = 0; i < MAX_POOL_SIZE; i++) {
            struct gbm_bo *bo = gbm_bo_create(gbm_dev, padded_width, padded_height,
                                GBM_FORMAT_XRGB8888, GBM_BO_USE_RENDERING | GBM_BO_USE_LINEAR);

            int fd = gbm_bo_get_fd(bo);
            pool->buffers[i].fd = fd;
            pool->buffers[i].size = size;
            pool->buffers[i].mapping = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            pool->buffers[i].in_use = false;
        }
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

void convert2drm(const AVFrame *src, AVFrame *dst) {
    AVDRMFrameDescriptor *desc = calloc(1, sizeof(*desc));

    int width = src->width;
    int height = src->height;

    size_t sizeY = width * height;
    size_t sizeUV = width * (height / 2);
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
    desc->layers[0].planes[0].pitch = width;
    desc->layers[0].planes[1].object_index = 0;
    desc->layers[0].planes[1].offset = offsetUV;
    desc->layers[0].planes[1].pitch = width;

    memcpy(buffer->mapping, src->data[0], sizeY);
    uint8_t *dst_uv = buffer->mapping + offsetUV;
    for (int i = 0; i < sizeUV / 2; i++) {
        dst_uv[i * 2 + 0] = src->data[1][i];
        dst_uv[i * 2 + 1] = src->data[2][i];
    }

    dst->format = AV_PIX_FMT_OPENCL; // Off-by-one...
    dst->width = src->width;
    dst->height = src->height;
    dst->crop_top = src->crop_top;
    dst->crop_bottom = src->crop_bottom;
    dst->crop_left = src->crop_left;
    dst->crop_right = src->crop_right;
    dst->data[0] = (uint8_t*)desc;
    dst->buf[0] = av_buffer_create((uint8_t*)dst->data[0], sizeof(desc), buffer_used, buffer, 0);
}