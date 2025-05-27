#define _GNU_SOURCE

#include "hdcd-common.h"

#define DMA_HEAP_PATH "/dev/dma_heap/vidbuf_cached"

extern int current_device, padded_width, padded_height;

int convert2drm(const AVFrame *src, AVFrame *dst);
static int (*real_avcodec_open2)(AVCodecContext *avctx, const AVCodec *codec, AVDictionary **options);
static int (*real_avcodec_receive_frame)(AVCodecContext *avctx, const AVFrame *frame);
static AVBufferRef* (*real_av_buffer_ref)(const AVBufferRef *buf);

int real_open(const char *path, int flags, __u32 mode) {
    return syscall(SYS_openat, AT_FDCWD, path, flags, mode);
}

int open64(const char *path, int flags, __u32 mode) {
    if (path && strcmp(path, DMA_HEAP_PATH) == 0) {
        if (current_device != DEVICE_TYPE_NONE) {
            return real_open("/dev/null", O_RDWR, 0);
        }
    }
    return real_open(path, flags, mode);
}

int open(const char *path, int flags, __u32 mode) {
    return open64(path, flags, mode);
}

enum AVPixelFormat get_drm_format(AVCodecContext *avctx, const enum AVPixelFormat *pix_fmts) {
    while (*pix_fmts != -1) {
        if (*pix_fmts == AV_PIX_FMT_DRM_PRIME) {
            return *pix_fmts;
        }
        pix_fmts++;
    }
    return AV_PIX_FMT_NONE;
}

static int is_h264_sw_decoder(const AVCodec *codec) {
    if (codec->id != AV_CODEC_ID_H264) return 0;
    if (codec->capabilities & (AV_CODEC_CAP_HARDWARE | AV_CODEC_CAP_HYBRID)) return 0;
    return 1;
}

int avcodec_open2(AVCodecContext *avctx, const AVCodec *codec, AVDictionary **options) {
    if (!real_avcodec_open2) {
        real_avcodec_open2 = dlsym(RTLD_NEXT, "avcodec_open2");
    }

    if (codec && is_h264_sw_decoder(codec) && current_device != DEVICE_TYPE_NONE) {
        avcodec_free_context(&avctx);
        if (current_device == DEVICE_TYPE_V4L2REQ) {
            codec = avcodec_find_decoder_by_name("h264");
            avctx = avcodec_alloc_context3(codec);
            av_hwdevice_ctx_create(&avctx->hw_device_ctx, AV_HWDEVICE_TYPE_V4L2REQUEST, NULL, NULL, 0);
        } else if (current_device == DEVICE_TYPE_V4L2) {
            codec = avcodec_find_decoder_by_name("h264_v4l2m2m");
            avctx = avcodec_alloc_context3(codec);
            av_hwdevice_ctx_create(&avctx->hw_device_ctx, AV_HWDEVICE_TYPE_DRM, "/dev/dri/card0", NULL, 0);
            avctx->get_format = get_drm_format;
            avctx->pix_fmt = AV_PIX_FMT_DRM_PRIME;
        } else if (current_device == DEVICE_TYPE_ROCKCHIP) {
            codec = avcodec_find_decoder_by_name("h264_rkmpp");
            avctx = avcodec_alloc_context3(codec);
        } else if (current_device == DEVICE_TYPE_ALLWINNER || current_device == DEVICE_TYPE_SWCOMPAT) {
            codec = avcodec_find_decoder_by_name("h264");
            avctx = avcodec_alloc_context3(codec);
        }
        avctx->width = padded_width;
        avctx->height = padded_height;
    }
    return real_avcodec_open2(avctx, codec, options);
}

int avcodec_receive_frame(AVCodecContext *avctx, AVFrame *frame) {
    if (!real_avcodec_receive_frame) {
        real_avcodec_receive_frame = dlsym(RTLD_NEXT, "avcodec_receive_frame");
    }

    if (avctx->codec->type == AVMEDIA_TYPE_VIDEO && current_device != DEVICE_TYPE_NONE) {
        int ret = real_avcodec_receive_frame(avctx, frame);
        if (ret < 0) {
            return ret;
        }

        // Do this garbage to fix an off-by-one in Steam Link caused by changing ffmpeg libararies...
        if (frame->format == AV_PIX_FMT_DRM_PRIME) {
            frame->format = AV_PIX_FMT_OPENCL;
        }
        if (frame->format == AV_PIX_FMT_YUV420P) {
            AVFrame *tmpframe = av_frame_alloc();
            av_frame_move_ref(tmpframe, frame);
            convert2drm(tmpframe, frame);
            av_frame_free(&tmpframe);
        }
        return 0;
    }
    return real_avcodec_receive_frame(avctx, frame);
}

AVBufferRef *av_buffer_ref(const AVBufferRef *buf) {
    if (!real_av_buffer_ref) {
        real_av_buffer_ref = dlsym(RTLD_NEXT, "av_buffer_ref");
    }

    // And this because Steam Link just needs it??????
    if (!buf){
        return 0;
    }

    return real_av_buffer_ref(buf);
}
