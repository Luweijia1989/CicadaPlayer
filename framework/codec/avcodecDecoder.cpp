//
// Created by moqi on 2018/8/10.
//
#define LOG_TAG "avcodecDecoder"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
};

#include "avcodecDecoder.h"
#include "base/media/AVAFPacket.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <utils/AFUtils.h>
#include <utils/errors/framework_error.h>
#include <utils/ffmpeg_utils.h>
#include <utils/frame_work_log.h>
#include <utils/mediaFrame.h>

#include "vlc/VideoAcceleration.h"

#define MAX_INPUT_SIZE 4

#ifdef WIN32
/*** CPU ***/
unsigned vlc_GetCPUCount(void)
{
    SYSTEM_INFO systemInfo;

    GetNativeSystemInfo(&systemInfo);

    return systemInfo.dwNumberOfProcessors;
}
#endif

using namespace std;

namespace Cicada {
    static int lavc_GetFrame(struct AVCodecContext *ctx, AVFrame *frame, int flags)
    {
        auto *p_sys = ((avcodecDecoder *) ctx->opaque)->mPDecoder;

        for (unsigned i = 0; i < AV_NUM_DATA_POINTERS; i++) {
            frame->data[i] = NULL;
            frame->linesize[i] = 0;
            frame->buf[i] = NULL;
        }
        frame->opaque = NULL;

        if (p_sys->mVA == NULL) {
            return avcodec_default_get_buffer2(ctx, frame, flags);
        }

        return p_sys->mVA->getFrame(frame);
    }

    static enum AVPixelFormat ffmpeg_GetFormat(AVCodecContext *p_context, const enum AVPixelFormat *pi_fmt)
    {
        auto *p_sys = ((avcodecDecoder *) p_context->opaque)->mPDecoder;

        /* Enumerate available formats */
        enum AVPixelFormat swfmt = avcodec_default_get_format(p_context, pi_fmt);
        bool can_hwaccel = false;

        for (size_t i = 0; pi_fmt[i] != AV_PIX_FMT_NONE; i++) {
            const AVPixFmtDescriptor *dsc = av_pix_fmt_desc_get(pi_fmt[i]);
            if (dsc == NULL) continue;
            bool hwaccel = (dsc->flags & AV_PIX_FMT_FLAG_HWACCEL) != 0;

            AF_LOGD("available %sware decoder output format %d (%s)", hwaccel ? "hard" : "soft", pi_fmt[i], dsc->name);
            if (hwaccel) can_hwaccel = true;
        }

        if (p_sys->pix_fmt == AV_PIX_FMT_NONE) goto no_reuse;

        if (p_sys->width != p_context->coded_width || p_sys->height != p_context->coded_height) {
            AF_LOGD("mismatched dimensions %ux%u was %ux%u", p_context->coded_width, p_context->coded_height, p_sys->width, p_sys->height);
            goto no_reuse;
        }
        if (p_context->profile != p_sys->profile || p_context->level > p_sys->level) {
            AF_LOGD("mismatched profile level %d/%d was %d/%d", p_context->profile, p_context->level, p_sys->profile, p_sys->level);
            goto no_reuse;
        }

        for (size_t i = 0; pi_fmt[i] != AV_PIX_FMT_NONE; i++)
            if (pi_fmt[i] == p_sys->pix_fmt) {
                if (((avcodecDecoder *) p_context->opaque)->getVideoFormat(p_context, p_sys->pix_fmt, swfmt) == 0) {
                    return p_sys->pix_fmt;
                }
            }

    no_reuse:
        ((avcodecDecoder *) p_context->opaque)->close_va_decoder();

        p_sys->profile = p_context->profile;
        p_sys->level = p_context->level;
        p_sys->width = p_context->coded_width;
        p_sys->height = p_context->coded_height;

        if (!can_hwaccel) return swfmt;

#if (LIBAVCODEC_VERSION_MICRO >= 100) && (LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57, 83, 101))
        if (p_context->active_thread_type) {
            AF_LOGD("thread type %d: disabling hardware acceleration", p_context->active_thread_type);
            return swfmt;
        }
#endif

