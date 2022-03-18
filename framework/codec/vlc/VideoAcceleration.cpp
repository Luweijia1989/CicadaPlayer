#include "VideoAcceleration.h"
#include "VADxva.h"
#include <iostream>

VideoAcceleration *VideoAcceleration::createVA(AVCodecContext *ctx, AVPixelFormat fmt)
{
    if (fmt == AV_PIX_FMT_DXVA2_VLD) return new VADxva(ctx, fmt);

    return nullptr;
}

vlc_fourcc_t VideoAcceleration::vlc_va_GetChroma(AVPixelFormat hwfmt, AVPixelFormat swfmt)
{
    /* NOTE: At the time of writing this comment, the return value was only
     * used to probe support as decoder output. So incorrect values were not
     * fatal, especially not if a software format. */
    switch (hwfmt) {
        case AV_PIX_FMT_VAAPI_VLD:
            switch (swfmt) {
                case AV_PIX_FMT_YUVJ420P:
                case AV_PIX_FMT_YUV420P:
                    return VLC_CODEC_VAAPI_420;
                case AV_PIX_FMT_YUV420P10LE:
                    return VLC_CODEC_VAAPI_420_10BPP;
                default:
                    return 0;
            }
        case AV_PIX_FMT_DXVA2_VLD:
            switch (swfmt) {
                case AV_PIX_FMT_YUV420P10LE:
                    return VLC_CODEC_D3D9_OPAQUE_10B;
                default:
                    return VLC_CODEC_D3D9_OPAQUE;
            }
            break;
        case AV_PIX_FMT_D3D11VA_VLD:
            switch (swfmt) {
                case AV_PIX_FMT_YUV420P10LE:
                    return VLC_CODEC_D3D11_OPAQUE_10B;
                default:
                    return VLC_CODEC_D3D11_OPAQUE;
            }
            break;
#if (LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(52, 4, 0))
        case AV_PIX_FMT_VDPAU:
            switch (swfmt) {
                case AV_PIX_FMT_YUVJ444P:
                case AV_PIX_FMT_YUV444P:
                    return VLC_CODEC_VDPAU_VIDEO_444;
                case AV_PIX_FMT_YUVJ422P:
                case AV_PIX_FMT_YUV422P:
                    return VLC_CODEC_VDPAU_VIDEO_422;
                case AV_PIX_FMT_YUVJ420P:
                case AV_PIX_FMT_YUV420P:
                    return VLC_CODEC_VDPAU_VIDEO_420;
                default:
                    return 0;
            }
            break;
#endif
        default:
            return 0;
    }
}

typedef struct ffmpeg_va_ref_t {
    VideoAcceleration *va;
    void *opaque;//va surface from AVFrame.opaque
} ffmpeg_va_ref_t;

static void ffmpeg_release_va_buffer2(void *opaque, uint8_t *data)
{
    ffmpeg_va_ref_t *ref = (ffmpeg_va_ref_t *) opaque;
    ref->va->release(ref->opaque, data);
    delete ref;
}

int VideoAcceleration::getFrame(AVFrame *frame)
{
    if (get(&frame->opaque, &frame->data[0])) {
        std::clog << "hardware acceleration picture allocation failed";
        return -1;
    }
    assert(frame->data[0] != NULL);
    /* data[0] must be non-NULL for libavcodec internal checks.
     * data[3] actually contains the format-specific surface handle. */
    frame->data[3] = frame->data[0];

    ffmpeg_va_ref_t *ref = new ffmpeg_va_ref_t;
    ref->va = this;
    ref->opaque = frame->opaque;
    frame->buf[0] = av_buffer_create(frame->data[0], 0, ffmpeg_release_va_buffer2, ref, 0);
    if (frame->buf[0] == NULL) {
        ffmpeg_release_va_buffer2(ref, frame->data[0]);
        return -1;
    }

    return 0;
}