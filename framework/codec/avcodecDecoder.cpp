//
// Created by moqi on 2018/8/10.
//
#define LOG_TAG "avcodecDecoder"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
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

#define  MAX_INPUT_SIZE 4

using namespace std;

namespace Cicada {
#ifdef ENABLE_HWDECODER
	static AVBufferRef *hw_device_ctx = NULL;
    static enum AVPixelFormat hw_pix_fmt;
    static int hw_decoder_init(AVCodecContext *ctx, const enum AVHWDeviceType type)
    {
        int err = 0;

        if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type, NULL, NULL, 0)) < 0) {
            AF_LOGE("Failed to create specified HW device.");
            return err;
        }
        ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

        return err;
    }

    static enum AVPixelFormat get_hw_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts)
    {
        const enum AVPixelFormat *p;

        for (p = pix_fmts; *p != -1; p++) {
            if (*p == hw_pix_fmt) return *p;
        }

        AF_LOGE("Failed to get HW surface format");
        return AV_PIX_FMT_NONE;
    }
#endif

    avcodecDecoder avcodecDecoder::se(0);

    void avcodecDecoder::close_decoder()
    {
        if (mPDecoder == nullptr) {
            return;
        }

        if (mPDecoder->codecCont != nullptr) {
            avcodec_close(mPDecoder->codecCont);

            avcodec_free_context(&mPDecoder->codecCont);
            mPDecoder->codecCont = nullptr;

#ifdef ENABLE_HWDECODER
			if (hw_device_ctx)
				av_buffer_unref(&hw_device_ctx);
#endif
        }

        mPDecoder->codec = nullptr;
        av_frame_free(&mPDecoder->avFrame);
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
#ifdef ENABLE_HWDECODER
        if (flags & DECFLAG_HW) {
			const char *hw_name = "dxva2";
			auto type = av_hwdevice_find_type_by_name(hw_name);
			if (type != AV_HWDEVICE_TYPE_NONE) {
                for (auto i = 0;; i++) {
                    const AVCodecHWConfig *config = avcodec_get_hw_config(mPDecoder->codec, i);
                    if (!config) {
                        AF_LOGE("Decoder %s does not support device type %s.", mPDecoder->codec->name, av_hwdevice_get_type_name(type));
                        break;
                    }
                    if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && config->device_type == type) {
                        hw_pix_fmt = config->pix_fmt;
                        break;
                    }
                }

				if (hw_pix_fmt != AV_PIX_FMT_NONE) {
					mPDecoder->codecCont->get_format = get_hw_format;
					if (hw_decoder_init(mPDecoder->codecCont, type) < 0)
						return -1;
				}
			}
        }
#endif
        av_opt_set_int(mPDecoder->codecCont, "refcounted_frames", 1, 0);
        int threadcount = (AFGetCpuCount() > 0 ? AFGetCpuCount() + 1 : 0);

        if ((flags & DECFLAG_OUTPUT_FRAME_ASAP)
                && ((0 == threadcount) || (threadcount > 2))) {
            // set too much thread need more video buffer in ffmpeg
            threadcount = 2;
        }

        AF_LOGI("set decoder thread as :%d\n", threadcount);
        mPDecoder->codecCont->thread_count = threadcount;

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
#ifdef ENABLE_HWDECODER
		if (mPDecoder->avFrame->format == hw_pix_fmt) {
			if (!mPDecoder->swFrame)
				mPDecoder->swFrame = av_frame_alloc();

			av_hwframe_transfer_data(mPDecoder->swFrame, mPDecoder->avFrame, 0);
			av_frame_copy_props(mPDecoder->swFrame, mPDecoder->avFrame);
			dst = mPDecoder->swFrame;
		}
#endif
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
