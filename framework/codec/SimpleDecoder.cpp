#define LOG_TAG "SimpleDecoder"
#include "SimpleDecoder.h"
#include <codec/vlc/VideoAcceleration.h>
#include "utils/frame_work_log.h"

#define FRAME_CACHE_COUNT 4

int SimpleDecoder::lavc_GetFrame(struct AVCodecContext *ctx, AVFrame *frame, int flags)
{
	auto *decoder = (SimpleDecoder *)ctx->opaque;

	for (unsigned i = 0; i < AV_NUM_DATA_POINTERS; i++) {
		frame->data[i] = NULL;
		frame->linesize[i] = 0;
		frame->buf[i] = NULL;
	}
	frame->opaque = NULL;

	if (decoder->mVA == NULL) {
		return avcodec_default_get_buffer2(ctx, frame, flags);
	}

	return decoder->mVA->getFrame(frame);
}

enum AVPixelFormat SimpleDecoder::ffmpeg_GetFormat(AVCodecContext *p_context, const enum AVPixelFormat *pi_fmt)
{
	auto *decoder = (SimpleDecoder *)p_context->opaque;

	/* Enumerate available formats */
	enum AVPixelFormat swfmt = avcodec_default_get_format(p_context, pi_fmt);
	bool can_hwaccel = false;

	for (size_t i = 0; pi_fmt[i] != AV_PIX_FMT_NONE; i++) {
		const AVPixFmtDescriptor *dsc = av_pix_fmt_desc_get(pi_fmt[i]);
		if (dsc == NULL) continue;
		bool hwaccel = (dsc->flags & AV_PIX_FMT_FLAG_HWACCEL) != 0;

		if (hwaccel) can_hwaccel = true;
	}

	if (decoder->pix_fmt == AV_PIX_FMT_NONE) goto no_reuse;

	if (decoder->width != p_context->coded_width || decoder->height != p_context->coded_height) {
		goto no_reuse;
	}
	if (p_context->profile != decoder->profile || p_context->level > decoder->level) {
		goto no_reuse;
	}

	for (size_t i = 0; pi_fmt[i] != AV_PIX_FMT_NONE; i++)
		if (pi_fmt[i] == decoder->pix_fmt) {
			if (((SimpleDecoder *)p_context->opaque)->getVideoFormat(p_context, decoder->pix_fmt, swfmt) == 0) {
				return decoder->pix_fmt;
			}
		}

no_reuse:
	((SimpleDecoder *)p_context->opaque)->close_va_decoder();

	decoder->profile = p_context->profile;
	decoder->level = p_context->level;
	decoder->width = p_context->coded_width;
	decoder->height = p_context->coded_height;

	if (!can_hwaccel) {
		((SimpleDecoder *)p_context->opaque)->getVideoFormat(p_context, swfmt, swfmt);
		decoder->pix_fmt = swfmt;
		return swfmt;
	}

#if (LIBAVCODEC_VERSION_MICRO >= 100) && (LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57, 83, 101))
	if (p_context->active_thread_type) {
		AF_LOGD("thread type %d: disabling hardware acceleration", p_context->active_thread_type);
		return swfmt;
	}
#endif

	static const AVPixelFormat hwfmts[] = {
#ifdef _WIN32
		AV_PIX_FMT_D3D11VA_VLD,
		AV_PIX_FMT_DXVA2_VLD,
#endif
		AV_PIX_FMT_VAAPI_VLD,
#if (LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(52, 4, 0))
		AV_PIX_FMT_VDPAU,
#endif
		AV_PIX_FMT_NONE,
	};

	if (decoder->m_useHW) {
		for (size_t i = 0; hwfmts[i] != AV_PIX_FMT_NONE; i++) {
			AVPixelFormat hwfmt = AV_PIX_FMT_NONE;
			for (size_t j = 0; hwfmt == AV_PIX_FMT_NONE && pi_fmt[j] != AV_PIX_FMT_NONE; j++)
				if (hwfmts[i] == pi_fmt[j]) hwfmt = hwfmts[i];

			if (hwfmt == AV_PIX_FMT_NONE) continue;

			decoder->m_frameInfo.format.i_chroma = VideoAcceleration::vlc_va_GetChroma(hwfmt, swfmt);
			if (decoder->m_frameInfo.format.i_chroma == 0) continue;      /* Unknown brand of hardware acceleration */
			if (p_context->width == 0 || p_context->height == 0) { /* should never happen */
				continue;
			}

			auto va = VideoAcceleration::createVA(p_context, hwfmt);
			if (!va) continue;

			if (va->open() != VLC_SUCCESS) {
				delete va;
				continue;
			}

			decoder->mVA = va;
			decoder->pix_fmt = hwfmt;
			p_context->draw_horiz_band = NULL;
			((SimpleDecoder *)p_context->opaque)->getVideoFormat(p_context, hwfmt, swfmt);
			return hwfmt;
		}
	}