        static const AVPixelFormat hwfmts[] = {
#ifdef _WIN32
            AV_PIX_FMT_D3D11VA_VLD,
            AV_PIX_FMT_DXVA2_VLD,
#endif
            AV_PIX_FMT_VAAPI_VLD,
#if (LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(52, 4, 0))
            AV_PIX_FMT_VDPAU,
#endif
            AV_PIX_FMT_NONE,
        };

        for (size_t i = 0; hwfmts[i] != AV_PIX_FMT_NONE; i++) {
            AVPixelFormat hwfmt = AV_PIX_FMT_NONE;
            for (size_t j = 0; hwfmt == AV_PIX_FMT_NONE && pi_fmt[j] != AV_PIX_FMT_NONE; j++)
                if (hwfmts[i] == pi_fmt[j]) hwfmt = hwfmts[i];

            if (hwfmt == AV_PIX_FMT_NONE) continue;

            p_sys->videoForamt.i_chroma = VideoAcceleration::vlc_va_GetChroma(hwfmt, swfmt);
            if (p_sys->videoForamt.i_chroma == 0) continue;        /* Unknown brand of hardware acceleration */
            if (p_context->width == 0 || p_context->height == 0) { /* should never happen */
                AF_LOGE("unspecified video dimensions");
                continue;
            }
            const AVPixFmtDescriptor *dsc = av_pix_fmt_desc_get(hwfmt);
            AF_LOGD("trying format %s", dsc ? dsc->name : "unknown");

			if (hwfmt == AV_PIX_FMT_DXVA2_VLD && !avcodecDecoder::DXNVInteropAvailable)
				continue;

            auto va = VideoAcceleration::createVA(p_context, hwfmt);
            if (!va) continue;

            if (va->open() != VLC_SUCCESS) {
                delete va;
                continue;
            }

            AF_LOGD("Using %s for hardware decoding", va->description().c_str());

            p_sys->mVA = va;
            p_sys->pix_fmt = hwfmt;
            p_context->draw_horiz_band = NULL;
            ((avcodecDecoder *) p_context->opaque)->getVideoFormat(p_context, hwfmt, swfmt);
            return hwfmt;
        }

        AF_LOGE("Fallback to default behaviour");
        ((avcodecDecoder *) p_context->opaque)->getVideoFormat(p_context, swfmt, swfmt);
        p_sys->pix_fmt = swfmt;
        return swfmt;
    }

    avcodecDecoder avcodecDecoder::se(0);
	bool avcodecDecoder::DXNVInteropAvailable = false;
	bool avcodecDecoder::DXNVInteropAvailableChecked = false;

    void avcodecDecoder::close_va_decoder()
    {
        if (mPDecoder->mVA) {
            mPDecoder->mVA->close();
            delete mPDecoder->mVA;
            mPDecoder->mVA = nullptr;
        }
    }

    void avcodecDecoder::close_decoder()
    {
        if (mPDecoder == nullptr) {
            return;
        }

        if (mPDecoder->codecCont != nullptr) {
            avcodec_close(mPDecoder->codecCont);

            avcodec_free_context(&mPDecoder->codecCont);
            mPDecoder->codecCont = nullptr;
        }
        mPDecoder->codec = nullptr;

        av_frame_free(&mPDecoder->avFrame);

        close_va_decoder();

        delete mPDecoder;
        mPDecoder = nullptr;
    }

