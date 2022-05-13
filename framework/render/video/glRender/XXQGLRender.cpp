//
// Created by lifujun on 2019/8/12.
//
#define LOG_TAG "XXQGLRender"

#include "XXQGLRender.h"
#include "vlc/GLRender.h"
#include <base/media/AVAFPacket.h>
#include <cassert>
#include <cstdlib>
#include <render/video/glRender/base/utils.h>
#include <utils/AFMediaType.h>
#include <utils/timer.h>

using namespace std;

static const int MAX_IN_SIZE = 3;

#include "render/video/vsync/VSyncFactory.h"

using namespace Cicada;

XXQGLRender::XXQGLRender(float Hz)
{
    mVSync = VSyncFactory::create(*this, Hz);
    mHz = 0;
    mVSyncPeriod = static_cast<int64_t>(1000000 / Hz);
}

XXQGLRender::~XXQGLRender()
{
    AF_LOGE("~GLRender");
    // MUST delete Vsync here,because it has callback
    mVSync = nullptr;
}

int XXQGLRender::init()
{
    AF_LOGD("-----> init .");
    // don't auto start in background
    std::unique_lock<std::mutex> locker(mInitMutex);

    mVSync->start();

    return 0;
}

int XXQGLRender::clearScreen()
{
    AF_LOGD("-----> clearScreen");
    std::unique_lock<std::mutex> locker(renderMutex);
    for (auto iter = mRenders.begin(); iter != mRenders.end(); iter++) {
        RenderInfo &info = iter->second;
        info.clearScreen = true;
    }
    return 0;
}

int XXQGLRender::renderFrame(std::unique_ptr<IAFFrame> &frame)
{
    //    AF_LOGD("-----> renderFrame");

    if (frame == nullptr) {
        bFlushAsync = true;
        return 0;
    }

    std::unique_lock<std::mutex> locker(mFrameMutex);
    mInputQueue.push(move(frame));
    return 0;
}

void XXQGLRender::dropFrame()
{
    int64_t framePts = mInputQueue.front()->getInfo().pts;
    AF_LOGI("drop a frame pts = %lld ", framePts);
    mInputQueue.front()->setDiscard(true);
    mInputQueue.pop();
    if (mListener) {
        mListener->onFrameInfoUpdate(mVideoInfo, false);
    }
}

int XXQGLRender::setRotate(IVideoRender::Rotate rotate)
{
    AF_LOGD("-----> setRotate");
    mRotate = rotate;
    return 0;
}

int XXQGLRender::setFlip(IVideoRender::Flip flip)
{
    AF_LOGD("-----> setFlip");
    mFlip = flip;
    return 0;
}

int XXQGLRender::setScale(IVideoRender::Scale scale)
{
    AF_LOGD("-----> setScale");
    mScale = scale;
    return 0;
}


void XXQGLRender::setBackgroundColor(uint32_t color)
{
    mBackgroundColor = color;
};

int XXQGLRender::onVSync(int64_t tick)
{
    int ret = onVsyncInner(tick);
#ifdef __ANDROID__
    {
        unique_lock<mutex> lock(mRenderCallbackMutex);
        mRenderCallbackCon.notify_one();
    }
#endif
    return ret;
}

int XXQGLRender::onVsyncInner(int64_t tick)
{
    if (mHz == 0) {
        mHz = mVSync->getHz();

        if (mHz == 0) {
            mHz = 60;
        }

        mVSyncPeriod = static_cast<int64_t>(1000000 / mHz);
    }

    {
        std::unique_lock<std::mutex> locker(mFrameMutex);

        if (bFlushAsync) {
            while (!mInputQueue.empty()) {
                dropFrame();
            }
            bFlushAsync = false;
			bClearScreen = true;
        }

        if (!mInputQueue.empty()) {
            if (mInputQueue.size() >= MAX_IN_SIZE) {
                while (mInputQueue.size() >= MAX_IN_SIZE) {
                    dropFrame();
                }

                mRenderClock.set(mInputQueue.front()->getInfo().pts);
                mRenderClock.start();
            } else {
                assert(mInputQueue.front() != nullptr);

                if (mRenderClock.get() == 0) {
                    mRenderClock.set(mInputQueue.front()->getInfo().pts);
                    mRenderClock.start();
                }

                int64_t late = mInputQueue.front()->getInfo().pts - mRenderClock.get();

                if (llabs(late) > 100000) {
                    mRenderClock.set(mInputQueue.front()->getInfo().pts);
                } else if (late - mVSyncPeriod * mRenderClock.getSpeed() > 0) {
                    //                    AF_LOGD("mVSyncPeriod is %lld\n", mVSyncPeriod);
                    //                    AF_LOGD("mRenderClock.get() is %lld\n", mRenderClock.get());
                    //                    AF_LOGD("mInputQueue.front()->getInfo().pts is %lld\n", mInputQueue.front()->getInfo().pts);
                    calculateFPS(tick);
                    return 0;
                }
            }
        }

        std::unique_lock<mutex> lock(renderMutex);
        for (auto iter = mRenders.begin(); iter != mRenders.end(); iter++) {
            RenderInfo &renderInfo = iter->second;
            if (renderInfo.cb) renderInfo.cb(this);
        }

        updateRenderFrame = true;
    }

    mRenderCount++;

    calculateFPS(tick);
    return 0;
}