	((SimpleDecoder *)p_context->opaque)->getVideoFormat(p_context, swfmt, swfmt);
	decoder->pix_fmt = swfmt;
	return swfmt;
}

SimpleDecoder::SimpleDecoder()
{}

SimpleDecoder::~SimpleDecoder()
{}

void SimpleDecoder::setVideoTag(const std::string &tag)
{
    m_sourceTag = tag;
    AF_LOGI("setVideoTag : %s", m_sourceTag.c_str());
}

void SimpleDecoder::enableHWDecoder(bool enable)
{
	m_useHW = enable;
}

bool SimpleDecoder::setupDecoder(AVCodecID id, uint8_t *extra_data, int extra_data_size)
{
	m_eof = false;
	m_codec = avcodec_find_decoder(id);
	if (!m_codec) return false;

	m_codecCont = avcodec_alloc_context3(m_codec);
	if (!m_codecCont) return false;

	m_codecCont->get_format = ffmpeg_GetFormat;
	m_codecCont->get_buffer2 = lavc_GetFrame;
	m_codecCont->opaque = this;

	m_codecCont->thread_count = 1;
	m_codecCont->thread_safe_callbacks = true;
	m_codecCont->thread_type = 0;

	av_opt_set_int(m_codecCont, "refcounted_frames", 1, 0);

	m_codecCont->extradata = (uint8_t *)av_mallocz(extra_data_size + AV_INPUT_BUFFER_PADDING_SIZE);
	memcpy(m_codecCont->extradata, extra_data, extra_data_size);
	m_codecCont->extradata_size = extra_data_size;

	if (avcodec_open2(m_codecCont, m_codec, nullptr) < 0) return false;

	m_decodedFrame = av_frame_alloc();
	m_currentFrameIndex = 0;
	return true;
}

void SimpleDecoder::closeDecoder()
{
	if (m_codecCont != nullptr) {
		avcodec_close(m_codecCont);

		avcodec_free_context(&m_codecCont);
		m_codecCont = nullptr;
	}
	m_codec = nullptr;

	av_frame_free(&m_decodedFrame);
	{
		std::lock_guard<std::mutex> locker(m_frameMutex);
		for (auto frame : m_outputFrames)
		{
			av_frame_free(&std::get<0>(frame));
		}
		m_outputFrames.clear();
	}

	close_va_decoder();

	m_nCount = 0;
}

void SimpleDecoder::close_va_decoder()
{
	if (mVA) {
		mVA->close();
		delete mVA;
		mVA = nullptr;
	}
}

