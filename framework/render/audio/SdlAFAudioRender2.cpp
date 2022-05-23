//
// Created by yuyuan on 2020/12/03.
//

#define LOG_TAG "SdlAFAudioRender2"

#include "SdlAFAudioRender2.h"
#include <assert.h>
#include <base/media/AVAFPacket.h>
#include <utils/ffmpeg_utils.h>
#include <utils/frame_work_log.h>

using namespace Cicada;

std::map<uint32_t, std::string> IAudioRender::audioOutputDevices()
{
	std::map<uint32_t, std::string> ret;

	SDL_InitSubSystem(SDL_INIT_AUDIO);

	auto count = SDL_GetNumAudioDevices(0);
	for (auto i = 0; i < count; i++) {
        ret.insert({i, SDL_GetAudioDeviceName(i, 0)});
	}
	
	ret.insert({UINT32_MAX, u8"默认"});

	SDL_QuitSubSystem(SDL_INIT_AUDIO);

	return ret;
}

SdlAFAudioRender2::SdlAFAudioRender2()
{}

SdlAFAudioRender2::~SdlAFAudioRender2()
{
    if (mDevID > 0) {
        SDL_CloseAudioDevice(mDevID);
        mDevID = 0;
    }
    SDL_QuitSubSystem(SDL_INIT_AUDIO);

    if (mPcmBuffer) {
        free(mPcmBuffer);
    }
}

bool SdlAFAudioRender2::device_require_format(const IAFFrame::audioInfo &info)
{
    int format = info.format;
    if (format == AF_SAMPLE_FMT_S16 || format == AF_SAMPLE_FMT_S16P || format == AF_SAMPLE_FMT_FLT || format == AF_SAMPLE_FMT_FLTP) {
        mInputInfo = info;
        return true;
    }
    return false;
}

int SdlAFAudioRender2::open_device(uint32_t device)
{
	const char *name = nullptr;
	if (device != UINT32_MAX)
		name = SDL_GetAudioDeviceName(device, 0);

	// init sdl audio device
    if (mDevID == 0) {
        SDL_AudioSpec inputSpec{0};
        int format = mInputInfo.format;
        if (format == AF_SAMPLE_FMT_S16 || format == AF_SAMPLE_FMT_S16P) {
            inputSpec.format = AUDIO_S16SYS;
        } else if (format == AF_SAMPLE_FMT_FLT || format == AF_SAMPLE_FMT_FLTP) {
            inputSpec.format = AUDIO_F32SYS;
        }
        inputSpec.freq = mInputInfo.sample_rate;
        inputSpec.channels = mInputInfo.channels;
        inputSpec.silence = 0;
        inputSpec.samples = mInputInfo.nb_samples;
        inputSpec.userdata = this;
        inputSpec.callback = nullptr;
        mDevID = SDL_OpenAudioDevice(name, false, &inputSpec, &mSpec, 0);
        if (mDevID == 0) {
            AF_LOGE("SdlAFAudioRender could not openAudio! Error: %s\n", SDL_GetError());
            return OPEN_AUDIO_DEVICE_FAILED;
        }
        mOutputInfo.channels = mSpec.channels;
        mOutputInfo.nb_samples = mSpec.samples;
        mOutputInfo.sample_rate = mSpec.freq;
    }

	deviceIndex = device;
	return 0;
}

int SdlAFAudioRender2::init_device()
{
    needFilter = true;
    // init sdl audio subsystem
    if (!mSdlAudioInited) {
        int initRet = SDL_Init(SDL_INIT_AUDIO);
        if (initRet < 0) {
            AF_LOGE("SdlAFAudioRender could not initialize! Error: %s\n", SDL_GetError());
            return initRet;
        }
        mSdlAudioInited = true;
    }

    return open_device(UINT32_MAX);
}

