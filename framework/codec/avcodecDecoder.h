//
// Created by moqi on 2018/8/10.
//

#ifndef FRAMEWORK_AVCODECDECODER_H
#define FRAMEWORK_AVCODECDECODER_H

#include "codec/ActiveDecoder.h"

#include <mutex>
#include <codec/IDecoder.h>
#include "base/media/AVAFPacket.h"
#include "codecPrototype.h"
#include "utils_windows.h"
extern "C" {
#include <vlc_es.h>
#include <chroma.h>
}

typedef struct cicada_decoder_handle_v_t cicada_decoder_handle_v;

class VideoAcceleration;

namespace Cicada{
    class CICADA_CPLUS_EXTERN avcodecDecoder : public ActiveDecoder, private codecPrototype {
    private:
        struct decoder_handle_v {
            AVCodecContext *codecCont;
            AVCodec *codec;
            AVFrame *avFrame;

            int flags;

			/* VA API */
            video_format_t videoForamt = {0};
            VideoAcceleration *mVA = nullptr;
            AVPixelFormat pix_fmt;
            int profile;
            int level;
			int width;
			int height;
        };
    public:
        avcodecDecoder();

        ~avcodecDecoder() override;

        static bool is_supported(enum AFCodecID codec);

        void setEOF() override
        {}

        void close_va_decoder();

        int getVideoFormat(AVCodecContext *ctx, enum AVPixelFormat pix_fmt, enum AVPixelFormat sw_pix_fmt);

    private:
        explicit avcodecDecoder(int dummy)
        {
            addPrototype(this);
        };

        avcodecDecoder *clone() override
        {
            return new avcodecDecoder();
        };

        bool is_supported(const Stream_meta& meta, uint64_t flags, int maxSize) override
        {
            return is_supported(meta.codec);
        };

        bool is_drmSupport(const DrmInfo *drmInfo) override {
            return false;
        }

        static avcodecDecoder se;

    private:

        int enqueue_decoder(std::unique_ptr<IAFPacket> &pPacket) override;

        int dequeue_decoder(std::unique_ptr<IAFFrame> &pFrame) override;

        int init_decoder(const Stream_meta *meta, void *wnd, uint64_t flags, const DrmInfo *drmInfo) override;

        void close_decoder() override;

        void flush_decoder() override;

        int get_decoder_recover_size() override
        {
            return 0;
        };
        virtual bool supportReuse() override;

        void decoder_updateMetaData(const Stream_meta *meta) override
        {}
    public:
        decoder_handle_v *mPDecoder = nullptr;
		static bool DXNVInteropAvailable;
		static bool DXNVInteropAvailableChecked;
    };
}


#endif //FRAMEWORK_AVCODECDECODER_H
