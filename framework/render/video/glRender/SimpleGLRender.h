#pragma once

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
		std::unique_ptr<GLRender> render = nullptr;
		int surfaceWidth = 0;
		int surfaceHeight = 0;

		void reset() {
			render.reset();
		}
	};

	explicit SimpleGLRender();

	~SimpleGLRender();

	void renderVideo(void *vo, AVFrame *frame, unsigned int fbo_id);
	void setVideoSurfaceSize(void *vo, int width, int height);
	void setMaskMode(IVideoRender::MaskMode mode, const std::string& data);
	void setVapInfo(const std::string& info);
	void clearGLResource(void *vo);

	static void foreignGLContextDestroyed(void *vo);
	static std::map<void *, RenderInfo> mRenders;

protected:
	std::mutex mMaskInfoMutex;
	IVideoRender::MaskMode mMode{ IVideoRender::MaskMode::Mask_None };
	std::string mMaskVapData{};
	std::string mMaskVapInfo{};
};