void SdlAFAudioRender2::device_change_device(uint32_t deviceId)
{
	if (deviceIndex == deviceId || (deviceId >= SDL_GetNumAudioDevices(0) && deviceId != UINT32_MAX))
		return;

	std::lock_guard<std::mutex> lock(mutex);

	if (mDevID > 0) {
        SDL_CloseAudioDevice(mDevID);
        mDevID = 0;
    }

	open_device(deviceId);

	start_device();
}

int SdlAFAudioRender2::pause_device()
{
    if (mDevID != 0) {
        SDL_PauseAudioDevice(mDevID, 1);
    }
    return 0;
}

int SdlAFAudioRender2::start_device()
{
    if (mDevID != 0) {
        SDL_PauseAudioDevice(mDevID, 0);
    }
    return 0;
}

void SdlAFAudioRender2::flush_device()
{
    if (mRunning) {
        pause_device();
    }
    if (mDevID != 0) {
        SDL_ClearQueuedAudio(mDevID);
    }
    mPlayedDuration = 0;
    if (mRunning) {
        start_device();
    }
}

void SdlAFAudioRender2::device_setVolume(float gain)
{}

int64_t SdlAFAudioRender2::device_get_position()
{
    return mPlayedDuration - device_get_que_duration();
}

int SdlAFAudioRender2::device_write(unique_ptr<IAFFrame> &frame)
{
	std::lock_guard<std::mutex> lock(mutex);

    if (device_get_que_duration() > 50 * 1000) {
        return -EAGAIN;
    }
    int pcmDataLength = getPCMDataLen(frame->getInfo().audio.channels, (enum AVSampleFormat) frame->getInfo().audio.format,
                                      frame->getInfo().audio.nb_samples);
    if (mPcmBufferSize < pcmDataLength) {
        mPcmBufferSize = pcmDataLength;
        mPcmBuffer = static_cast<uint8_t *>(realloc(mPcmBuffer, mPcmBufferSize));
    }
    bool rendered = false;
    if (mRenderingCb) {
        rendered = mRenderingCb(mRenderingCbUserData, frame.get());
    }
    if (!mMute && !rendered) {
        copyPCMData(getAVFrame(frame.get()), mPcmBuffer);
    } else {
        memset(mPcmBuffer, 0, pcmDataLength);
    }
    SDL_QueueAudio(mDevID, mPcmBuffer, pcmDataLength);

    assert(frame->getInfo().duration > 0);
    if (mListener) {
        mListener->onFrameInfoUpdate(frame->getInfo(), true);
    }
    mPlayedDuration += (uint64_t) frame->getInfo().audio.nb_samples * 1000000 / frame->getInfo().audio.sample_rate;
    //AF_LOGD("queued duration is %llu\n", getQueDuration());
    frame = nullptr;
    return 0;
}

void SdlAFAudioRender2::device_mute(bool bMute)
{
    mMute = bMute;
    // mute all queued audio buffer
    uint32_t queuedAudioSize = SDL_GetQueuedAudioSize(mDevID);
    uint8_t *muteAudioBuffer = (uint8_t *) malloc(queuedAudioSize);
    memset(muteAudioBuffer, 0, queuedAudioSize);
    SDL_ClearQueuedAudio(mDevID);
    SDL_QueueAudio(mDevID, muteAudioBuffer, queuedAudioSize);
    free(muteAudioBuffer);
}

uint64_t SdlAFAudioRender2::device_get_que_duration()
{
    int sampleSize = 4;
    if (mSpec.format == AUDIO_S16SYS) {
        sampleSize = 2;
    }
    double byteRate = mOutputInfo.channels * sampleSize * mOutputInfo.sample_rate;
    return (uint64_t)((SDL_GetQueuedAudioSize(mDevID) / byteRate) * 1000000);
}

int SdlAFAudioRender2::loopChecker()
{
    return 0;
}

void SdlAFAudioRender2::device_preClose()
{}

uint64_t SdlAFAudioRender2::device_get_ability()
{
    return 0;
}