    int avcodecDecoder::getVideoFormat(AVCodecContext *ctx, enum AVPixelFormat pix_fmt, enum AVPixelFormat sw_pix_fmt)
    {
        int width = ctx->coded_width;
        int height = ctx->coded_height;
        int aligns[AV_NUM_DATA_POINTERS] = {0};

        video_format_Init(&mPDecoder->videoForamt, 0);

		mPDecoder->videoForamt.decoder_p = this;

        bool software_decoding = pix_fmt == sw_pix_fmt;
        if (software_decoding) { /* software decoding */
            avcodec_align_dimensions2(ctx, &width, &height, aligns);
            mPDecoder->videoForamt.i_chroma = FindVlcChroma(pix_fmt);
        } else /* hardware decoding */
            mPDecoder->videoForamt.i_chroma = VideoAcceleration::vlc_va_GetChroma(pix_fmt, sw_pix_fmt);

        if (width == 0 || height == 0 || width > 8192 || height > 8192 || width < ctx->width || height < ctx->height) {
            AF_LOGE("Invalid frame size %dx%d vsz %dx%d", width, height, ctx->width, ctx->height);
            return -1; /* invalid display size */
        }

        const vlc_chroma_description_t *p_dsc = vlc_fourcc_GetChromaDescription(mPDecoder->videoForamt.i_chroma);
        if (!p_dsc) return VLC_EGENERIC;

        /* We want V (width/height) to respect:
        (V * p_dsc->p[i].w.i_num) % p_dsc->p[i].w.i_den == 0
        (V * p_dsc->p[i].w.i_num/p_dsc->p[i].w.i_den * p_dsc->i_pixel_size) % 16 == 0
		   Which is respected if you have
		   V % lcm( p_dsc->p[0..planes].w.i_den * 16) == 0
		*/

        int tw = ctx->coded_width;
        if (software_decoding) {
            int linesize[4] = {0};
            int unaligned = 0;
            do {
                // NOTE: do not align linesizes individually, this breaks e.g. assumptions
                // that linesize[0] == 2*linesize[1] in the MPEG-encoder for 4:2:2
                av_image_fill_linesizes(linesize, sw_pix_fmt, width);
                width += width & ~(width - 1);

                unaligned = 0;
                for (int i = 0; i < 4; i++) unaligned |= linesize[i] % aligns[i];
            } while (unaligned);
            tw = linesize[0] / p_dsc->pixel_size;
        }

        const int i_width_aligned = tw;
        const int i_height_aligned = ctx->coded_height;
        const int i_height_extra = 0; /* This one is a hack for some ASM functions */

        mPDecoder->videoForamt.i_width = i_width_aligned;
        mPDecoder->videoForamt.i_height = i_height_aligned;
        mPDecoder->videoForamt.i_visible_width = ctx->width;
        mPDecoder->videoForamt.i_visible_height = ctx->height;

        mPDecoder->videoForamt.i_sar_num = ctx->sample_aspect_ratio.num;
        mPDecoder->videoForamt.i_sar_den = ctx->sample_aspect_ratio.den;

        if (mPDecoder->videoForamt.i_sar_num == 0 || mPDecoder->videoForamt.i_sar_den == 0)
            mPDecoder->videoForamt.i_sar_num = mPDecoder->videoForamt.i_sar_den = 1;

        for (unsigned i = 0; i < p_dsc->plane_count; i++) {
            auto *p = &mPDecoder->videoForamt.plane[i];

            p->i_lines = (i_height_aligned + i_height_extra) * p_dsc->p[i].h.num / p_dsc->p[i].h.den;
            p->i_visible_lines =
                    (mPDecoder->videoForamt.i_visible_height + (p_dsc->p[i].h.den - 1)) / p_dsc->p[i].h.den * p_dsc->p[i].h.num;
            p->i_pitch = i_width_aligned * p_dsc->p[i].w.num / p_dsc->p[i].w.den * p_dsc->pixel_size;
            p->i_visible_pitch = (mPDecoder->videoForamt.i_visible_width + (p_dsc->p[i].w.den - 1)) / p_dsc->p[i].w.den *
                                 p_dsc->p[i].w.num * p_dsc->pixel_size;
            p->i_pixel_pitch = p_dsc->pixel_size;

            assert((p->i_pitch % 16) == 0);
        }
        mPDecoder->videoForamt.i_planes = p_dsc->plane_count;

        /* FIXME we should only set the known values and let the core decide
     * later of fallbacks, but we can't do that with a boolean */
        switch (ctx->color_range) {
            case AVCOL_RANGE_JPEG:
                mPDecoder->videoForamt.b_color_range_full = true;
                break;
            case AVCOL_RANGE_UNSPECIFIED:
                mPDecoder->videoForamt.b_color_range_full = !vlc_fourcc_IsYUV(mPDecoder->videoForamt.i_chroma);
                break;
            case AVCOL_RANGE_MPEG:
            default:
                mPDecoder->videoForamt.b_color_range_full = false;
                break;
        }

        switch (ctx->colorspace) {
            case AVCOL_SPC_BT709:
                mPDecoder->videoForamt.space = COLOR_SPACE_BT709;
                break;
            case AVCOL_SPC_SMPTE170M:
            case AVCOL_SPC_BT470BG:
                mPDecoder->videoForamt.space = COLOR_SPACE_BT601;
                break;
            case AVCOL_SPC_BT2020_NCL:
            case AVCOL_SPC_BT2020_CL:
                mPDecoder->videoForamt.space = COLOR_SPACE_BT2020;
                break;
            default:
                break;
        }

        switch (ctx->color_trc) {
            case AVCOL_TRC_LINEAR:
                mPDecoder->videoForamt.transfer = TRANSFER_FUNC_LINEAR;
                break;
            case AVCOL_TRC_GAMMA22:
                mPDecoder->videoForamt.transfer = TRANSFER_FUNC_SRGB;
                break;
            case AVCOL_TRC_BT709:
                mPDecoder->videoForamt.transfer = TRANSFER_FUNC_BT709;
                break;
            case AVCOL_TRC_SMPTE170M:
            case AVCOL_TRC_BT2020_10:
            case AVCOL_TRC_BT2020_12:
                mPDecoder->videoForamt.transfer = TRANSFER_FUNC_BT2020;
                break;
            case AVCOL_TRC_ARIB_STD_B67:
                mPDecoder->videoForamt.transfer = TRANSFER_FUNC_ARIB_B67;
                break;
            case AVCOL_TRC_SMPTE2084:
                mPDecoder->videoForamt.transfer = TRANSFER_FUNC_SMPTE_ST2084;
                break;
            case AVCOL_TRC_SMPTE240M:
                mPDecoder->videoForamt.transfer = TRANSFER_FUNC_SMPTE_240;
                break;
            case AVCOL_TRC_GAMMA28:
                mPDecoder->videoForamt.transfer = TRANSFER_FUNC_BT470_BG;
                break;
            default:
                break;
        }

        switch (ctx->color_primaries) {
            case AVCOL_PRI_BT709:
                mPDecoder->videoForamt.primaries = COLOR_PRIMARIES_BT709;
                break;
            case AVCOL_PRI_BT470BG:
                mPDecoder->videoForamt.primaries = COLOR_PRIMARIES_BT601_625;
                break;
            case AVCOL_PRI_SMPTE170M:
            case AVCOL_PRI_SMPTE240M:
                mPDecoder->videoForamt.primaries = COLOR_PRIMARIES_BT601_525;
                break;
            case AVCOL_PRI_BT2020:
                mPDecoder->videoForamt.primaries = COLOR_PRIMARIES_BT2020;
                break;
            default:
                break;
        }

        switch (ctx->chroma_sample_location) {
            case AVCHROMA_LOC_LEFT:
                mPDecoder->videoForamt.chroma_location = CHROMA_LOCATION_LEFT;
                break;
            case AVCHROMA_LOC_CENTER:
                mPDecoder->videoForamt.chroma_location = CHROMA_LOCATION_CENTER;
                break;
            case AVCHROMA_LOC_TOPLEFT:
                mPDecoder->videoForamt.chroma_location = CHROMA_LOCATION_TOP_LEFT;
                break;
            default:
                break;
        }

        if (mPDecoder->mVA) mPDecoder->videoForamt.extra_info = mPDecoder->mVA->getExtraInfoForRender();

        return 0;
    }