void XXQGLRender::calculateFPS(int64_t tick)
{
    if ((tick / uint64_t(mHz)) != mRendertimeS) {
        if (mRendertimeS == 0 || 1) {
            mRendertimeS = tick / uint64_t(mHz);
        } else {
            mRendertimeS++;
        }

        AF_LOGD("video fps is %llu\n", mRenderCount);
        mFps = mRenderCount;
        mRenderCount = 0;
    }
}

int XXQGLRender::VSyncOnInit()
{
    return 0;
}

void XXQGLRender::VSyncOnDestroy()
{}

void XXQGLRender::captureScreen()
{
    //int64_t captureStartTime = af_getsteady_ms();
    //GLint pView[4];
    //glGetIntegerv(GL_VIEWPORT, pView);
    //int width = pView[2];
    //int height = pView[3];
    //GLsizei bufferSize = width * height * sizeof(GLubyte) * 4;//RGBA
    //GLubyte *bufferData = (GLubyte *) malloc(bufferSize);
    //memset(bufferData, 0, bufferSize);
    //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    //glReadPixels(pView[0], pView[1], pView[2], pView[3], GL_RGBA, GL_UNSIGNED_BYTE, bufferData);
    //int64_t captureEndTime = af_getsteady_ms();
    //AF_LOGD("capture cost time : capture = %d ms", (captureEndTime - captureStartTime));
    //mCaptureFunc(bufferData, width, height);
    //free(bufferData);
    //mCaptureOn = false;
}

int XXQGLRender::setDisPlay(void *view)
{
    AF_LOGD("-----> setDisPlay view = %p", view);

    if (mDisplayView != view) {
        mVSync->pause();
        {
            unique_lock<mutex> viewLock(mViewMutex);
            mDisplayView = view;
        }
        std::unique_lock<std::mutex> locker(mInitMutex);

        mVSync->start();
    }

    return 0;
}

void XXQGLRender::captureScreen(std::function<void(uint8_t *, int, int)> func)
{
    {
        std::unique_lock<std::mutex> locker(mCaptureMutex);
        mCaptureFunc = func;
        mCaptureOn = true;
    }
}

void *XXQGLRender::getSurface(bool cached)
{
#ifdef __ANDROID__
    IProgramContext *programContext = getProgram(AF_PIX_FMT_CICADA_MEDIA_CODEC);

    if (programContext == nullptr || programContext->getSurface() == nullptr || !cached) {
        std::unique_lock<std::mutex> locker(mCreateOutTextureMutex);
        needCreateOutTexture = true;
        mCreateOutTextureCondition.wait(locker, [this]() -> int { return !needCreateOutTexture; });
    }

    programContext = getProgram(AF_PIX_FMT_CICADA_MEDIA_CODEC);
    if (programContext) {
        return programContext->getSurface();
    }
#endif
    return nullptr;
}

void XXQGLRender::setSpeed(float speed)
{
    mRenderClock.setSpeed(speed);
}

float XXQGLRender::getRenderFPS()
{
    return mFps;
}

void XXQGLRender::surfaceChanged()
{
#ifdef __ANDROID__

    if (mInitRet == INT32_MIN || mInitRet != 0) {
        return;
    }

    std::unique_lock<mutex> lock(mRenderCallbackMutex);
    mRenderCallbackCon.wait(lock);
#endif
}

