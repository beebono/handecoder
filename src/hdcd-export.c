#include "hdcd.h"
#include <drm/drm_fourcc.h>
#include <libavutil/hwcontext_drm.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <twig.h>

#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

static struct SwsContext *sws_ctx = NULL;
static display_buf_t *pool = NULL;
static bool pool_initialized = false;

static AVDRMFrameDescriptor *export_rgb(const AVFrame *frame, display_buf_t *buffer) {
    int width = frame->width;
    int height = frame->height;
    AVDRMFrameDescriptor *desc = calloc(1, sizeof(AVDRMFrameDescriptor));

    if (!sws_ctx) {
        sws_ctx = sws_getContext(
            width, height, AV_PIX_FMT_NV12,
            width, height, AV_PIX_FMT_0RGB32, // A133(P) w/ PVR GE8300 requires 32 bpp, can't use standard 0RGB
            SWS_FAST_BILINEAR | SWS_FULL_CHR_H_INT | SWS_ACCURATE_RND,
            NULL, NULL, NULL);
    }

    uint8_t *data[4];
    int linesize[4];
    av_image_fill_arrays(data, linesize, (const uint8_t*)buffer->mapping, AV_PIX_FMT_0RGB32, width, height, 32);
    sws_scale(sws_ctx, (const uint8_t * const*)frame->data, frame->linesize, 0, height, data, linesize);

    desc->nb_objects = 1;
    desc->objects[0].fd = buffer->fd;
    desc->objects[0].size = buffer->size;
    desc->objects[0].format_modifier = DRM_FORMAT_MOD_LINEAR;
    desc->nb_layers = 1;
    desc->layers[0].format = DRM_FORMAT_BGRX8888; // Byte-order reverse required, hence AV 1234 -> DRM 4321
    desc->layers[0].nb_planes = 1;
    desc->layers[0].planes[0].object_index = 0;
    desc->layers[0].planes[0].offset = 0;
    desc->layers[0].planes[0].pitch = linesize[0];

    return desc;
}

static AVDRMFrameDescriptor *export_yuv(const AVFrame *frame, display_buf_t *buffer) {
    int width = frame->width;
    int height = frame->height;
    AVDRMFrameDescriptor *desc = calloc(1, sizeof(AVDRMFrameDescriptor));

    int y_size = width * height;
    int uv_size = (width * height) / 4;

    // Standard Y -> Y copy
    memcpy(buffer->mapping, frame->data[0], y_size);

    // Need to deinterleave UV for NV12 -> YUV420P, easiest way is to just read the bits out into seperated offsets.
    uint8_t *src_uv = frame->data[1];
    uint8_t *dst_u = (uint8_t*)buffer->mapping + y_size;
    uint8_t *dst_v = (uint8_t*)buffer->mapping + y_size + uv_size;
    for (int i = 0; i < uv_size; i++) {
        dst_u[i] = src_uv[i * 2];
        dst_v[i] = src_uv[i * 2 + 1];
    }

    desc->nb_objects = 1;
    desc->objects[0].fd = buffer->fd;
    desc->objects[0].size = buffer->size;
    desc->objects[0].format_modifier = DRM_FORMAT_MOD_LINEAR;
    desc->nb_layers = 1;
    desc->layers[0].format = DRM_FORMAT_YUV420;
    desc->layers[0].nb_planes = 3;
    desc->layers[0].planes[0].object_index = 0;
    desc->layers[0].planes[0].offset = 0;
    desc->layers[0].planes[0].pitch = width;
    desc->layers[0].planes[1].object_index = 0;
    desc->layers[0].planes[1].offset = y_size;
    desc->layers[0].planes[1].pitch = width / 2;
    desc->layers[0].planes[2].object_index = 0;
    desc->layers[0].planes[2].offset = y_size + uv_size;
    desc->layers[0].planes[2].pitch = width / 2;
    return desc;
}

void yuv2drm(const AVFrame *src, AVFrame *dst) {
    int width = src->width;
    int height = src->height;

    if (current_device == DEVICE_TYPE_ALLWINNER) {
        // TODO: Add libtwig support via twig.h
    }


    if (!pool_initialized) {
        size_t total_size;
        if (needs_rgb)
            total_size = av_image_get_buffer_size(AV_PIX_FMT_0RGB32, width, height, 32);
        else
            total_size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, width, height, 32);

        pool = calloc(MAX_POOL_SIZE, sizeof(display_buf_t));
        buffer_pool_init(pool, total_size);
        pool_initialized = true;
    }
    display_buf_t *buffer = buffer_pool_acquire(&pool);

    AVDRMFrameDescriptor *desc;
    if (needs_rgb)
        desc = export_rgb(src, buffer);
    else
        desc = export_yuv(src, buffer);

    dst->width = width;
    dst->height = height;
    dst->format = AV_PIX_FMT_OPENCL; // HACK: Required off-by-one...
    dst->crop_top = src->crop_top;
    dst->crop_bottom = src->crop_bottom;
    dst->crop_left = src->crop_left;
    dst->crop_right = src->crop_right;
    dst->data[0] = (uint8_t*)desc;
    dst->buf[0] = av_buffer_create((uint8_t*)desc, sizeof(*desc), buffer_used, buffer, 0);
}

void buffer_pool_cleanup(display_buf_t *pool);

void hdcd_export_cleanup(void) {
    if (sws_ctx) {
        sws_freeContext(sws_ctx);
        sws_ctx = NULL;
    }
    
    if (pool) {
        buffer_pool_cleanup(pool);
        free(pool);
        pool = NULL;
    }
    pool_initialized = false;
}