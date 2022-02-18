//
// Created by lifujun on 2019/8/12.
//

#ifndef SOURCE_GLRENDER_H
#define SOURCE_GLRENDER_H

#include <render/video/AFActiveVideoRender.h>
#include <mutex>
#include <render/video/vsync/timedVSync.h>
#include <map>
#include <queue>

#include "platform/platform_gl.h"
#include "IProgramContext.h"

#ifdef __APPLE__

#include <TargetConditionals.h>

#endif

using namespace Cicada;


#if TARGET_OS_IPHONE
#include <codec/utils_ios.h>
class XXQGLRender : public IVideoRender, private IVSync::Listener , private IOSNotificationObserver {
#else
class XXQGLRender : public IVideoRender, private IVSync::Listener {
#endif

public:

    explicit XXQGLRender(float Hz = 60);

    ~XXQGLRender() override;

    int init() override;

    int setDisPlay(void *view) override;

    int clearScreen() override;

    void setBackgroundColor(unsigned int color) override;

    int renderFrame(std::unique_ptr<IAFFrame> &frame) override;

    int setRotate(Rotate rotate) override;

    int setFlip(Flip flip) override;

    int setScale(Scale scale) override;

    void setSpeed(float speed) override;

    void captureScreen(std::function<void(uint8_t *, int, int)> func) override;

    void *getSurface(bool cached) override;

    float getRenderFPS() override;

    void surfaceChanged() override;
    uint64_t getFlags() override
    {
        return 0;
    };

	void renderVideo() override;
	void setVideoSurfaceSize(int width, int height) override;
	void setRenderCallback(std::function<void(void * vo_opaque)> cb) override;
	void setMaskMode(MaskMode mode, const std::string& data) override;
	void setVapInfo(const std::string& info) override;

private:
    int onVSync(int64_t tick) override;

	int VSyncOnInit() override;

    void VSyncOnDestroy() override;

#if TARGET_OS_IPHONE

    void AppWillResignActive() override;

    void AppDidBecomeActive() override;

#endif

private:

    void dropFrame();

    void captureScreen();

    void glClearScreen();

	void clearGLResource();

    void calculateFPS(int64_t tick);

    IProgramContext *getProgram(int frameFormat, IAFFrame *frame = nullptr);

    int onVsyncInner(int64_t tick);

protected:

    std::atomic<Rotate> mVideoRotate{Rotate_None};
    std::atomic<Rotate> mRotate{Rotate_None};
    std::atomic<Flip> mFlip{Flip_None};
    std::atomic<Scale> mScale{Scale_AspectFit};
    std::atomic<uint32_t> mBackgroundColor{0xff000000};

	std::mutex mMaskInfoMutex;
	MaskMode mMode{Mask_None};
	std::string mMaskVapData{};
	std::string mMaskVapInfo{};

    int mVideoSurfaceWidth = 0;
    int mVideoSurfaceHeight = 0;
	bool mVideoSurfaceSizeChanged = false;
	std::mutex mRenderCBackMutex;
	std::function<void(void* vo_opaque)> mRenderCallback = nullptr;
	std::unique_ptr<IAFFrame> mRenderFrame = nullptr;

private:
    std::mutex mInitMutex;
    std::mutex mFrameMutex;
    std::queue<std::unique_ptr<IAFFrame>> mInputQueue;
//    std::unique_ptr<IAFFrame> mLastRenderFrame = nullptr;
    std::mutex mViewMutex;
    void *mDisplayView = nullptr;
  /*  Cicada::GLContext *mContext = nullptr;
    Cicada::GLSurface *mGLSurface = nullptr;*/
    std::unique_ptr<IVSync> mVSync = nullptr;
    std::mutex mCaptureMutex;
    bool mCaptureOn = false;
    std::function<void(uint8_t *, int, int)> mCaptureFunc = nullptr;
    std::map<int, std::unique_ptr<IProgramContext>> mPrograms;
    std::mutex mCreateOutTextureMutex;
    std::condition_variable mCreateOutTextureCondition;
    bool needCreateOutTexture = false;
    uint64_t mRenderCount{};
    uint64_t mRendertimeS{0};
    uint8_t mFps{0};
    float mHz{};
    int64_t mVSyncPeriod;
    af_scalable_clock mRenderClock;

    IProgramContext *mProgramContext = nullptr;
    int mProgramFormat = -1;

    bool mClearScreenOn = false;
    bool mScreenCleared = false;
    IAFFrame::AFFrameInfo mVideoInfo{};

    std::atomic_bool bFlushAsync{false};

#ifdef __ANDROID__
    std::mutex mRenderCallbackMutex{};
    std::condition_variable mRenderCallbackCon{};
#endif

};


#endif //SOURCE_GLRENDER_H
