//
// Created by moqi on 2018/8/10.
//
#define LOG_TAG "avcodecDecoder"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavcodec/avcodec.h>
};

#include <utils/AFUtils.h>
#include <cstring>
#include <cstdlib>
#include <utils/frame_work_log.h>
#include <utils/mediaFrame.h>
#include <utils/ffmpeg_utils.h>
#include <cassert>
#include <deque>
#include "avcodecDecoder.h"
#include "base/media/AVAFPacket.h"
#include <utils/errors/framework_error.h>

#include "vlc/VideoAcceleration.h"

#define  MAX_INPUT_SIZE 4

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
            AF_LOGD("mismatched dimensions %ux%u was %ux%u", p_context->coded_width, p_context->coded_height, p_sys->width,
                    p_sys->height);
            goto no_reuse;
        }
        if (p_context->profile != p_sys->profile || p_context->level > p_sys->level) {
            AF_LOGD("mismatched profile level %d/%d was %d/%d", p_context->profile, p_context->level, p_sys->profile, p_sys->level);
            goto no_reuse;
        }

        for (size_t i = 0; pi_fmt[i] != AV_PIX_FMT_NONE; i++)
            if (pi_fmt[i] == p_sys->pix_fmt) {
                return p_sys->pix_fmt;
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

            auto i_chroma = VideoAcceleration::vlc_va_GetChroma(hwfmt, swfmt);
            if (i_chroma == 0) continue;      /* Unknown brand of hardware acceleration */
            if (p_context->width == 0 || p_context->height == 0) { /* should never happen */
                AF_LOGE("unspecified video dimensions");
                continue;
            }
            const AVPixFmtDescriptor *dsc = av_pix_fmt_desc_get(hwfmt);
            AF_LOGD("trying format %s", dsc ? dsc->name : "unknown");
            
			auto va = VideoAcceleration::createVA(p_context, hwfmt);
			if (!va)
				continue;

			if (va->open() != VLC_SUCCESS) {
				delete va;
				continue;
			}

			AF_LOGD("Using %s for hardware decoding", va->description().c_str());

            p_sys->mVA = va;
            p_sys->pix_fmt = hwfmt;
            p_context->draw_horiz_band = NULL;
            return hwfmt;
        }

        /* Fallback to default behaviour */
        p_sys->pix_fmt = swfmt;
        return swfmt;
    }

    avcodecDecoder avcodecDecoder::se(0);

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

    int avcodecDecoder::init_decoder(const Stream_meta *meta, void *wnd, uint64_t flags,
                                     const DrmInfo *drmInfo)
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

        // TODO: not set extradata when XXvc

        if (AF_CODEC_ID_PCM_S16LE == meta->codec) {
            mPDecoder->codecCont->channels = meta->channels;
            mPDecoder->codecCont->sample_rate = meta->samplerate;
        }

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
        mPDecoder->vInfo.height = mPDecoder->codecCont->height;
        mPDecoder->vInfo.width = mPDecoder->codecCont->width;
        mPDecoder->vInfo.pix_fmt = mPDecoder->codecCont->pix_fmt;
        return 0;
    }


    avcodecDecoder::avcodecDecoder() : ActiveDecoder()
    {
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
        if (dst->metadata){
            AVDictionaryEntry *t = av_dict_get(dst->metadata,"timePosition", nullptr,AV_DICT_IGNORE_SUFFIX);
            if (t){
                timePosition = atoll(t->value);
            }
        }
        pFrame = unique_ptr<IAFFrame>(new AVAFFrame(dst));
        pFrame->getInfo().timePosition = timePosition;
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

        if (pkt){
            AVDictionary *dict = nullptr;
            int size = 0;
            av_dict_set_int(&dict,"timePosition",pPacket->getInfo().timePosition,0);
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
            AF_LOGE("Error while decoding frame %d :%s\n", ret,  getErrorString(ret));
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
}
