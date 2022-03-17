#pragma once

#include <cstdint>
#include <string>
#include <vlc_common.h>
#include <vlc_fourcc.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/pixfmt.h>
}

class VideoAcceleration {
public:
    VideoAcceleration(AVCodecContext *ctx, AVPixelFormat fmt) : mCtx(ctx), mFmt(fmt)
    {}

	static VideoAcceleration *createVA(AVCodecContext *ctx, AVPixelFormat fmt);

    static vlc_fourcc_t vlc_va_GetChroma(AVPixelFormat hwfmt, AVPixelFormat swfmt);

    virtual std::string description() = 0;
    virtual int get(void **opaque, uint8_t **data) = 0;
    virtual void release(void **opaque, uint8_t **data) = 0;

public:
    AVCodecContext *mCtx = nullptr;
    AVPixelFormat mFmt = AV_PIX_FMT_NONE;
};