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
    AF_LOGI("===========[%s] statr path: %s=============",m_sourceTag.c_str(),path.c_str());
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
            AF_LOGI("VideoSizeChanged : %s", m_sourceTag.c_str());
            m_listener.VideoSizeChanged(codec->width, codec->height, m_listener.userData);
        }
        m_vw = codec->width;
        m_vh = codec->height;
    }

    m_fps = av_q2d(m_ctx->streams[m_videoIndex]->r_frame_rate);
    AF_LOGI("fps: %f, tag: %s", m_fps, m_sourceTag.c_str());
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

void SimpleEffectPlayer::setSourceTag(const std::string &tag)
{
    AF_LOGI("setSourceTag : %s", tag.c_str());
    m_sourceTag = tag;
    if (m_decoder) {
        m_decoder->setVideoTag(tag);
    }
    if (m_render) {
        m_render->setVideoTag(tag);
    }
}

void SimpleEffectPlayer::setSurfaceSize(void *vo, int w, int h)
{
    AF_LOGI("setSurfaceSize : %s", m_sourceTag.c_str());
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
    double ms = 1000.0 / m_fps;
    bool eof = false;
    int64_t beginPts = af_getsteady_ms();
    std::atomic_int frameCount = 0;

    AF_LOGI("videoThreadInternal begin. %ld,%s", beginPts, m_sourceTag.c_str());

    while (!m_requestStopped) {
        auto lastPts = af_getsteady_ms();
        while (!eof) {
            auto ret = m_decoder->getDecodedFrame();
            eof = ret == AVERROR_EOF;
            if (ret == 0 || eof) break;

            ret = m_decoder->sendPkt(readPacket());
            if (ret != 0 && ret != AVERROR(EAGAIN)) break;
        }

        int64_t interval = 0;
        if (!eof) {
            frameCount.fetch_add(1, std::memory_order_relaxed);
            int64_t endPts = af_getsteady_ms();
            interval  = endPts - lastPts;
            //beginPts = endPts;
            AF_LOGI("decoder frameCount: %d,cost interval: %d [%s]", frameCount.load(), interval, m_sourceTag.c_str());
        }

        {
            std::lock_guard<std::mutex> lock(m_cbMutex);
            for (auto iter = m_cbs.begin(); iter != m_cbs.end(); iter++) {
                auto &cb = iter->second;
                m_rendered.store(false);
                cb.cb(cb.param);
            }
        }

        if (eof) {
            if (m_listener.Completion) {
                AF_LOGI("Completion tag: %s", m_sourceTag.c_str());
                m_videoStaged = STAGE_COMPLETED;
                m_listener.Completion(m_listener.userData);
            }
            break;
        } else {
            //wait render finish or timeout
            //AF_LOGI("render finished before");
            std::unique_lock<std::mutex> lock(m_waitMutex);
            m_waitVar.wait_for(lock, frameCount.load() == 1 ? 60ms : 20ms, [=] { return m_rendered.load(); });
            //AF_LOGI("render finished after");
        }

        int64_t needPts = beginPts + frameCount.load() * ms;
        int64_t endPts = af_getsteady_ms();
        //AF_LOGI("decoder needPts: %ld,endPts: %ld ,wait internal: %ld ,FrameCount: %d [%s]", needPts, endPts, (endPts - lastPts),frameCount.load(), m_sourceTag.c_str());
        if (needPts > endPts) {
            std::unique_lock<std::mutex> lock(m_waitMutex);
            //AF_LOGI("wait 1 [%s]",m_sourceTag.c_str());
            m_waitVar.wait_for(lock, std::chrono::milliseconds(needPts - endPts));
        } else {
            std::unique_lock<std::mutex> lock(m_waitMutex);
            //AF_LOGI("wait 2 [%s]", m_sourceTag.c_str());
            m_waitVar.wait_for(lock, std::chrono::milliseconds((int)ms - (endPts - lastPts)));
        }

        //calculate fps
        if (frameCount.load() % int(m_fps) == 0) {
            int64_t endPts = af_getsteady_ms();
            double dy_fps = frameCount.load() * 1000.0 / (endPts - beginPts);
            AF_LOGI("curPts: %ld,caculate dynamic fps : %f  framecount: %d [%s]", endPts, dy_fps, frameCount.load(), m_sourceTag.c_str());
        }
    }
}

void SimpleEffectPlayer::renderVideo(void *vo, unsigned int fbo_id)
{
    //AF_LOGI("renderVideo entry [%s]", m_sourceTag.c_str());
    int64_t lastPts = af_getsteady_ms();

    int nRes = m_decoder->renderFrame(
            [=](void *v, int frameIndex, AVFrame *frame, unsigned int id) {
                if (frame) {
                    if (m_videoStaged & STAGE_PLAYING) {
                        m_videoStaged = STAGE_FIRST_DECODEED;
                    }

                    m_render->renderVideo(v, frame, frameIndex, id);
                    //AF_LOGI("render finished notify before");
                    m_rendered.store(true);
                    m_waitVar.notify_one();
                    //AF_LOGI("render finished notify after");

                    if (m_videoStaged & STAGE_FIRST_DECODEED) {
                        m_videoStaged = STAGE_FIRST_RENDER;
                        AF_LOGI("FirstFrameShow tag: %s", m_sourceTag.c_str());
                        if (m_listener.FirstFrameShow) {
                            m_listener.FirstFrameShow(m_listener.userData);
                        }
                    }

                    int64_t endPts = af_getsteady_ms();
                    AF_LOGI("renderVideo  frameIndex : %d,cost time Interval: %d [%s]", frameIndex, endPts - lastPts, m_sourceTag.c_str());
                } else {
                    AF_LOGE("renderVideo  no enough buffer to Render [%s]", m_sourceTag.c_str());
                }
            },
            vo, fbo_id);
}

void SimpleEffectPlayer::clearGLResource(void *vo)
{
    m_render->clearGLResource(vo);
}

void SimpleEffectPlayer::foreignGLContextDestroyed(void *vo, const std::string &tag)
{
    SimpleGLRender::foreignGLContextDestroyed(vo, tag);
}

void SimpleEffectPlayer::setRenderCallback(std::function<void(void *vo_opaque)> cb, void *vo)
{
    AF_LOGI("setRenderCallback %p,%p,tag: %s", cb, vo, m_sourceTag.c_str());
    std::lock_guard<std::mutex> lock(m_cbMutex);
    if (cb) {
        UpdateCallbackInfo info;
        info.cb = cb;
        info.param = vo;
        info.sourceTag = m_sourceTag;
        m_cbs[vo] = info;
    } else {
        m_cbs.erase(vo);
    }
}

void SimpleEffectPlayer::setMaskMode(IVideoRender::MaskMode mode, const std::string &data)
{
    AF_LOGI("setMaskMode tag: %s", m_sourceTag.c_str());
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
    AF_LOGI("EnableHardwareDecoder : %d tag: %s", enable, m_sourceTag.c_str());
    m_decoder->enableHWDecoder(enable);
    m_render->enableHWDecoder(enable);
}