// renderVideo setVideoSurfaceSize clearGLSource foreignGLContextDestroyed should call in same thread
void XXQGLRender::renderVideo(void *vo)
{
    GLRender *glRender = nullptr;
    RenderInfo *info = nullptr;

    int64_t renderStartTime = af_getsteady_ms();

    {
        std::unique_lock<std::mutex> locker(mFrameMutex);
        std::unique_lock<std::mutex> renderLocker(renderMutex);
        info = &mRenders[vo];
        if (updateRenderFrame && !mInputQueue.empty()) {
            std::shared_ptr<IAFFrame> frame = move(mInputQueue.front());
            mInputQueue.pop();
            updateRenderFrame = false;
            {
                for (auto iter = mRenders.begin(); iter != mRenders.end(); iter++) {
                    RenderInfo &i = iter->second;
                    i.frame = frame;
                }
            }
        }

		for (auto iter = mRenders.begin(); iter != mRenders.end(); iter++) {
            RenderInfo &i = iter->second;
			i.clearLastRenderFrame = bClearScreen;
        }
		if (bClearScreen)
			bClearScreen = false;
    }

    std::shared_ptr<IAFFrame> mRenderFrame = std::move(info->frame);
    if (!mRenderFrame) {
        if (info->clearLastRenderFrame && info->render) info->render->clearScreen(mBackgroundColor);
        return;
    }

    if (!info->render) {
        std::unique_ptr<GLRender> render = std::make_unique<GLRender>((video_format_t *) mRenderFrame->getInfo().video.vlc_fmt);
        if (render->initGL()) info->render = move(render);
    } else {
        if (info->render->videoFormatChanged((video_format_t *) mRenderFrame->getInfo().video.vlc_fmt)) {
            info->render.reset();

            std::unique_ptr<GLRender> render = std::make_unique<GLRender>((video_format_t *) mRenderFrame->getInfo().video.vlc_fmt);
            if (render->initGL()) info->render = move(render);
        }
    }

    glRender = info->render.get();

    if (glRender == nullptr) return;

    if (info->surfaceSizeChanged) {
        glRender->clearScreen(mBackgroundColor);
        info->surfaceSizeChanged = false;
    }

    auto frame = ((AVAFFrame *) mRenderFrame.get())->ToAVFrame();
    auto videoInfo = mRenderFrame->getInfo().video;

    mMaskInfoMutex.lock();
    int ret = glRender->displayGLFrame(mMaskVapInfo, mMode, mMaskVapData, frame, mRenderFrame->getInfo().video.frameIndex, mRotate, mScale,
                                       mFlip, (video_format_t *) videoInfo.vlc_fmt, info->surfaceWidth, info->surfaceHeight);
    mMaskInfoMutex.unlock();


    if (ret == 0) {
        //if frame not change, don`t need present surface
        if (mListener) {
            mVideoInfo = mRenderFrame->getInfo();
            mListener->onFrameInfoUpdate(mVideoInfo, true);
        }
    }

    {
        std::unique_lock<std::mutex> renderLocker(renderMutex);
        if (info->clearScreen) {
            glRender->clearScreen(mBackgroundColor);
            info->clearScreen = false;
        }
    }

    int64_t end = af_getsteady_ms();

    if (end - renderStartTime > 100) {
        AF_LOGD("renderActually use:%lld", end - renderStartTime);
    }

    AF_LOGD(" cost time : render = %d ms", (af_getsteady_ms() - renderStartTime));
}

void XXQGLRender::setVideoSurfaceSize(int width, int height, void *vo)
{
    std::unique_lock<mutex> lock(renderMutex);

    RenderInfo &renderInfo = mRenders[vo];
    if (renderInfo.surfaceWidth == width && renderInfo.surfaceHeight == height) return;

    renderInfo.surfaceWidth = width;
    renderInfo.surfaceHeight = height;
    renderInfo.surfaceSizeChanged = true;
}

void XXQGLRender::setRenderCallback(std::function<void(void *vo_opaque)> cb, void *vo)
{
    AF_LOGD("enter setRenderCallback.");

    std::unique_lock<mutex> lock(renderMutex);

    RenderInfo &renderInfo = mRenders[vo];
    renderInfo.cb = cb;
}

void XXQGLRender::clearGLResource(void *vo)
{
    std::unique_lock<mutex> lock(renderMutex);

    RenderInfo &renderInfo = mRenders[vo];
    renderInfo.reset();
}

void XXQGLRender::setMaskMode(MaskMode mode, const std::string &data)
{
    std::unique_lock<std::mutex> locker(mMaskInfoMutex);
    mMode = mode;
    mMaskVapData = data;
}

void XXQGLRender::setVapInfo(const std::string &info)
{
    std::unique_lock<std::mutex> locker(mMaskInfoMutex);
    mMaskVapInfo = info;
}