//
// Created by lifujun on 2019/8/20.
//

#ifndef SOURCE_IPROGRAMCONTEXT_H
#define SOURCE_IPROGRAMCONTEXT_H

#include "platform/platform_gl.h"
#include "render/video/IVideoRender.h"
#include <utils/AFMediaType.h>

class IProgramContext {

public:
    virtual ~IProgramContext() = default;

    virtual int initProgram(AFPixelFormat format)
    {
        return 0;
    }

    virtual void useProgram()
    {}

    virtual void createSurface()
    {}

    virtual void *getSurface()
    {
        return nullptr;
    };

    virtual void updateScale(IVideoRender::Scale scale) = 0;

    virtual void updateFlip(IVideoRender::Flip flip) = 0;

    virtual void updateRotate(IVideoRender::Rotate rotate) = 0;

    virtual void updateBackgroundColor(uint32_t color)
    {}

    virtual void updateWindowSize(int width, int height, bool windowChanged) = 0;

    virtual int updateFrame(std::unique_ptr<IAFFrame> &frame) = 0;

    virtual void setRenderingCb(videoRenderingFrameCB cb, void *userData)
    {
        mRenderingCb = cb;
        mRenderingCbUserData = userData;
    }

    virtual void setGLContext(void *glContext)
    {
        mGLContext = glContext;
    }

    virtual void updateMaskInfo(const std::string &vapInfo, IVideoRender::MaskMode mode, const std::string &data)
    {}

    virtual void clearScreen(uint32_t color)
    {}

protected:
    videoRenderingFrameCB mRenderingCb{nullptr};
    void *mRenderingCbUserData{nullptr};

    void *mGLContext{nullptr};
};


#endif//SOURCE_IPROGRAMCONTEXT_H
