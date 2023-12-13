#pragma once

#include <condition_variable>
#include <mutex>
#include <map>
#include <queue>
#include <atomic>

#include "platform/platform_gl.h"
#include "IProgramContext.h"
#include <render/video/IVideoRender.h>
#include <render/video/glRender/vlc/GLRender.h>

struct AVFrame;
class SimpleGLRender {
public:
	struct RenderInfo {
		std::shared_ptr<GLRender> render = nullptr;
		int surfaceWidth = 0;
		int surfaceHeight = 0;
        std::string sourceTag;

		void reset() {
			render.reset();
		}
	};

	explicit SimpleGLRender();

	~SimpleGLRender();

	void setVideoTag(const std::string &tag);
    void enableHWDecoder(bool hw);
	void renderVideo(void *vo, AVFrame *frame, unsigned int fbo_id);
	void setVideoSurfaceSize(void *vo, int width, int height);
	void setMaskMode(IVideoRender::MaskMode mode, const std::string& data);
	void setVapInfo(const std::string& info);
	void clearGLResource(void *vo);

	static void foreignGLContextDestroyed(void *vo,const std::string& tag);
	//static std::map<void *, RenderInfo> mRenders;
    static std::map<RenderKey, RenderInfo> mRenders;
    static std::string referenceTag;
    static int referenceIndex;
    static std::condition_variable m_cv;
    static std::mutex m_cv_mutex;

protected:
	std::mutex mMaskInfoMutex;
	IVideoRender::MaskMode mMode{ IVideoRender::MaskMode::Mask_None };
	std::string mMaskVapData{};
	std::string mMaskVapInfo{};
    std::string m_sourceTag{};
    std::atomic_bool m_enableHW{true};
};

