#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libavutil/imgutils.h>
#include <libavcodec/codec.h>
}

#define QTAV_PIX_FMT_C(X) AV_PIX_FMT_##X