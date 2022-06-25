#include "IVideoRender.h"
#include "glRender/vlc/GLRender.h"

std::map<void *, IVideoRender::RenderInfo> IVideoRender::mRenders = {};
std::mutex IVideoRender::renderMutex;

void IVideoRender::RenderInfo::reset()
{
	if (render)
		render.reset();
}

void IVideoRender::foreignGLContextDestroyed(void *vo)
{
    std::unique_lock<std::mutex> lock(renderMutex);

	RenderInfo &info = mRenders[vo];
    info.reset();

	if (!info.cb)
		mRenders.erase(vo);
}
