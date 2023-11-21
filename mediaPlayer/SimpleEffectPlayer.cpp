#define LOG_TAG "SimpleEffectPlayer"
#include "SimpleEffectPlayer.h"
#include <codec/SimpleDecoder.h>
#include <data_source/vapparser/BinaryFileStream.hpp>
#include <render/video/glRender/SimpleGLRender.h>
#include <utils/timer.h>
using namespace std::chrono_literals;
extern "C" {
#include <libavformat/avformat.h>
}

SimpleEffectPlayer::SimpleEffectPlayer()
{
    AF_LOGI("Ctor");
    m_render = new SimpleGLRender;
    m_render->setMaskMode(IVideoRender::MaskMode::Mask_Right, "");
    m_decoder = new SimpleDecoder();
}
SimpleEffectPlayer::~SimpleEffectPlayer()
{
    stop();
    delete m_render;
    delete m_decoder;
}

void SimpleEffectPlayer::parseVapInfo(const std::string &path)
{
    ISOBMFF::BinaryFileStream stream(path);
    uint64_t length = 0;
    uint32_t offset = 0;
    std::string name;
    while (stream.HasBytesAvailable()) {
        length = stream.ReadBigEndianUInt32();
        name = stream.ReadFourCC();
        offset = 8;
        if (length == 1) {
            length = stream.ReadBigEndianUInt64();
            offset = 16;
        }

        if (length == 0) break;

        if (name == "vapc") {
            auto vapData = std::move(stream.Read(static_cast<uint32_t>(length) - offset));
            std::string vap;
            vap.assign((char *) vapData.data(), vapData.size());
            m_render->setVapInfo(vap);
            break;
        } else {
            try {
                stream.Seek(length - offset, ISOBMFF::BinaryStream::SeekDirection::Current);
            } catch (const std::exception &) {
                break;
            }
        }
    }
}

