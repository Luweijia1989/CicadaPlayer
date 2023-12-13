#define LOG_TAG "SimpleGLRender"

#include "SimpleGLRender.h"
#include "vlc/GLRender.h"
#include <base/media/AVAFPacket.h>
#include <cassert>
#include <cstdlib>
#include <render/video/glRender/base/utils.h>
#include <utils/AFMediaType.h>
#include <utils/timer.h>
#include <codec/SimpleDecoder.h>

using namespace std;

std::map<RenderKey, SimpleGLRender::RenderInfo> SimpleGLRender::mRenders = {};
std::string SimpleGLRender::referenceTag = "tag1";
int SimpleGLRender::referenceIndex =0;
std::condition_variable SimpleGLRender::m_cv;
std::mutex SimpleGLRender::m_cv_mutex;

SimpleGLRender::SimpleGLRender()
{

}

SimpleGLRender::~SimpleGLRender()
{
	AF_LOGE("~SimpleGLRender [%s]",m_sourceTag.c_str());
}

void SimpleGLRender::setVideoTag(const std::string &tag)
{
    AF_LOGI("setVideoTag : %", tag.c_str());
    m_sourceTag = tag;
}

void SimpleGLRender::enableHWDecoder(bool hw)
{
    AF_LOGI("enableHardWareDecoder %d", hw);
    m_enableHW = hw;
}

void SimpleGLRender::renderVideo(void *vo, AVFrame *frame, unsigned int fbo_id)
{
    //AF_LOGI("before renders: %d [%s]", mRenders.size(),m_sourceTag.c_str());
	RenderInfo &renderInfo = mRenders[{vo,m_sourceTag}];
	if (!frame) {
		if (renderInfo.render) renderInfo.render->clearScreen(0);
		return;
	}

	auto vfmInfo = (video_frame_info *)frame->opaque;
	auto vfmt = &vfmInfo->format;
    auto frameIndex = vfmInfo->index;

	//frame internal sync
    if (referenceTag != m_sourceTag) {
        std::unique_lock<std::mutex> lock(m_cv_mutex);
        m_cv.wait_for(lock,15ms, [=] { return referenceIndex == frameIndex; });
	}
    if (referenceTag == m_sourceTag) {
        referenceIndex = frameIndex;
        m_cv.notify_all();
    }

	if (!renderInfo.render) {
		AF_LOGI("Implement GL [%s]", m_sourceTag.c_str());
		std::shared_ptr<GLRender> render = std::make_shared<GLRender>(vfmt);
		if (render->initGL()) renderInfo.render = render;

		if (!m_enableHW) {
            for (auto &item : mRenders) {
                if (!item.second.render) {
                    item.second.render = render;
                }
            }
		}
	}
	else {
		if (renderInfo.render->videoFormatChanged(vfmt)) {
			renderInfo.render.reset();

			std::shared_ptr<GLRender> render = std::make_shared<GLRender>(vfmt);
			if (render->initGL()) renderInfo.render = render;
		}
	}

	auto glRender = renderInfo.render.get();

	if (glRender == nullptr) return;

	glRender->setExternalFboId(fbo_id);
	glRender->clearScreen(0);

	AF_LOGD("after tagName: %s,frameIndex: %d,render:%p", m_sourceTag.c_str(), frameIndex, glRender);

	mMaskInfoMutex.lock();
	int ret = glRender->displayGLFrame(mMaskVapInfo, mMode, mMaskVapData, frame, vfmInfo->index, IVideoRender::Rotate::Rotate_None, IVideoRender::Scale::Scale_AspectFit,
		IVideoRender::Flip::Flip_None, vfmt, renderInfo.surfaceWidth, renderInfo.surfaceHeight);
	mMaskInfoMutex.unlock();
}

void SimpleGLRender::setVideoSurfaceSize(void *vo, int width, int height)
{
    AF_LOGI("setVideoSurfaceSize width: %d,height: %d [%s]", width, height, m_sourceTag.c_str());
	RenderInfo &renderInfo = mRenders[{vo,m_sourceTag}];
    renderInfo.sourceTag = m_sourceTag;
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

void clearGL(std::map<RenderKey, SimpleGLRender::RenderInfo> &renders, void *vo,const std::string& tag)
{
    AF_LOGI("clearGL %s",tag.c_str());
	SimpleGLRender::RenderInfo &renderInfo = renders[{vo,tag}];
	renderInfo.reset();
	renders.erase(vo);
}

void SimpleGLRender::clearGLResource(void *vo)
{
    AF_LOGI("clearGLResource");
	clearGL(mRenders, vo,m_sourceTag);
}

void SimpleGLRender::foreignGLContextDestroyed(void *vo, const std::string &tag)
{
    AF_LOGI("foreignGLContextDestroyed");
	clearGL(mRenders, vo,tag);
}