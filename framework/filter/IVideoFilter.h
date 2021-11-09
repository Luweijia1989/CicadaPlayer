//
// Created by moqi on 2020/5/28.
//

#ifndef CICADAMEDIA_IVIDEOFILTER_H
#define CICADAMEDIA_IVIDEOFILTER_H
#include <base/media/IAFPacket.h>
#include <memory>
#include <string>
#include <utils/AFMediaType.h>

namespace Cicada {

    const static std::string FILTER_INVALID_PERFORMANCE = "filter stop due to poor device performance";
    const static std::string FILTER_INVALID_OVER_FPS = "filter stop due to fps is too big";
    const static std::string FILTER_VALID_RECOVERY = "filter recovery";
    class DCACallback {
    protected:
        void *mUserData;

    public:
        explicit DCACallback(void *userData) : mUserData(userData){};

        virtual void send(std::string target, int level, std::string content) = 0;
    };

    class CICADA_CPLUS_EXTERN IVideoFilter {

    public:
        enum Feature {
            None,
            PassThrough = 1, // support do not do filter
            Buffer = 1 << 1, //buffer after decode
            Texture = 1 << 2,// texture before render
            HDR = 1 << 3 , // HDR viedeo buffer
        };

    public:
        IVideoFilter(const IAFFrame::videoInfo &srcFormat, const IAFFrame::videoInfo &dstFormat, bool active)
            : mSrcFormat(srcFormat), mDstFormat(dstFormat), mActive(active){};
        IVideoFilter(){};
        virtual ~IVideoFilter() = default;

        //        virtual bool beSupported(const char* capacity) = 0;
        virtual bool setOption(const std::string &key, const std::string &value, const std::string &capacity) = 0;

        /**
         * init filter.
         * @param type  TEXTURE_YUV: 0, TEXTURE_RGBA:1.
         * @return
         */
        virtual bool init(int type) = 0;

        /**
         *
         * @param frame
         * @param timeOut
         * @return
         * Success: >=0
         * Fail: < 0
         */
        virtual int push(std::unique_ptr<IAFFrame> &frame, uint64_t timeOut) = 0;

        /**
         *
         * @param frame
         * @param timeOut
         * @return
         * Success: >=0
         * Fail: < 0
         */
        virtual int pull(std::unique_ptr<IAFFrame> &frame, uint64_t timeOut) = 0;

        virtual void flush() = 0;

        virtual std::string getName() = 0;

        virtual bool is_supported(const std::string &target, int width, int height, int format) = 0;

        virtual Cicada::IVideoFilter *clone(IAFFrame::videoInfo srcFormat, IAFFrame::videoInfo dstFormat, bool active) = 0;

        virtual bool isFeatureSupported(Feature feature) = 0;

        virtual void setInvalid(bool invalid)
        {
            mInvalid = invalid;
        }

        virtual bool isInvalid()
        {
            return mInvalid;
        }

        void setSpeed(float speed)
        {
            mSpeed = speed;
        }

        virtual void clearBuffer()
        {
            mClearFlag = true;
            flush();
        }

        virtual void setDCACb(DCACallback *dcaCallback)
        {
            this->mDcaCallback = dcaCallback;
        }

        virtual void setCurrentTarget(const std::string &target)
        {
            currentTarget = target;
        }

    protected:
        virtual bool isOverFps(IAFFrame *frame)
        {
            return false;
        };

        IAFFrame::videoInfo mSrcFormat{};
        IAFFrame::videoInfo mDstFormat{};
        bool mInvalid{false};
        bool mActive{false};
        float mSpeed = 1.0;
        bool mClearFlag{false};
        std::string currentTarget;
        DCACallback *mDcaCallback;
    };
}// namespace Cicada


#endif//CICADAMEDIA_IVIDEOFILTER_H
