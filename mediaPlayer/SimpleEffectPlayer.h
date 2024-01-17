#pragma once

#include <condition_variable>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <atomic>
#include <utils/CicadaType.h>
#include <native_cicada_player_def.h>
#include <render/video/IVideoRender.h>

struct AVFormatContext;
struct AVPacket;
class SimpleDecoder;
class SimpleGLRender;

class CICADA_CPLUS_EXTERN SimpleEffectPlayer {
public:
	struct UpdateCallbackInfo {
		std::function<void(void *vo_opaque)> cb;
		void *param;
        std::string sourceTag;
	};

	SimpleEffectPlayer();
	~SimpleEffectPlayer();

	int SetListener(const playerListener &Listener);
	void EnableHardwareDecoder(bool enable);

	void start(const std::string &path);
	void stop();

	void setSmoothLoop(bool enable);

	void setSourceTag(const std::string& tag);
	void setSurfaceSize(void *vo,int w, int h);
    void renderVideo(void *vo,unsigned int fbo_id);
	void clearGLResource(void *vo);
	void setRenderCallback(std::function<void(void *vo_opaque)> cb, void *vo);
	void setMaskMode(IVideoRender::MaskMode mode, const std::string& data);

	static void foreignGLContextDestroyed(void *vo,const std::string& tag);

public:
	static void videoThread(void *param);

private:
	void parseVapInfo(const std::string &path);
	void videoThreadInternal();
	AVPacket *readPacket();

private:
	playerListener m_listener{};
	std::string m_filePath = "";
	double m_fps = 25.;
	bool m_smoothLoop = false;

	bool m_started = false;
	bool m_requestStopped = false;
	std::mutex m_waitMutex;
	std::condition_variable m_waitVar;

	std::map<void *, UpdateCallbackInfo> m_cbs;
	std::mutex m_cbMutex;

	std::thread m_videoThread;
	int m_videoIndex = -1;
	AVFormatContext *m_ctx = nullptr;
	SimpleDecoder *m_decoder = nullptr;
	SimpleGLRender *m_render = nullptr;

	int m_vw = 0, m_vh = 0;

	VideoPlayerStage m_videoStaged = STAGE_IDEL;

	std::string m_sourceTag;
    std::atomic_bool m_rendered{true};
};