    int avcodecDecoder::init_decoder(const Stream_meta *meta, void *wnd, uint64_t flags, const DrmInfo *drmInfo)
    {
        auto codecId = (enum AVCodecID) CodecID2AVCodecID(meta->codec);
        mPDecoder->codec = avcodec_find_decoder(codecId);
        bool isAudio = meta->channels > 0;

        if (mPDecoder->codec == nullptr) {
            return gen_framework_errno(error_class_codec, isAudio ? codec_error_audio_not_support : codec_error_video_not_support);
        }

        mPDecoder->codecCont = avcodec_alloc_context3((const AVCodec *) mPDecoder->codec);

        if (mPDecoder->codecCont == nullptr) {
            AF_LOGE("init_decoder error");
            return gen_framework_errno(error_class_codec, isAudio ? codec_error_audio_not_support : codec_error_video_not_support);
        }

        if (isAudio) {
            mPDecoder->codecCont->channels = meta->channels;
            mPDecoder->codecCont->sample_rate = meta->samplerate;
        }

        // TODO: not set extradata when XXvc

        if (meta->extradata != nullptr && meta->extradata_size > 0) {
            mPDecoder->codecCont->extradata = (uint8_t *) av_mallocz(meta->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
            memcpy(mPDecoder->codecCont->extradata, meta->extradata, meta->extradata_size);
            mPDecoder->codecCont->extradata_size = meta->extradata_size;
        }

        mPDecoder->flags = DECFLAG_SW;
        if (!isAudio) {
            mPDecoder->codecCont->get_format = ffmpeg_GetFormat;
            mPDecoder->codecCont->get_buffer2 = lavc_GetFrame;
            mPDecoder->codecCont->opaque = this;

            int i_thread_count = vlc_GetCPUCount();
            if (i_thread_count > 1) i_thread_count++;

#if VLC_WINSTORE_APP
            i_thread_count = __MIN(i_thread_count, 6);
#else
            i_thread_count = std::min<int>(i_thread_count, mPDecoder->codec->id == AV_CODEC_ID_HEVC ? 10 : 6);
#endif

            i_thread_count = std::min<int>(i_thread_count, mPDecoder->codec->id == AV_CODEC_ID_HEVC ? 32 : 16);
            AF_LOGD("allowing %d thread(s) for decoding", i_thread_count);
            mPDecoder->codecCont->thread_count = i_thread_count;
            mPDecoder->codecCont->thread_safe_callbacks = true;

            switch (mPDecoder->codec->id) {
                case AV_CODEC_ID_MPEG4:
                case AV_CODEC_ID_H263:
                    mPDecoder->codecCont->thread_type = 0;
                    break;
                case AV_CODEC_ID_MPEG1VIDEO:
                case AV_CODEC_ID_MPEG2VIDEO:
                    mPDecoder->codecCont->thread_type &= ~FF_THREAD_SLICE;
                    /* fall through */
#if (LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55, 1, 0))
                case AV_CODEC_ID_H264:
                case AV_CODEC_ID_VC1:
                case AV_CODEC_ID_WMV3:
                    p_context->thread_type &= ~FF_THREAD_FRAME;
#endif
                default:
                    break;
            }
        } else {
            mPDecoder->codecCont->thread_count = 1;
            mPDecoder->codecCont->thread_safe_callbacks = true;
        }

        av_opt_set_int(mPDecoder->codecCont, "refcounted_frames", 1, 0);

        if (avcodec_open2(mPDecoder->codecCont, mPDecoder->codec, nullptr) < 0) {
            AF_LOGE("could not open codec\n");
            avcodec_free_context(&mPDecoder->codecCont);
            return -1;
        }

        mPDecoder->avFrame = av_frame_alloc();
        return 0;
    }

    avcodecDecoder::avcodecDecoder() : ActiveDecoder()
    {
		if (!DXNVInteropAvailableChecked) {
			DXNVInteropAvailable = checkDxInteropAvailable();
			DXNVInteropAvailableChecked = true;
		}

        mName = "VD.avcodec";
        mPDecoder = new decoder_handle_v();
        memset(mPDecoder, 0, sizeof(decoder_handle_v));
        //        mPDecoderder->dstFormat = AV_PIX_FMT_NONE;

        avcodec_register_all();
        mFlags |= DECFLAG_PASSTHROUGH_INFO;
    }

    avcodecDecoder::~avcodecDecoder()
    {
        close();
    }

    bool avcodecDecoder::is_supported(enum AFCodecID codec)
    {
        //        return codec == AF_CODEC_ID_H264
        //               || codec == AF_CODEC_ID_MPEG4
        //               || codec == AF_CODEC_ID_HEVC
        //               || codec == AF_CODEC_ID_AAC
        //               || codec == AF_CODEC_ID_MP1
        //               || codec == AF_CODEC_ID_MP2
        //               || codec == AF_CODEC_ID_MP3
        //               || codec == AF_CODEC_ID_PCM_S16LE;
        if (avcodec_find_decoder(CodecID2AVCodecID(codec))) {
            return true;
        }

        return false;
    }

    void avcodecDecoder::flush_decoder()
    {
        avcodec_flush_buffers(mPDecoder->codecCont);
    }

    int avcodecDecoder::dequeue_decoder(unique_ptr<IAFFrame> &pFrame)
    {
        int ret = avcodec_receive_frame(mPDecoder->codecCont, mPDecoder->avFrame);

        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                return STATUS_EOS;
            }

            return ret;
        }