void SimpleEffectPlayer::start(const std::string &path)
{
    if (m_started) return;

    if (path.empty()) return;

    parseVapInfo(path);

    m_videoStaged = STAGE_INIIALIZED;
    bool ready = false;
    do {
        m_ctx = avformat_alloc_context();
        auto err = avformat_open_input(&m_ctx, path.c_str(), nullptr, nullptr);
        if (err < 0) break;

        err = avformat_find_stream_info(m_ctx, nullptr);
        if (err < 0) break;

        m_videoIndex = av_find_best_stream(m_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
        if (m_videoIndex < 0) break;

        if (!m_decoder->setupDecoder(m_ctx->streams[m_videoIndex]->codec->codec_id, m_ctx->streams[m_videoIndex]->codecpar->extradata,
                                     m_ctx->streams[m_videoIndex]->codecpar->extradata_size)) {
            m_decoder->closeDecoder();
            break;
        }

        ready = true;
        m_videoStaged = STAGE_PREPARED;
    } while (0);

    if (!ready) {
        if (m_ctx) {
            avformat_free_context(m_ctx);
            m_ctx = nullptr;
        }
    }

    auto codec = m_ctx->streams[m_videoIndex]->codec;
    if (m_vw != codec->width || m_vh != codec->height) {
        if (m_listener.VideoSizeChanged) {
            AF_LOGI("VideoSizeChanged");
            m_listener.VideoSizeChanged(codec->width, codec->height, m_listener.userData);
        }
        m_vw = codec->width;
        m_vh = codec->height;
    }

    m_fps = av_q2d(m_ctx->streams[m_videoIndex]->r_frame_rate);
    m_requestStopped = false;
    m_videoThread = std::thread(SimpleEffectPlayer::videoThread, this);
    m_started = true;
    m_videoStaged = STAGE_PLAYING;
}
void SimpleEffectPlayer::stop()
{
    if (!m_started) return;

    m_videoStaged = STAGE_STOPED;
    m_waitVar.notify_all();
    m_requestStopped = true;
    if (m_videoThread.joinable()) m_videoThread.join();

    avformat_free_context(m_ctx);
    m_decoder->closeDecoder();
    m_vw = -1;
    m_vh = -1;

    m_started = false;
    m_videoStaged = STAGE_RELEASED;
}

void SimpleEffectPlayer::setSmoothLoop(bool enable)
{
    m_smoothLoop = enable;
}

void SimpleEffectPlayer::setSurfaceSize(void *vo, int w, int h)
{
    m_render->setVideoSurfaceSize(vo, w, h);
}

void SimpleEffectPlayer::videoThread(void *param)
{
    SimpleEffectPlayer *p = (SimpleEffectPlayer *) param;
    p->videoThreadInternal();
}

AVPacket *SimpleEffectPlayer::readPacket()
{
    AVPacket *pkt = av_packet_alloc();
    int err;
    av_init_packet(pkt);

    do {
        err = av_read_frame(m_ctx, pkt);

        if (err < 0) {
            if (err == AVERROR_EOF) {
                if (m_smoothLoop) {
                    avformat_seek_file(m_ctx, -1, 0, 0, 0, 0);
                    continue;
                }

                av_packet_free(&pkt);
                return nullptr;// EOS
            }

            if (err == AVERROR_EXIT) {
                av_packet_free(&pkt);
                return nullptr;
            }

            av_packet_free(&pkt);
            return nullptr;
        }

        if (pkt->stream_index == m_videoIndex) break;

        av_packet_unref(pkt);
    } while (true);

    return pkt;
}

void SimpleEffectPlayer::videoThreadInternal()
{
    AF_LOGI("videoThreadInternal begin.");
    int ms = 1000. / m_fps;
    bool eof = false;
    int64_t beginPts = af_getsteady_ms();
    std::atomic_int frameCount = 0;
    while (!m_requestStopped) {
        while (!eof) {
            auto ret = m_decoder->getDecodedFrame();
            eof = ret == AVERROR_EOF;
            if (ret == 0 || eof) break;

            ret = m_decoder->sendPkt(readPacket());
            if (ret != 0 && ret != AVERROR(EAGAIN)) break;
        }

        int64_t interval = -1;
        if (!eof) {
            frameCount.fetch_add(1, std::memory_order_relaxed);
            int64_t endPts = af_getsteady_ms();
            int64_t interval = endPts - beginPts;
            beginPts = endPts;
            AF_LOGI("decoder const ms: %f,frameCount: %d,cost interval: %d", ms, frameCount.load(), interval);
        }

        {
            std::lock_guard<std::mutex> lock(m_cbMutex);
            for (auto iter = m_cbs.begin(); iter != m_cbs.end(); iter++) {
                auto &cb = iter->second;
                cb.cb(cb.param);
            }
        }

        if (eof) {
            if (m_listener.Completion) {
                AF_LOGI("Completion");
                m_videoStaged = STAGE_COMPLETED;
                m_listener.Completion(m_listener.userData);
            }
            break;
        } else {
            std::unique_lock<std::mutex> lock(m_waitMutex);
            m_waitVar.wait_for(lock, std::chrono::milliseconds(ms - interval));
        }

        std::unique_lock<std::mutex> lock(m_waitMutex);
        std::chrono::system_clock::time_point timeout = std::chrono::system_clock::now() + std::chrono::milliseconds((int) ms - interval);
        if (interval < ms) m_waitVar.wait_until(lock, timeout);
    }
}

void SimpleEffectPlayer::renderVideo(void *vo, unsigned int fbo_id)
{
    int64_t lastPts = af_getsteady_ms();

    m_decoder->renderFrame(
            [=](void *v, int frameIndex, AVFrame *frame, unsigned int id) {
                if (frame) {
                    if (m_videoStaged & STAGE_PLAYING) {
                        m_videoStaged = STAGE_FIRST_DECODEED;
                    }

                    m_render->renderVideo(v, frame, id);
                    m_waitVar.notify_one();

                    if (m_videoStaged & STAGE_FIRST_DECODEED) {
                        m_videoStaged = STAGE_FIRST_RENDER;
                        AF_LOGI("FirstFrameShow");
                        if (m_listener.FirstFrameShow) {
                            m_listener.FirstFrameShow(m_listener.userData);
                        }
                    }

                    int64_t endPts = af_getsteady_ms();
                    AF_LOGI("renderVideo  frameIndex : %d,cost time Interval: %d", frameIndex, endPts - lastPts);
                }
            },
            vo, fbo_id);
}

void SimpleEffectPlayer::clearGLResource(void *vo)
{
    m_render->clearGLResource(vo);
}

void SimpleEffectPlayer::foreignGLContextDestroyed(void *vo)
{
    SimpleGLRender::foreignGLContextDestroyed(vo);
}

void SimpleEffectPlayer::setRenderCallback(std::function<void(void *vo_opaque)> cb, void *vo)
{
    AF_LOGI("setRenderCallback");
    std::lock_guard<std::mutex> lock(m_cbMutex);
    if (cb) {
        UpdateCallbackInfo info;
        info.cb = cb;
        info.param = vo;
        m_cbs[vo] = info;
    } else {
        m_cbs.erase(vo);
    }
}

void SimpleEffectPlayer::setMaskMode(IVideoRender::MaskMode mode, const std::string &data)
{
    AF_LOGI("setMaskMode");
    m_render->setMaskMode(mode, data);
}

int SimpleEffectPlayer::SetListener(const playerListener &Listener)
{
    AF_LOGI("SetListener");
    m_listener = Listener;
    return 0;
}

void SimpleEffectPlayer::EnableHardwareDecoder(bool enable)
{
    AF_LOGI("EnableHardwareDecoder : %d", enable);
    m_decoder->enableHWDecoder(enable);
}