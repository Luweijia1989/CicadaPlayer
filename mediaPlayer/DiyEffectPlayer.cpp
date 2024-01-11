#define LOG_TAG "DiyEffectPlayer"
#include "DiyEffectPlayer.h"
#include "DiyImplement.h"
#include <codec/SimpleDecoder.h>
#include <data_source/vapparser/BinaryFileStream.hpp>
#include <render/video/glRender/SimpleGLRender.h>
#include <utils/timer.h>
using namespace std::chrono_literals;
extern "C" {
#include <libavformat/avformat.h>
}

DiyEffectPlayer::DiyEffectPlayer()
{

}
DiyEffectPlayer ::~DiyEffectPlayer()
{

}

int DiyEffectPlayer::SetListener(const playerListener &Listener)
{
    AF_LOGI("SetListener");
    m_listener = Listener;

    return 0;
}

void DiyEffectPlayer::EnableHardwareDecoder(bool enable)
{
    m_hwEnable = enable;
}

void DiyEffectPlayer::start(const std::list<std::string> &path)
{
    AF_LOGI("========start==========");
    if (m_started) return;

	int index = 0;
	for (const auto item :path){
        index++;
        std::string sourceTag = m_rootTag + std::to_string(index);
        if (m_keyTag.empty()) m_keyTag = sourceTag;
        m_mapPath[sourceTag] = item;
	}

	playerListener listener{nullptr};
    listener.userData = this;
    listener.Completion = completionCallback;
    listener.FirstFrameShow = firstFrameCallback;
    listener.VideoSizeChanged = videoSizeChangedCallback;
    listener.VideoDecode = videoDecodeCallback;

    for (const auto item: m_mapPath) {
        DiyImplement *pDiy = new DiyImplement();
        if (pDiy) {
            pDiy->setSourceTag(item.first);
            pDiy->setSurfaceSize(m_viewVo, m_viewWidth, m_viewHeight);
            pDiy->setSmoothLoop(m_smoothLoop);
            pDiy->EnableHardwareDecoder(m_hwEnable);
            pDiy->SetListener(listener);

            pDiy->start(item.second);
            m_mapDiyImp[item.first] = pDiy;
		}
	}

    m_videoStage = VideoPlayerStage::STAGE_PLAYING;
    m_started = true;
}
void DiyEffectPlayer::stop()
{
    if (!m_started) return;
	{
        std::lock_guard<std::mutex> autoLock(m_DiyMutex);
        for (const auto item : m_mapDiyImp) {
            item.second->stop();
            delete item.second;
		}
	}

	m_mapPath.clear();
    m_started = false;
    m_keyIndex = 0;
    m_keyTag.clear();
    m_videoStage = VideoPlayerStage::STAGE_STOPED;
}

void DiyEffectPlayer::setSmoothLoop(bool enable)
{
    m_smoothLoop = enable;
}

void DiyEffectPlayer::setRootTag(const std::string &tag)
{
    AF_LOGI("setRootTag : %s", tag.c_str());
    m_rootTag = tag;
}
void DiyEffectPlayer::setSurfaceSize(void *vo, int w, int h)
{
    AF_LOGI("setSurfaceSize : %p %d %d", vo,w,h);
    m_viewVo = vo;
    m_viewHeight = h;
    m_viewWidth = w;

    std::lock_guard<std::mutex> autoLock(m_DiyMutex);
    for (auto item : m_mapDiyImp){
        item.second->setSurfaceSize(vo, w, h);
        m_viewVo = nullptr;
        m_viewWidth = 0;
        m_viewHeight = 0;
	}
}
void DiyEffectPlayer::renderVideo(void *vo, unsigned int fbo_id)
{
    std::lock_guard<std::mutex> autoLock(m_DiyMutex);
    m_keyIndex++;
    for (auto item : m_mapDiyImp) {
        item.second->renderVideo(vo, fbo_id);
    }
    if (m_mapDiyImp.size())
		m_mapDiyImp[m_keyTag]->getNextDecodeFrame(m_keyTag);
}
void DiyEffectPlayer::clearGLResource(void *vo)
{
	{
        std::lock_guard<std::mutex> autolock(m_DiyMutex);
        for (const auto item : m_mapDiyImp) {
            item.second->clearGLResource(vo);
		}
	}
}
void DiyEffectPlayer::setRenderCallback(std::function<void(void *vo_opaque)> cb, void *vo)
{
    AF_LOGI("setRenderCallback %p,%p,tag: %s", cb, vo, m_rootTag.c_str());
    std::lock_guard<std::mutex> lock(m_cbMutex);
    if (cb) {
        UpdateCallbackInfo info;
        info.cb = cb;
        info.param = vo;
        info.sourceTag = m_rootTag;
        m_cbs[vo] = info;
    } else {
        m_cbs.erase(vo);
    }
}
void DiyEffectPlayer::setMaskMode(IVideoRender::MaskMode mode, const std::string &data)
{

}

void DiyEffectPlayer::foreignGLContextDestroyed(void *vo, const std::string &tag)
{
    SimpleGLRender::foreignGLContextDestroyed(vo, tag);
}

void DiyEffectPlayer::videoThreadInternal()
{

}

void DiyEffectPlayer::completionCallback(void *userData)
{
    DiyEffectPlayer *pThis = (DiyEffectPlayer *) userData;

	if (pThis->m_videoStage == VideoPlayerStage::STAGE_FIRST_RENDER) {
        if (pThis->m_listener.Completion) {
            pThis->m_listener.Completion(pThis->m_listener.userData);
            pThis->m_videoStage = VideoPlayerStage::STAGE_COMPLETED;
        }
	}
}

void DiyEffectPlayer::firstFrameCallback(void *userData)
{
    DiyEffectPlayer *pThis = (DiyEffectPlayer *) userData;

	if (pThis->m_videoStage == VideoPlayerStage::STAGE_FIRST_DECODEED) {
        if (pThis->m_listener.FirstFrameShow) {
            pThis->m_listener.FirstFrameShow(pThis->m_listener.userData);
            pThis->m_videoStage = VideoPlayerStage::STAGE_FIRST_RENDER;
        }
    }
}
void DiyEffectPlayer::videoSizeChangedCallback(int64_t width, int64_t height, void *userData)
{
    DiyEffectPlayer *pThis = (DiyEffectPlayer *) userData;

    if (pThis->m_listener.VideoSizeChanged) {
    }
}
void DiyEffectPlayer::preparedRender()
{
    AF_LOGI("preparedRender");
    std::lock_guard<std::mutex> autolock(m_cbMutex);
    for (const auto &item : m_cbs) {
        item.second.cb(item.second.param);

		if (m_videoStage == VideoPlayerStage::STAGE_PLAYING) {
            m_videoStage = VideoPlayerStage::STAGE_FIRST_DECODEED;
        }
	}
}

void DiyEffectPlayer::videoDecodeCallback(int64_t frameIndex, const void *tag, void *userData)
{
    DiyEffectPlayer *pThis = (DiyEffectPlayer *) userData;

    std::string sourceTag = (const char*)tag;
    AF_LOGI("videoDecodeCallback index: %d,tag:%s", frameIndex, sourceTag.c_str());
	
	static int keyFrameCount = 0;
    if (pThis->m_keyIndex == frameIndex) {
        keyFrameCount++;

		if (keyFrameCount == pThis->m_mapPath.size()) {
            keyFrameCount = 0;
            pThis->preparedRender();
		}
	}
}
