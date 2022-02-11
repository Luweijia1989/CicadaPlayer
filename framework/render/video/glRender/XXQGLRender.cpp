//
// Created by lifujun on 2019/8/12.
//
#define  LOG_TAG "XXQGLRender"

#include "XXQGLRender.h"
#include <utils/timer.h>
#include <utils/AFMediaType.h>
#include <cassert>
#include <cstdlib>
#include <render/video/glRender/base/utils.h>
#include "XXQProgramContext.h"

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
    mClearScreenOn = true;
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
    }

    std::unique_lock<mutex> lock(mRenderCBackMutex);
	if (mRenderCallback)
		mRenderCallback(this);

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
{
    
}

void XXQGLRender::glClearScreen()
{
    glViewport(0, 0, mVideoSurfaceWidth, mVideoSurfaceHeight);
    unsigned int backgroundColor = mBackgroundColor;
    float color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    cicada::convertToGLColor(backgroundColor, color);
    glClearColor(color[0], color[1], color[2], color[3]);
    glClear(GL_COLOR_BUFFER_BIT);
}

void XXQGLRender::captureScreen()
{
    int64_t captureStartTime = af_getsteady_ms();
    GLint pView[4];
    glGetIntegerv(GL_VIEWPORT, pView);
    int width = pView[2];
    int height = pView[3];
    GLsizei bufferSize = width * height * sizeof(GLubyte) * 4; //RGBA
    GLubyte *bufferData = (GLubyte *) malloc(bufferSize);
    memset(bufferData, 0, bufferSize);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glReadPixels(pView[0], pView[1], pView[2], pView[3], GL_RGBA, GL_UNSIGNED_BYTE,
                 bufferData);
    int64_t captureEndTime = af_getsteady_ms();
    AF_LOGD("capture cost time : capture = %d ms", (captureEndTime - captureStartTime));
    mCaptureFunc(bufferData, width, height);
    free(bufferData);
    mCaptureOn = false;
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
        mCreateOutTextureCondition.wait(locker, [this]() -> int {
            return !needCreateOutTexture;
        });
    }

    programContext = getProgram(AF_PIX_FMT_CICADA_MEDIA_CODEC);
    if (programContext) {
        return programContext->getSurface();
    }
#endif
    return nullptr;
}

IProgramContext *XXQGLRender::getProgram(int frameFormat, IAFFrame *frame)
{
    if (mPrograms.count(frameFormat) > 0) {
        IProgramContext *pContext = mPrograms[frameFormat].get();
        pContext->setRenderingCb(mRenderingCb, mRenderingCbUserData);
        pContext->useProgram();
        return pContext;
    }

    unique_ptr<IProgramContext> targetProgram{nullptr};

    if (frameFormat == AF_PIX_FMT_YUV420P || frameFormat == AF_PIX_FMT_YUVJ420P
            || frameFormat == AF_PIX_FMT_YUV422P || frameFormat == AF_PIX_FMT_YUVJ422P) {
        targetProgram = unique_ptr<IProgramContext>(new XXQYUVProgramContext());
    }

    if (targetProgram == nullptr) {
        return nullptr;
    }

	glewInit();
    int ret = targetProgram->initProgram();

    if (ret == 0) {
        targetProgram->setRenderingCb(mRenderingCb, mRenderingCbUserData);
        mPrograms[frameFormat] = move(targetProgram);
        return mPrograms[frameFormat].get();
    } else {
        return nullptr;
    }
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
        return ;
    }

    std::unique_lock<mutex> lock(mRenderCallbackMutex);
    mRenderCallbackCon.wait(lock);
#endif
}

