#include "IVideoRender.h"
#include "glRender/vlc/GLRender.h"

std::map<void *, std::map<int, std::unique_ptr<GLRender>>> IVideoRender::mRenders = {};

void IVideoRender::foreignGLContextDestroyed()
{
    mRenders.clear();
}
