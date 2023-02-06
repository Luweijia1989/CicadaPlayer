/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "audio-resampler.h"
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>

#define MAX_AUDIO_CHANNELS 8

struct audio_resampler {
    struct SwrContext *context;
    bool opened;

    uint32_t input_freq;
    uint64_t input_layout;
    enum AVSampleFormat input_format;

    uint8_t *output_buffer[MAX_AV_PLANES];
    uint64_t output_layout;
    enum AVSampleFormat output_format;
    int output_size;
    uint32_t output_ch;
    uint32_t output_freq;
    uint32_t output_planes;
};

static inline bool is_audio_planar(enum AVSampleFormat format)
{
    switch (format) {
        case AV_SAMPLE_FMT_U8:
        case AV_SAMPLE_FMT_S16:
        case AV_SAMPLE_FMT_S32:
        case AV_SAMPLE_FMT_FLT:
            return false;

        case AV_SAMPLE_FMT_U8P:
        case AV_SAMPLE_FMT_S16P:
        case AV_SAMPLE_FMT_S32P:
        case AV_SAMPLE_FMT_FLTP:
        case AV_SAMPLE_FMT_DBLP:
            return true;

        case AV_SAMPLE_FMT_NONE:
            return false;
    }

    return false;
}

audio_resampler_t *audio_resampler_create(const struct resample_info *dst, const struct resample_info *src)
{
    struct audio_resampler *rs = malloc(sizeof(struct audio_resampler));
	memset(rs, 0, sizeof(struct audio_resampler));
    int errcode;

    rs->opened = false;
    rs->input_freq = src->samples_per_sec;
    rs->input_layout = src->layout;
    rs->input_format = src->format;
    rs->output_size = 0;
    rs->output_ch = dst->channels;
    rs->output_freq = dst->samples_per_sec;
    rs->output_layout = dst->layout;
    rs->output_format = dst->format;
    rs->output_planes = is_audio_planar(dst->format) ? rs->output_ch : 1;

    rs->context = swr_alloc_set_opts(NULL, rs->output_layout, rs->output_format, dst->samples_per_sec, rs->input_layout, rs->input_format,
                                     src->samples_per_sec, 0, NULL);

    if (!rs->context) {
        audio_resampler_destroy(rs);
        return NULL;
    }

    if (rs->input_layout == AV_CH_LAYOUT_MONO && rs->output_ch > 1) {
        const double matrix[MAX_AUDIO_CHANNELS][MAX_AUDIO_CHANNELS] = {
                {1}, {1, 1}, {1, 1, 0}, {1, 1, 1, 1}, {1, 1, 1, 0, 1}, {1, 1, 1, 1, 1, 1}, {1, 1, 1, 0, 1, 1, 1}, {1, 1, 1, 0, 1, 1, 1, 1},
        };
        if (swr_set_matrix(rs->context, matrix[rs->output_ch - 1], 1) < 0) {
        }
    }

    errcode = swr_init(rs->context);
    if (errcode != 0) {
        audio_resampler_destroy(rs);
        return NULL;
    }

    return rs;
}

void audio_resampler_destroy(audio_resampler_t *rs)
{
    if (rs) {
        if (rs->context) swr_free(&rs->context);
        if (rs->output_buffer[0]) av_freep(&rs->output_buffer[0]);

        free(rs);
    }
}

bool audio_resampler_resample(audio_resampler_t *rs, uint8_t *output[], uint32_t *out_frames, uint64_t *ts_offset,
                              const uint8_t *const input[], uint32_t in_frames)
{
    if (!rs) return false;

    struct SwrContext *context = rs->context;
    int ret;

    int64_t delay = swr_get_delay(context, rs->input_freq);
    int estimated = (int) av_rescale_rnd(delay + (int64_t) in_frames, (int64_t) rs->output_freq, (int64_t) rs->input_freq, AV_ROUND_UP);

    *ts_offset = (uint64_t) swr_get_delay(context, 1000000000);

    /* resize the buffer if bigger */
    if (estimated > rs->output_size) {
        if (rs->output_buffer[0]) av_freep(&rs->output_buffer[0]);

        av_samples_alloc(rs->output_buffer, NULL, rs->output_ch, estimated, rs->output_format, 0);

        rs->output_size = estimated;
    }

    ret = swr_convert(context, rs->output_buffer, rs->output_size, (const uint8_t **) input, in_frames);

    if (ret < 0) {
        return false;
    }

    for (uint32_t i = 0; i < rs->output_planes; i++) output[i] = rs->output_buffer[i];

    *out_frames = (uint32_t) ret;
    return true;
}