int SimpleDecoder::getVideoFormat(AVCodecContext *ctx, enum AVPixelFormat pix_fmt, enum AVPixelFormat sw_pix_fmt)
{
	int width = ctx->coded_width;
	int height = ctx->coded_height;
	int aligns[AV_NUM_DATA_POINTERS] = { 0 };

	video_format_Init(&m_frameInfo.format, 0);

	m_frameInfo.format.decoder_p = this;

	bool software_decoding = pix_fmt == sw_pix_fmt;
	if (software_decoding) { /* software decoding */
		avcodec_align_dimensions2(ctx, &width, &height, aligns);
		m_frameInfo.format.i_chroma = FindVlcChroma(pix_fmt);
	}
	else /* hardware decoding */
		m_frameInfo.format.i_chroma = VideoAcceleration::vlc_va_GetChroma(pix_fmt, sw_pix_fmt);

	if (width == 0 || height == 0 || width > 8192 || height > 8192 || width < ctx->width || height < ctx->height) {
		return -1; /* invalid display size */
	}

	const vlc_chroma_description_t *p_dsc = vlc_fourcc_GetChromaDescription(m_frameInfo.format.i_chroma);
	if (!p_dsc) return VLC_EGENERIC;

	int tw = ctx->coded_width;
	if (software_decoding) {
		int linesize[4] = { 0 };
		int unaligned = 0;
		do {
			// NOTE: do not align linesizes individually, this breaks e.g. assumptions
			// that linesize[0] == 2*linesize[1] in the MPEG-encoder for 4:2:2
			av_image_fill_linesizes(linesize, sw_pix_fmt, width);
			width += width & ~(width - 1);

			unaligned = 0;
			for (int i = 0; i < 4; i++) unaligned |= linesize[i] % aligns[i];
		} while (unaligned);
		tw = linesize[0] / p_dsc->pixel_size;
	}

	const int i_width_aligned = tw;
	const int i_height_aligned = ctx->coded_height;
	const int i_height_extra = 0; /* This one is a hack for some ASM functions */

	m_frameInfo.format.i_width = i_width_aligned;
	m_frameInfo.format.i_height = i_height_aligned;
	m_frameInfo.format.i_visible_width = ctx->width;
	m_frameInfo.format.i_visible_height = ctx->height;

	m_frameInfo.format.i_sar_num = ctx->sample_aspect_ratio.num;
	m_frameInfo.format.i_sar_den = ctx->sample_aspect_ratio.den;

	if (m_frameInfo.format.i_sar_num == 0 || m_frameInfo.format.i_sar_den == 0) m_frameInfo.format.i_sar_num = m_frameInfo.format.i_sar_den = 1;

	for (unsigned i = 0; i < p_dsc->plane_count; i++) {
		auto *p = &m_frameInfo.format.plane[i];

		p->i_lines = (i_height_aligned + i_height_extra) * p_dsc->p[i].h.num / p_dsc->p[i].h.den;
		p->i_visible_lines = (m_frameInfo.format.i_visible_height + (p_dsc->p[i].h.den - 1)) / p_dsc->p[i].h.den * p_dsc->p[i].h.num;
		p->i_pitch = i_width_aligned * p_dsc->p[i].w.num / p_dsc->p[i].w.den * p_dsc->pixel_size;
		p->i_visible_pitch =
			(m_frameInfo.format.i_visible_width + (p_dsc->p[i].w.den - 1)) / p_dsc->p[i].w.den * p_dsc->p[i].w.num * p_dsc->pixel_size;
		p->i_pixel_pitch = p_dsc->pixel_size;

		assert((p->i_pitch % 16) == 0);
	}
	m_frameInfo.format.i_planes = p_dsc->plane_count;

	/* FIXME we should only set the known values and let the core decide
	 * later of fallbacks, but we can't do that with a boolean */
	switch (ctx->color_range) {
	case AVCOL_RANGE_JPEG:
		m_frameInfo.format.b_color_range_full = true;
		break;
	case AVCOL_RANGE_UNSPECIFIED:
		m_frameInfo.format.b_color_range_full = !vlc_fourcc_IsYUV(m_frameInfo.format.i_chroma);
		break;
	case AVCOL_RANGE_MPEG:
	default:
		m_frameInfo.format.b_color_range_full = false;
		break;
	}

	switch (ctx->colorspace) {
	case AVCOL_SPC_BT709:
		m_frameInfo.format.space = COLOR_SPACE_BT709;
		break;
	case AVCOL_SPC_SMPTE170M:
	case AVCOL_SPC_BT470BG:
		m_frameInfo.format.space = COLOR_SPACE_BT601;
		break;
	case AVCOL_SPC_BT2020_NCL:
	case AVCOL_SPC_BT2020_CL:
		m_frameInfo.format.space = COLOR_SPACE_BT2020;
		break;
	default:
		break;
	}

	switch (ctx->color_trc) {
	case AVCOL_TRC_LINEAR:
		m_frameInfo.format.transfer = TRANSFER_FUNC_LINEAR;
		break;
	case AVCOL_TRC_GAMMA22:
		m_frameInfo.format.transfer = TRANSFER_FUNC_SRGB;
		break;
	case AVCOL_TRC_BT709:
		m_frameInfo.format.transfer = TRANSFER_FUNC_BT709;
		break;
	case AVCOL_TRC_SMPTE170M:
	case AVCOL_TRC_BT2020_10:
	case AVCOL_TRC_BT2020_12:
		m_frameInfo.format.transfer = TRANSFER_FUNC_BT2020;
		break;
	case AVCOL_TRC_ARIB_STD_B67:
		m_frameInfo.format.transfer = TRANSFER_FUNC_ARIB_B67;
		break;
	case AVCOL_TRC_SMPTE2084:
		m_frameInfo.format.transfer = TRANSFER_FUNC_SMPTE_ST2084;
		break;
	case AVCOL_TRC_SMPTE240M:
		m_frameInfo.format.transfer = TRANSFER_FUNC_SMPTE_240;
		break;
	case AVCOL_TRC_GAMMA28:
		m_frameInfo.format.transfer = TRANSFER_FUNC_BT470_BG;
		break;
	default:
		break;
	}

	switch (ctx->color_primaries) {
	case AVCOL_PRI_BT709:
		m_frameInfo.format.primaries = COLOR_PRIMARIES_BT709;
		break;
	case AVCOL_PRI_BT470BG:
		m_frameInfo.format.primaries = COLOR_PRIMARIES_BT601_625;
		break;
	case AVCOL_PRI_SMPTE170M:
	case AVCOL_PRI_SMPTE240M:
		m_frameInfo.format.primaries = COLOR_PRIMARIES_BT601_525;
		break;
	case AVCOL_PRI_BT2020:
		m_frameInfo.format.primaries = COLOR_PRIMARIES_BT2020;
		break;
	default:
		break;
	}

	switch (ctx->chroma_sample_location) {
	case AVCHROMA_LOC_LEFT:
		m_frameInfo.format.chroma_location = CHROMA_LOCATION_LEFT;
		break;
	case AVCHROMA_LOC_CENTER:
		m_frameInfo.format.chroma_location = CHROMA_LOCATION_CENTER;
		break;
	case AVCHROMA_LOC_TOPLEFT:
		m_frameInfo.format.chroma_location = CHROMA_LOCATION_TOP_LEFT;
		break;
	default:
		break;
	}

	if (mVA) m_frameInfo.format.extra_info = mVA->getExtraInfoForRender();

	return 0;
}

