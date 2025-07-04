#include "hdcd.h"
#include <dlfcn.h>
#include <sys/syscall.h>

// First hardcoded entry in Steam Link, will always be opened on stream init
#define DMA_HEAP_PATH "/dev/dma_heap/vidbuf_cached"

void yuv2drm(AVCodecContext *avctx, const AVFrame *src, AVFrame *dst);
static int (*real_avcodec_open2)(AVCodecContext *avctx, const AVCodec *codec, AVDictionary **options);
static int (*real_avcodec_receive_frame)(AVCodecContext *avctx, const AVFrame *frame);
static AVBufferRef* (*real_av_buffer_ref)(const AVBufferRef *buf);

// Do this chain of open hooks to ensure compatibility
int real_open(const char *path, int flags, uint32_t mode) {
    return syscall(SYS_openat, AT_FDCWD, path, flags, mode);
}

int open64(const char *path, int flags, uint32_t mode) {
    if (path && strcmp(path, DMA_HEAP_PATH) == 0) {
        if (device.type != NONE)
            return 0; // HACK: Force success because Steam Link will refuse like a brat on failure
    }
    return real_open(path, flags, mode);
}

int open(const char *path, int flags, uint32_t mode) {
    return open64(path, flags, mode);
}

static enum AVPixelFormat get_drm_format(AVCodecContext *avctx, const enum AVPixelFormat *pix_fmts) {
    while (*pix_fmts != -1) {
        if (*pix_fmts == AV_PIX_FMT_DRM_PRIME)
            return *pix_fmts;

        pix_fmts++;
    }
    return AV_PIX_FMT_NONE;
}

static int is_h264_sw_decoder(const AVCodec *codec) {
    if (codec->id != AV_CODEC_ID_H264)
        return 0;
    if (codec->capabilities & (AV_CODEC_CAP_HARDWARE | AV_CODEC_CAP_HYBRID))
        return 0;

    return 1;
}

int avcodec_open2(AVCodecContext *avctx, const AVCodec *codec, AVDictionary **options) {
    if (!real_avcodec_open2)
        real_avcodec_open2 = dlsym(RTLD_NEXT, "avcodec_open2");

    if (codec && is_h264_sw_decoder(codec) && device.type != NONE) {
        avcodec_free_context(&avctx); // Annihilate any previous references like get_format and get_buffer2.
        if (device.type == V4L2) { // The above might cause pointer integrity errors in the future, but it's okay for now.
            codec = avcodec_find_decoder_by_name("h264_v4l2m2m");
            avctx = avcodec_alloc_context3(codec);
            av_hwdevice_ctx_create(&avctx->hw_device_ctx, AV_HWDEVICE_TYPE_DRM, "/dev/dri/card0", NULL, 0);
            avctx->get_format = get_drm_format;
            avctx->pix_fmt = AV_PIX_FMT_DRM_PRIME;
        } else if (device.type == ROCKCHIP) {
            codec = avcodec_find_decoder_by_name("h264_rkmpp");
            avctx = avcodec_alloc_context3(codec);
        } else if (device.type == ALLWINNER) {
            codec = avcodec_find_decoder_by_name("h264_cedar");
            avctx = avcodec_alloc_context3(codec);
            avctx->get_format = get_drm_format;
            avctx->pix_fmt = AV_PIX_FMT_DRM_PRIME;
        } else {
            codec = avcodec_find_decoder_by_name("h264");
            avctx = avcodec_alloc_context3(codec);
            if (device.type == V4L2REQ)
                av_hwdevice_ctx_create(&avctx->hw_device_ctx, AV_HWDEVICE_TYPE_V4L2REQUEST, NULL, NULL, 0);
        }
        avctx->width = device.width;
        avctx->height = device.height;
    }
    return real_avcodec_open2(avctx, codec, options);
}

int avcodec_receive_frame(AVCodecContext *avctx, AVFrame *frame) {
    if (!real_avcodec_receive_frame)
        real_avcodec_receive_frame = dlsym(RTLD_NEXT, "avcodec_receive_frame");

    if (avctx->codec->type == AVMEDIA_TYPE_VIDEO && device.type != NONE) {
        int ret = real_avcodec_receive_frame(avctx, frame);
        if (ret < 0)
            return ret;

        // HACK: Do this garbage to fix an off-by-one in Steam Link caused by changing to updated ffmpeg libraries...
        if (frame->format == AV_PIX_FMT_DRM_PRIME) {
            frame->format = AV_PIX_FMT_OPENCL;
        }
        if (frame->format == AV_PIX_FMT_YUV420P || frame->format == AV_PIX_FMT_NV12) {
            AVFrame *tmpframe = av_frame_alloc();
            av_frame_move_ref(tmpframe, frame);
            yuv2drm(avctx, tmpframe, frame);
            av_frame_free(&tmpframe);
        }
        return 0;
    }
    return real_avcodec_receive_frame(avctx, frame);
}

AVBufferRef *av_buffer_ref(const AVBufferRef *buf) {
    if (!real_av_buffer_ref)
        real_av_buffer_ref = dlsym(RTLD_NEXT, "av_buffer_ref");

    // HACK: And this because Steam Link tries to ref a buffer that WILL NEVER EXIST?????
    //       I'm not even sure how it's NORMALLY supposed to work without this honestly.
    //       So this really sucks to do, but we can't change Steam Link's code...
    static bool notified = false;
    if (!buf){
        if (!notified) {
            fprintf(stderr, "NOTICE: Hooked app tried to call av_buffer_ref(NULL) at least once - Returning NULL to prevent fault\n");
            notified = true;
        }
        return NULL;
    }

    return real_av_buffer_ref(buf);
}
