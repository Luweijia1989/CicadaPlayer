//
// Created by lifujun on 2019/8/29.
//

#ifndef SOURCE_FILTERAUDIORENDER_H
#define SOURCE_FILTERAUDIORENDER_H


#include <queue>

#include <render/audio/IAudioRender.h>
#include <filter/IAudioFilter.h>
#include <utils/afThread.h>

namespace Cicada {
    class filterAudioRender : public IAudioRender {
    public:

        filterAudioRender();

        ~filterAudioRender() override;

        int init(const IAFFrame::audioInfo *info) override;

        int renderFrame(std::unique_ptr<IAFFrame> &frame, int timeOut) override;

        int renderFrame(std::unique_ptr<IAFPacket> &packet, int timeOut) override;

        uint64_t getQueDuration() override;

        int64_t getPosition() override;

        void mute(bool bMute) override;

		void setAudioOutputDevice(uint32_t deviceId) override;

        int setVolume(float volume) override;

        int setSpeed(float speed) override;

        void pause(bool bPause) override;

        void flush() override;

        void prePause() override;

		const IAFFrame::audioInfo &outputInfo() const 
		{
			return mOutputInfo;
		}

    private:
        virtual int init_device() = 0;

        virtual bool device_require_format(const IAFFrame::audioInfo &info) = 0;

        virtual int pause_device() = 0;

        virtual int start_device() = 0;

        virtual void flush_device() = 0;

		virtual void device_change_device(uint32_t deviceId)
        {}

        virtual void device_setVolume(float gain) = 0;

        virtual int device_setSpeed(float speed)
        {
            return -ENOTSUP;
        }

        virtual int64_t device_get_position() = 0;

        virtual int device_write(unique_ptr<IAFFrame> &frame) = 0;

        virtual void device_mute(bool bMute) = 0;

        virtual uint64_t device_get_que_duration()
        {
            return 0;
        }

        virtual int loopChecker()
        {
            return 0;
        };

        virtual void device_preClose()
        {}

        virtual uint64_t device_get_ability() = 0;


    private:
        int renderLoop();

        unique_ptr<IAFFrame> getFrame();

        int pauseThread();

        int startThread();

        int applySpeed();

        int applyVolume();

        void requireSetting();

    protected:
        IAFFrame::audioInfo mInputInfo{};
        IAFFrame::audioInfo mOutputInfo{};
        bool needFilter = false;
        atomic<bool> mRunning{false};

    private:
        volatile std::atomic<float> mSpeed{1};
        std::atomic<float> mFilterSpeed{1};
        std::atomic<int64_t> mSpeedDeltaDuration{0};
        volatile std::atomic<float> mVolume{1};
        std::atomic<float> mFilterVolume{1};
        volatile std::atomic_bool mMute{false};
        std::unique_ptr<Cicada::IAudioFilter> mFilter{};
        std::mutex mFrameQueMutex;
        std::queue<std::unique_ptr<IAFFrame>> mFrameQue{};
        std::unique_ptr<IAFFrame> mRenderFrame{nullptr};
        bool mUseActiveFilter{false};
        std::atomic_int mMaxQueSize{2};
        uint64_t mFilterFlags{};
        bool mSuccessInitDevice = false;

    protected:
        std::unique_ptr<afThread> mRenderThread = nullptr;
    };
}

#endif //SOURCE_FILTERAUDIORENDER_H
