#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
#include <libavutil/opt.h>
#include <vlc_es.h>
#include <chroma.h>
}
#include <mutex>

class VideoAcceleration;
struct video_frame_info
{
	int index = 0;
	video_format_t format{};
};
struct video_format_t;
class SimpleDecoder {
public:
	SimpleDecoder();
	~SimpleDecoder();

	void enableHWDecoder(bool enable);
	bool setupDecoder(AVCodecID id, uint8_t *extra_data, int extra_data_size);
	void closeDecoder();

	void close_va_decoder();
	int getVideoFormat(AVCodecContext *ctx, enum AVPixelFormat pix_fmt, enum AVPixelFormat sw_pix_fmt);

	int sendPkt(AVPacket *pkt);
	int getDecodedFrame();

	void renderFrame(std::function<void(void*, int ,AVFrame *, unsigned int)> cb, void *vo, unsigned int fbo_id);

public:
	static int lavc_GetFrame(struct AVCodecContext *ctx, AVFrame *frame, int flags);
	static enum AVPixelFormat ffmpeg_GetFormat(AVCodecContext *p_context, const enum AVPixelFormat *pi_fmt);

private:
	AVCodecContext *m_codecCont = nullptr;
	AVCodec *m_codec = nullptr;
	AVFrame *m_decodedFrame = nullptr;
	AVFrame *m_outputFrame = nullptr;
	bool m_eof = false;
	std::mutex m_frameMutex;
	bool m_useHW = true;
	int m_currentFrameIndex = 0;
	/* VA API */
	video_frame_info m_frameInfo = { 0 };
	VideoAcceleration *mVA = nullptr;
	AVPixelFormat pix_fmt{};
	int profile = 0;
	int level = 0;
	int width = 0;
	int height = 0;
};
