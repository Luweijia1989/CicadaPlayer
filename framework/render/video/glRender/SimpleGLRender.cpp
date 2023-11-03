#define LOG_TAG "SimpleGLRender"

#include "SimpleGLRender.h"
#include "vlc/GLRender.h"
#include <base/media/AVAFPacket.h>
#include <cassert>
#include <cstdlib>
#include <render/video/glRender/base/utils.h>
#include <utils/AFMediaType.h>
#include <utils/timer.h>

using namespace std;

std::map<void *, SimpleGLRender::RenderInfo> SimpleGLRender::mRenders = {};

SimpleGLRender::SimpleGLRender()
{

}

SimpleGLRender::~SimpleGLRender()
{
	AF_LOGE("~GLRender");
}

void SimpleGLRender::renderVideo(void *vo, AVFrame *frame, unsigned int fbo_id)
{
	RenderInfo &renderInfo = mRenders[vo];
	if (!frame) {
		if (renderInfo.render) renderInfo.render->clearScreen(0xff000000);
		return;
	}

	auto vfmt = (video_format_t *)frame->opaque;
	if (!renderInfo.render) {
		std::unique_ptr<GLRender> render = std::make_unique<GLRender>(vfmt);
		if (render->initGL()) renderInfo.render = move(render);
	}
	else {
		if (renderInfo.render->videoFormatChanged(vfmt)) {
			renderInfo.render.reset();

			std::unique_ptr<GLRender> render = std::make_unique<GLRender>(vfmt);
			if (render->initGL()) renderInfo.render = move(render);
		}
	}

	auto glRender = renderInfo.render.get();

	if (glRender == nullptr) return;

	glRender->setExternalFboId(fbo_id);
	glRender->clearScreen(0xff000000);

	mMaskInfoMutex.lock();
	int ret = glRender->displayGLFrame(mMaskVapInfo, mMode, mMaskVapData, frame, /*mRenderFrame->getInfo().video.frameIndex*/0, IVideoRender::Rotate::Rotate_None, IVideoRender::Scale::Scale_AspectFit,
		IVideoRender::Flip::Flip_None, vfmt, renderInfo.surfaceWidth, renderInfo.surfaceHeight);
	mMaskInfoMutex.unlock();
}

void SimpleGLRender::setVideoSurfaceSize(void *vo, int width, int height)
{
	RenderInfo &renderInfo = mRenders[vo];
	if (renderInfo.surfaceWidth == width && renderInfo.surfaceHeight == height) return;

	renderInfo.surfaceWidth = width;
	renderInfo.surfaceHeight = height;
}

void SimpleGLRender::setMaskMode(IVideoRender::MaskMode mode, const std::string &data)
{
	std::unique_lock<std::mutex> locker(mMaskInfoMutex);
	mMode = mode;
	mMaskVapData = data;
}

void SimpleGLRender::setVapInfo(const std::string &info)
{
	std::unique_lock<std::mutex> locker(mMaskInfoMutex);
	mMaskVapInfo = info;
}

void clearGL(std::map<void *, SimpleGLRender::RenderInfo> &renders, void *vo)
{
	SimpleGLRender::RenderInfo &renderInfo = renders[vo];
	renderInfo.reset();
	renders.erase(vo);
}

void SimpleGLRender::clearGLResource(void *vo)
{
	clearGL(mRenders, vo);
}

void SimpleGLRender::foreignGLContextDestroyed(void *vo)
{
	clearGL(mRenders, vo);
}