// 在qt渲染线程调用
void XXQGLRender::renderVideo()
{ 
    auto ctx = wglGetCurrentContext();
	if (!ctx)
		return;
	
    //  AF_LOGD("renderActually .");
    int64_t renderStartTime = af_getsteady_ms();

    {
        std::unique_lock<std::mutex> locker(mFrameMutex);

        if (!mInputQueue.empty()) {
            mRenderFrame = move(mInputQueue.front());
            mInputQueue.pop();
        }
    }

    if (mRenderFrame != nullptr) {
        mProgramFormat = mRenderFrame->getInfo().video.format;
        mProgramContext = getProgram(mProgramFormat, mRenderFrame.get());
    }

    if (mProgramContext == nullptr) {
        mProgramFormat = -1;
		return;
    }

    int64_t framePts = INT64_MIN;

    if (mRenderFrame != nullptr) {
        framePts = mRenderFrame->getInfo().pts;
        mVideoInfo = mRenderFrame->getInfo();
        mVideoRotate = getRotate(mRenderFrame->getInfo().video.rotate);
    }

    Rotate finalRotate = Rotate_None;
    int tmpRotate = (mRotate + mVideoRotate) % 360;

    if (tmpRotate == 0) {
        finalRotate = Rotate_None;
    } else if (tmpRotate == 90) {
        finalRotate = Rotate_90;
    } else if (tmpRotate == 180) {
        finalRotate = Rotate_180;
    } else if (tmpRotate == 270) {
        finalRotate = Rotate_270;
    }

    mProgramContext->updateScale(mScale);
    mProgramContext->updateRotate(finalRotate);
    mProgramContext->updateWindowSize(mVideoSurfaceWidth, mVideoSurfaceHeight, false);
    mProgramContext->updateFlip(mFlip);
    mProgramContext->updateBackgroundColor(mBackgroundColor);
    int ret = -1;
    if (mScreenCleared && mRenderFrame == nullptr) {
        //do not draw last frame when need clear screen.
        if (mVideoSurfaceSizeChanged) {
            glClearScreen();
			mVideoSurfaceSizeChanged = false;
        }
    } else {
        mScreenCleared = false;
        ret = mProgramContext->updateFrame(mRenderFrame);
    }
    //work around for glReadPixels is upside-down.
    {
        std::unique_lock<std::mutex> locker(mCaptureMutex);

        if (mCaptureOn && mCaptureFunc != nullptr) {
            //if need capture , update flip and other
            if (mFlip == Flip_None ) {
                mProgramContext->updateFlip(Flip_Vertical);
            } else if (mFlip == Flip_Vertical) {
                mProgramContext->updateFlip(Flip_None);
            } else if ( mFlip == Flip_Horizontal) {
                mProgramContext->updateFlip(Flip_Both);
            }

            if (finalRotate == Rotate_90) {
                mProgramContext->updateRotate(Rotate_270);
            } else if (finalRotate == Rotate_270) {
                mProgramContext->updateRotate(Rotate_90);
            }

            std::unique_ptr<IAFFrame> dummyFrame = nullptr;
            mProgramContext->updateFrame(dummyFrame);
            captureScreen();
            //reset flip and other
            mProgramContext->updateFlip(mFlip);
            mProgramContext->updateRotate(finalRotate);
            mProgramContext->updateFrame(dummyFrame);
        }
    }

    if (ret == 0) {
        //if frame not change, don`t need present surface
        if (mListener) {
            mListener->onFrameInfoUpdate(mVideoInfo, true);
        }
    }

    if (mClearScreenOn) {
        glClearScreen();
        mScreenCleared = true;
        mClearScreenOn = false;
    }

    int64_t end = af_getsteady_ms();

    if (end - renderStartTime > 100) {
        AF_LOGD("renderActually use:%lld", end - renderStartTime);
    }

       AF_LOGD(" cost time : render = %d ms", (af_getsteady_ms() - renderStartTime));
}

void XXQGLRender::setVideoSurfaceSize(int width, int height)
{
	AF_LOGD("enter setVideoSurfaceSize, width = %d, height = %d.", width, height);
	if (mVideoSurfaceWidth == width && mVideoSurfaceHeight == height)
		return;

	mVideoSurfaceWidth = width;
	mVideoSurfaceHeight = height;
	mVideoSurfaceSizeChanged = true;

	// 如果这里宽高都是-1，调用线程在qt渲染线程。opengl ctx正在销毁，rendercallback也要置空。
	if (mVideoSurfaceWidth == -1 && mVideoSurfaceHeight == -1) {
		clearGLResource();
	}
}

void XXQGLRender::setRenderCallback(std::function<void(void * vo_opaque)> cb)
{
	AF_LOGD("enter setRenderCallback.");

	std::unique_lock<mutex> lock(mRenderCBackMutex);
	mRenderCallback = cb;
}

void XXQGLRender::clearGLResource()
{
	auto ctx = wglGetCurrentContext();
	if (!ctx)
		return;

	mPrograms.clear();
}