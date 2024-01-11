#ifndef __DIYEFFECTPLAYER_H__
#define __DIYEFFECTPLAYER_H__

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
class DiyImplement;

class CICADA_CPLUS_EXTERN DiyEffectPlayer {
public:
    struct UpdateCallbackInfo {
        std::function<void(void *vo_opaque)> cb;
        void *param;
        std::string sourceTag;
    };
    DiyEffectPlayer();
    ~DiyEffectPlayer();

	int SetListener(const playerListener &Listener);
    void EnableHardwareDecoder(bool enable);

    void start(const std::list<std::string> &path);
    void start(const std::string& path){;}
    void stop();

    void setSmoothLoop(bool enable);

    void setRootTag(const std::string &tag);
    void setSurfaceSize(void *vo, int w, int h);
    void renderVideo(void *vo, unsigned int fbo_id);
    void clearGLResource(void *vo);
    void setRenderCallback(std::function<void(void *vo_opaque)> cb, void *vo);
    void setMaskMode(IVideoRender::MaskMode mode, const std::string &data);

    static void foreignGLContextDestroyed(void *vo, const std::string &tag);

private:
    void videoThreadInternal();
    void preparedRender();

    static void completionCallback(void *userData);
    static void firstFrameCallback(void *userData);
    static void videoSizeChangedCallback(int64_t width, int64_t height, void *userData);
    static void videoDecodeCallback(int64_t frameIndex, const void *tag, void *userData);

private:
	playerListener m_listener{};
    std::list<std::string> m_filePath;
    bool m_smoothLoop = false;
    bool m_hwEnable = false;

	bool m_started = false;
    bool m_requestStopped = false;

	int m_viewWidth;
    int m_viewHeight;
    void *m_viewVo;

	std::map<void *, UpdateCallbackInfo> m_cbs;
    std::mutex m_cbMutex;

	std::string m_rootTag;
	std::map<std::string /*tag*/, std::string /*path*/> m_mapPath;
    std::map<std::string, DiyImplement *> m_mapDiyImp;
    std::mutex m_DiyMutex;

	std::string m_keyTag{""};
    int m_keyIndex{0};
	VideoPlayerStage m_videoStage{VideoPlayerStage::STAGE_IDEL};
};

#endif
