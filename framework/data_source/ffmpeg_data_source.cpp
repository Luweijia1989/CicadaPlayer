//
// Created by moqi on 2018/1/24.
//
#include "ffmpeg_data_source.h"

#define LOG_TAG "ffmpegDataSource"

#include <algorithm>
#include <utils/frame_work_log.h>
#include <utils/errors/framework_error.h>
#include <utils/ffmpeg_utils.h>
#include "vapparser/BinaryFileStream.hpp"

extern "C" {
#include <libavformat/avio.h>
#include <libavutil/error.h>
}
using std::string;
namespace Cicada {

    ffmpegDataSource ffmpegDataSource::se(0);

    ffmpegDataSource::ffmpegDataSource(const string &url) : IDataSource(url)
    {
        mInterruptCB.callback = check_interrupt;
        mInterruptCB.opaque = this;
        ffmpeg_init();
    }

    ffmpegDataSource::~ffmpegDataSource()
    {
        Close();
    }

    int ffmpegDataSource::Open(int flags)
    {
        AVDictionary *format_opts = nullptr;
        av_dict_set_int(&format_opts, "rw_timeout", mConfig.low_speed_time_ms * 1000, 0);
        int ret = avio_open2(&mPuc, mUri.c_str(), AVIO_FLAG_READ | AVIO_FLAG_NONBLOCK, &mInterruptCB, &format_opts);

        if (ret == AVERROR_PROTOCOL_NOT_FOUND) {
            ret = FRAMEWORK_ERR_PROTOCOL_NOT_SUPPORT;
        }
        if (format_opts) {
            av_dict_free(&format_opts);
        }

        if (ret < 0) {
            AF_LOGE("open error\n");
            return ret;
        }
        if (rangeStart != INT64_MIN) {
            avio_seek(mPuc, (int64_t) rangeStart, SEEK_SET);
        }
        if (strcmp(avio_find_protocol_name(mUri.c_str()), "file") == 0) {
            isNetWork = false;
        }

		parseVapInfo();

        return ret;
    }

    void ffmpegDataSource::Close()
    {
        if (mPuc) {
            avio_closep(&mPuc);
        }
    }

    int64_t ffmpegDataSource::Seek(int64_t offset, int whence)
    {
        if (mPuc == nullptr) {
            return -EINVAL;
        }
        int64_t pos = offset;
        switch (whence) {
            case SEEK_SIZE:
                return avio_size(mPuc);
            case SEEK_END: {
                int64_t size = avio_size(mPuc);
                if (size <= 0) {
                    return -EINVAL;
                }
                pos = size + offset;
                whence = SEEK_SET;
            }
            default:
                return avio_seek(mPuc, pos, whence);
        }
    }

    int ffmpegDataSource::Read(void *buf, size_t nbyte)
    {
        if (mPuc == nullptr) {
            return -EINVAL;
        }
        if (rangeEnd != INT64_MIN) {
            nbyte = std::min<size_t>(nbyte, (size_t) (rangeEnd - Seek(0, SEEK_CUR)));

            if (nbyte == 0) {
                return 0;
            }
        }

        int ret = avio_read(mPuc, (unsigned char *) buf, nbyte);

        if (ret == AVERROR_EOF) {
            ret = 0;
        }

        if (isNetWork && ret > 0 && mConfig.listener) {
            mConfig.listener->onNetWorkInPut(ret, IDataSource::Listener::bitStreamTypeMedia);
        }

        return ret;
    }

    void ffmpegDataSource::Interrupt(bool interrupt)
    {
        mInterrupted = interrupt;
    }

    string ffmpegDataSource::Get_error_info(int error)
    {
        int ret = av_strerror(error, mErrorMsg, sizeof(mErrorMsg));

        if (ret < 0) {
            return "";
        }

        return (const char *) mErrorMsg;
    }

    string ffmpegDataSource::GetOption(const string &key)
    {
        //TODO
//        std::lock_guard<std::mutex> lock(mMutex);
//        if (key == "responseInfo") {
//                CicadaJSONItem Json;
//                Json.addValue("response", response);
//                return Json.printJSON();
//        }
//
//        if (key == "connectInfo") {
//        }
        return IDataSource::GetOption(key);
    }

    int ffmpegDataSource::check_interrupt(void *pHandle)
    {
        auto *source = (ffmpegDataSource *) pHandle;
        return source->mInterrupted;
    }

	void ffmpegDataSource::parseVapInfo()
	{
		IDataSource::setVapData(std::string());

		ISOBMFF::BinaryFileStream stream(mUri);
        uint64_t length = 0;
		uint32_t offset = 0;
        std::string name;
		while (stream.HasBytesAvailable()) {
			length = stream.ReadBigEndianUInt32();
            name   = stream.ReadFourCC();
            offset = 8;
            if (length == 1) {
                length = stream.ReadBigEndianUInt64();
				offset = 16;
            }
            
			if (name == "vapc") {
				auto vapData = std::move(stream.Read(static_cast< uint32_t >( length ) - offset));
				std::string vap;
				vap.assign((char *)vapData.data(), vapData.size());
				IDataSource::setVapData(vap);
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
}