        if (mPDecoder->avFrame->decode_error_flags || mPDecoder->avFrame->flags) {
            AF_LOGW("get a error frame\n");
            return -EAGAIN;
        }

        auto dst = mPDecoder->avFrame;
        int64_t timePosition = INT64_MIN;
        int64_t utcTime = INT64_MIN;
        if (mPDecoder->avFrame->metadata){
            AVDictionaryEntry *t = av_dict_get(mPDecoder->avFrame->metadata,"timePosition", nullptr,AV_DICT_IGNORE_SUFFIX);
            if (t){
                timePosition = atoll(t->value);
            }
            AVDictionaryEntry *utcEntry = av_dict_get(mPDecoder->avFrame->metadata, "utcTime", nullptr, AV_DICT_IGNORE_SUFFIX);
            if (utcEntry) {
                utcTime = atoll(utcEntry->value);
            }
        }
        pFrame = unique_ptr<IAFFrame>(new AVAFFrame(&mPDecoder->videoForamt, dst));
        pFrame->getInfo().timePosition = timePosition;
        pFrame->getInfo().utcTime = utcTime;
        return ret;
    };

    int avcodecDecoder::enqueue_decoder(unique_ptr<IAFPacket> &pPacket)
    {
        int ret;
        AVPacket *pkt = nullptr;

        if (pPacket) {
            auto *avAFPacket = dynamic_cast<AVAFPacket *>(pPacket.get());
            assert(avAFPacket);

            if (avAFPacket == nullptr) {
                // TODO: tobe impl
            } else {
                pkt = avAFPacket->ToAVPacket();
                pkt->pts = pPacket->getInfo().pts;
                pkt->dts = pPacket->getInfo().dts;
                assert(pkt != nullptr);
            }
        }

        if (pkt == nullptr) {
            AF_LOGD("send null to decoder\n");
        }

        if (pkt) {
            AVDictionary *dict = nullptr;
            int size = 0;
            av_dict_set_int(&dict,"timePosition",pPacket->getInfo().timePosition,0);
            av_dict_set_int(&dict,"utcTime",pPacket->getInfo().utcTime,0);
            uint8_t *metadata = av_packet_pack_dictionary(dict, &size);
            av_dict_free(&dict);

            if (pPacket->getInfo().extra_data_size > 0) {
                int new_extradata_size;
                const uint8_t *new_extradata = av_packet_get_side_data(pkt, AV_PKT_DATA_NEW_EXTRADATA, &new_extradata_size);
                if (new_extradata == nullptr) {
                    uint8_t *side = av_packet_new_side_data(pkt, AV_PKT_DATA_NEW_EXTRADATA, pPacket->getInfo().extra_data_size);
                    if (side) {
                        memcpy(side, pPacket->getInfo().extra_data, pPacket->getInfo().extra_data_size);
                    }
                }
            }
            int addRet = av_packet_add_side_data(pkt, AV_PKT_DATA_STRINGS_METADATA, metadata, size);
            assert(metadata);
            assert(addRet >= 0);
        }

        ret = avcodec_send_packet(mPDecoder->codecCont, pkt);

        if (0 == ret) {
            pPacket = nullptr;
        } else if (ret == AVERROR(EAGAIN)) {
        } else if (ret == AVERROR_EOF) {
            AF_LOGD("Decode EOF\n");
            ret = 0;
        } else {
            AF_LOGE("Error while decoding frame %d :%s\n", ret, getErrorString(ret));
        }

        return ret;
    }
    bool avcodecDecoder::supportReuse()
    {
        if (mPDecoder->codecCont == nullptr) {
            return true;
        }
        // TODO: check the data format whether changed (avcc adts ...)
        //return mPDecoder->codecCont->extradata_size == 0;
        return false;
    }
}// namespace Cicada
