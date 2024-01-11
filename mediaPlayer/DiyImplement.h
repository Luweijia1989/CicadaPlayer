#ifndef __DIYIMPLEMENT_H__
#define __DIYIMPLEMENT_H__

#include <atomic>
#include <condition_variable>
#include <map>
#include <mutex>
#include <native_cicada_player_def.h>
#include <render/video/IVideoRender.h>
#include <string>
#include <thread>
#include <utils/CicadaType.h>

struct AVFormatContext;
struct AVPacket;
class SimpleDecoder;
class SimpleGLRender;

class DiyImplement {
public:
    DiyImplement();
    ~DiyImplement();

	void setSourceTag(const std::string &tag);
	void start(const std::string& path);
    void stop();

	int SetListener(const playerListener &Listener);
	void setSmoothLoop(bool enable);
    void EnableHardwareDecoder(bool enable);

    void setSurfaceSize(void *vo, int w, int h);
    void renderVideo(void *vo, unsigned int fbo_id);
    void clearGLResource(void *vo);

	void getNextDecodeFrame(const std::string& keyTag);

private:
    void videoThreadInternal();
    AVPacket *readPacket();

private:
    playerListener m_listener{};
    double m_fps = 25.;
    bool m_smoothLoop = false;

	bool m_started = false;
    bool m_requestStopped = false;

    std::string m_filePath = "";
    std::string m_sourceTag;
    std::atomic_bool m_rendered{true};

    std::thread m_videoThread;
    int m_videoIndex = -1;
    AVFormatContext *m_ctx = nullptr;
    SimpleDecoder *m_decoder = nullptr;
    SimpleGLRender *m_render = nullptr;

    int m_vw = 0, m_vh = 0;
    VideoPlayerStage m_videoStaged{VideoPlayerStage::STAGE_IDEL};

	static std::mutex m_s_decodeMutex;
    static std::condition_variable m_s_cv_decode;
};

#endif
