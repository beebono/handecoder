#define _GNU_SOURCE

#include "hdcd-common.h"
#include <libavcodec/avcodec.h>

extern int current_device;

typedef int (*avcodec_open2_func)(AVCodecContext *avctx, const AVCodec *codec, AVDictionary **options);
static avcodec_open2_func real_avcodec_open2 = NULL;

static int is_h264_software_decoder_or_hybrid(const AVCodec *codec) {
    if (codec->id != AV_CODEC_ID_H264) return 0;
    if (codec->capabilities & (AV_CODEC_CAP_HARDWARE | AV_CODEC_CAP_HYBRID)) return 0;
    return 1;
}

int avcodec_open2(AVCodecContext *avctx, const AVCodec *codec, AVDictionary **options) {
    if (!real_avcodec_open2) {
        real_avcodec_open2 = dlsym(RTLD_NEXT, "avcodec_open2");
    }

    if (codec && is_h264_software_decoder_or_hybrid(codec) && current_device != DEVICE_TYPE_NONE) {
        AVCodecContext *new_ctx = avcodec_alloc_context3(NULL);
        new_ctx->opaque = avctx->opaque;
        new_ctx->pkt_timebase = avctx->pkt_timebase;

        avcodec_free_context(&avctx);
        avctx = new_ctx;
        
        if (current_device == DEVICE_TYPE_V4L2) {
            const AVCodec *v4l2_codec = avcodec_find_decoder_by_name("h264_v4l2m2m");
            codec = v4l2_codec;
        } else if (current_device == DEVICE_TYPE_ALLWINNER) {
            const AVCodec *cedrus_codec = avcodec_find_decoder_by_name("h264_cedrus");
            codec = cedrus_codec;
        } else if (current_device == DEVICE_TYPE_ROCKCHIP) {
            const AVCodec *rkmpp_codec = avcodec_find_decoder_by_name("h264_rkmpp");
            codec = rkmpp_codec;
        }
    }
    return real_avcodec_open2(avctx, codec, options);
}