int SimpleDecoder::sendPkt(AVPacket *pkt)
{
    m_nCount++;
    auto ret = avcodec_send_packet(m_codecCont, pkt);
    AF_LOGI("sendPkt nCount:%d,ret: %d  [%s]", m_nCount, ret, m_sourceTag.c_str());
	av_packet_free(&pkt);
	return ret;
}

void SimpleDecoder::renderFrame(std::function<void(void*, int frameIndex,AVFrame *, unsigned int)> cb, void *vo, unsigned int fbo_id)
{
	std::lock_guard<std::mutex> locker(m_frameMutex);
	if (m_outputFrames.empty())
		return;

	auto frame = m_outputFrames.front();
    cb(vo, std::get<1>(frame),std::get<0>(frame), fbo_id);
	av_frame_free(&std::get<0>(frame));
	m_outputFrames.pop_front();
    AF_LOGI("renderFrame outFrames: %d [%s]", m_outputFrames.size(),m_sourceTag.c_str());
}

int SimpleDecoder::getDecodedFrame()
{
	int ret = avcodec_receive_frame(m_codecCont, m_decodedFrame);
	if (ret < 0) {
		if (ret == AVERROR_EOF) {
			m_eof = true;
			avcodec_flush_buffers(m_codecCont);

			if (!m_outputFrames.empty())
				return 0;
		}
		else if (ret == AVERROR(EAGAIN) && m_eof)
			return AVERROR_EOF;

		return ret;
	}

	if (m_decodedFrame->decode_error_flags || m_decodedFrame->flags)
		return ret;

	{
        std::lock_guard<std::mutex> locker(m_frameMutex);
        auto frame = av_frame_clone(m_decodedFrame);
        m_frameInfo.index = ++m_currentFrameIndex;
        frame->opaque = &m_frameInfo;

        if (m_outputFrames.size() == FRAME_CACHE_COUNT) {
            auto frame = m_outputFrames.front();
            av_frame_free(&std::get<0>(frame));
            m_outputFrames.pop_front();
            AF_LOGI("pop old video data [%s]",m_sourceTag.c_str());
		}
        m_outputFrames.push_back({frame, m_frameInfo.index});

        AF_LOGI("getDecodedFrame outFrames: %d [%s]", m_outputFrames.size(),m_sourceTag.c_str());
	}

	//if (m_outputFrames.size() > FRAME_CACHE_COUNT)
	//	return AVERROR(EAGAIN);
	//else
	//	return ret;
    return ret;
}