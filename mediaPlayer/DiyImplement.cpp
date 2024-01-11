#define LOG_TAG "DiyImplement"
#include "DiyImplement.h"
#include <codec/SimpleDecoder.h>
#include <data_source/vapparser/BinaryFileStream.hpp>
#include <render/video/glRender/SimpleGLRender.h>
#include <utils/timer.h>
using namespace std::chrono_literals;
extern "C" {
#include <libavformat/avformat.h>
}

std::mutex DiyImplement::m_s_decodeMutex;
std::condition_variable DiyImplement::m_s_cv_decode;

DiyImplement::DiyImplement()
{
    AF_LOGI("Ctor");
    m_render = new SimpleGLRender;
    m_render->setMaskMode(IVideoRender::MaskMode::Mask_Right, "");
    m_decoder = new SimpleDecoder();
}
DiyImplement ::~DiyImplement()
{
    stop();
    delete m_render;
    delete m_decoder;
}

void DiyImplement::setSourceTag(const std::string &tag)
{
    AF_LOGI("setSourceTag %s", tag.c_str());
    m_sourceTag = tag;
    if (m_render) m_render->setVideoTag(tag);
    if (m_decoder) m_decoder->setVideoTag(tag);
}
void DiyImplement::start(const std::string &path)
{
    if (m_started) return;

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
    m_videoThread = std::thread(&DiyImplement::videoThreadInternal,this);
    m_started = true;
    m_videoStaged = VideoPlayerStage::STAGE_PLAYING;
}

void DiyImplement::stop()
{
    m_requestStopped = true;
    if (m_videoThread.joinable()) m_videoThread.join();

    avformat_free_context(m_ctx);
    m_decoder->closeDecoder();
    //m_render->clearGLSurface();
    m_vw = -1;
    m_vh = -1;

    m_started = false;
}

void DiyImplement::videoThreadInternal()
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
            interval = endPts - lastPts;
            //beginPts = endPts;
            AF_LOGI("decoder frameCount: %d,cost interval: %d [%s]", frameCount.load(), interval, m_sourceTag.c_str());
        } else {
            if (m_videoStaged == VideoPlayerStage::STAGE_FIRST_RENDER) {
                if (m_listener.Completion) {
                    m_listener.Completion(m_listener.userData);
				}
			}
		}

		{
            if (m_listener.VideoDecode) {
                m_listener.VideoDecode(frameCount, (const void *) m_sourceTag.c_str(), m_listener.userData);
                if (m_videoStaged == VideoPlayerStage::STAGE_PLAYING) {
                    m_videoStaged = VideoPlayerStage::STAGE_FIRST_DECODEED;
				}
            }
        }

        int64_t needPts = beginPts + frameCount.load() * ms;
        int64_t endPts = af_getsteady_ms();

		{
            std::unique_lock<std::mutex> lock(m_s_decodeMutex);
            m_s_cv_decode.wait(lock);
		}

		if (endPts > lastPts)
		{
            std::unique_lock<std::mutex> lock(m_s_decodeMutex);
            m_s_cv_decode.wait_for(lock, std::chrono::milliseconds((int)ms - (endPts - lastPts)));
		}

        //calculate fps
        if (frameCount.load() % int(m_fps) == 0) {
            int64_t endPts = af_getsteady_ms();
            double dy_fps = frameCount.load() * 1000.0 / (endPts - beginPts);
            AF_LOGI("curPts: %ld,caculate dynamic fps : %f  framecount: %d [%s]", endPts, dy_fps, frameCount.load(), m_sourceTag.c_str());
        }
    }
}

AVPacket* DiyImplement::readPacket()
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

void DiyImplement::renderVideo(void *vo, unsigned int fbo_id)
{
    int64_t lastPts = af_getsteady_ms();

    m_decoder->renderFrame(
            [=](void *v, int frameIndex, AVFrame *frame, unsigned int id) {
                if (frame) {
                    m_render->renderVideo(v, frame, frameIndex, id);

                    if (m_videoStaged == VideoPlayerStage::STAGE_FIRST_DECODEED) {
                        m_videoStaged = VideoPlayerStage::STAGE_FIRST_RENDER;
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

void DiyImplement::getNextDecodeFrame(const std::string &keyTag)
{
    m_s_cv_decode.notify_all();
}
int DiyImplement::SetListener(const playerListener &Listener)
{
    m_listener = Listener;
    return 0;
}
void DiyImplement::setSmoothLoop(bool enable)
{
    AF_LOGI("setSmoothLoop : %d", enable);
    m_smoothLoop = enable;
}
void DiyImplement::EnableHardwareDecoder(bool enable)
{
    AF_LOGI("EnableHardwareDecoder : %d", enable);
    if (m_decoder) m_decoder->enableHWDecoder(enable);
    if (m_render) m_render->enableHWDecoder(enable);
}

void DiyImplement::setSurfaceSize(void *vo, int w, int h)
{
    AF_LOGI("setSurfaceSize : %p %d %d", vo,w,h);
    if (m_render) m_render->setVideoSurfaceSize(vo, w, h);
}

void DiyImplement::clearGLResource(void *vo)
{
    AF_LOGI("clearGLResource ");
    if (m_render) m_render->clearGLResource(vo);
}