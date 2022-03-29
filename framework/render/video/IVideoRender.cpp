#include "IVideoRender.h"
#include "glRender/vlc/GLRender.h"

std::map<void *, IVideoRender::RenderInfo> IVideoRender::mRenders = {};
std::mutex IVideoRender::renderMutex;

void IVideoRender::RenderInfo::reset()
{
    render.reset();
    videoFrame.reset();
}

void IVideoRender::foreignGLContextDestroyed()
{
    std::unique_lock<std::mutex> lock(renderMutex);
    mRenders.clear();
}
