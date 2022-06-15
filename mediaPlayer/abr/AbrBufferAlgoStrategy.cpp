//
//  AbrAlgoStrategy.cpp
//  apsara_player
//
//  Created by shiping.csp on 2018/11/1.
//
#define LOG_TAG "AbrBufferAlgoStrategy"
#include "AbrBufferAlgoStrategy.h"
#include "AbrBufferRefererData.h"
#include <cassert>
#include <utility>
#include <utils/CicadaJSON.h>
#include <utils/frame_work_log.h>
#include <utils/timer.h>
#define LOWER_SWITCH_VALUE_MS (15 * 1000)
#define UPPER_SWITCH_VALUE_MS (30 * 1000)
#define MAX_BUFFER_STATICS_SIZE 10

AbrBufferAlgoStrategy::AbrBufferAlgoStrategy(std::function<void(int)> func) : AbrAlgoStrategy(std::move(func))
{
    Reset();
}

AbrBufferAlgoStrategy::~AbrBufferAlgoStrategy() = default;

void AbrBufferAlgoStrategy::SetCurrentBitrate(int bitrate)
{
    AF_LOGI("BA already change to bitrate:%d", bitrate);
    AbrAlgoStrategy::SetCurrentBitrate(bitrate);
    mSwitching = false;
    mLastSwitchTimeMS = af_getsteady_ms();
}

void AbrBufferAlgoStrategy::Reset()
{
    mSwitching = false;
    mLastDownloadBytes = 0;
    mLastSwitchTimeMS = INT64_MIN;
    mLastBufferDuration = INT64_MIN;
    mBufferStatics.clear();
    mDownloadSpeed.clear();
    mIsUpHistory.clear();
}

void AbrBufferAlgoStrategy::ComputeBufferTrend(int64_t curTime)
{
    if (mSwitching || (mBitRates.empty()) || mRefererData->IsDownloadCompleted()) {
        return;
    }

    if (mLastSwitchTimeMS == INT64_MIN) {
        return;
    }

    if (mLastBufferDuration == INT64_MIN) {
        mLastBufferDuration = mRefererData->GetCurrentPacketBufferLength() / 1000;
        return;
    }

    bool buffering = mRefererData->GetReBuffering();

    int64_t maxBufferDuration = mRefererData->GetMaxBufferDurationInConfig() / 1000;
    int64_t bufferDuration = mRefererData->GetCurrentPacketBufferLength() / 1000;
    bool bufferFull = (bufferDuration >= (maxBufferDuration - 1000));

    if (!bufferFull && mDurationMS == 0) {
        //        AF_LOGI("BA connect %d full:%d", connect, bufferFull);
        if (mRefererData->GetIsConnected()) {
            bufferFull = mRefererData->GetRemainSegmentCount() == 0;
        }
    }

    if (!bufferFull) {
        mDownloadSpeed.push_back(mRefererData->GetCurrentDownloadSpeed());

        if (mDownloadSpeed.size() > 30) {
            mDownloadSpeed.pop_front();
        }
    }

    if (buffering) {
        //        assert(!bufferFull);
        mBufferStatics.push_back(-1);
    } else {
        if ((bufferDuration > mLastBufferDuration) || bufferFull) {
            mBufferStatics.push_back(1);
        } else {
            mBufferStatics.push_back(-1);
        }
    }

    mLastBufferDuration = bufferDuration;

    if (mBufferStatics.size() > MAX_BUFFER_STATICS_SIZE) {
        mBufferStatics.pop_front();
    }

    int bufferUp = 0;

    for (auto iter : mBufferStatics) {
        bufferUp += iter;
    }

    int64_t maxSpeed = 0;
    int64_t averageSpeed = 0;
    if (!mDownloadSpeed.empty()) {
        std::list<int64_t> downloadSpeed = mDownloadSpeed;
        downloadSpeed.sort(std::greater<int64_t>());
        int count = 0;

        for (auto iter : downloadSpeed) {
            averageSpeed += iter;

            // ignore lowest data
            if (++count > (mDownloadSpeed.size() * 2 / 3)) {
                break;
            }
        }

        if (count) {
            averageSpeed /= count;
        }

        maxSpeed = downloadSpeed.front();
    }
    AF_LOGD("BA bufferUp:%d,bufferDuration:%lld,isFull:%d Max:%lld average:%lld", bufferUp, bufferDuration, bufferFull, maxSpeed,
            averageSpeed);

    if (((bufferDuration < LOWER_SWITCH_VALUE_MS) && (bufferUp <= -(MAX_BUFFER_STATICS_SIZE - 2))) || bufferDuration < 100) {
        SwitchBitrate(false, averageSpeed, maxSpeed);
    } else if ((bufferDuration >= UPPER_SWITCH_VALUE_MS || bufferFull) && (bufferUp >= MAX_BUFFER_STATICS_SIZE - 2)) {
        // if last BA down
        if (!mIsUpHistory.empty() && !mIsUpHistory.back()) {
            // wait more time
            int64_t time = af_getsteady_ms();

            if ((time - mLastSwitchTimeMS) < mUpSpan) {
                return;
            }

            // It needs to be more rigorous to BA up
            if (bufferUp < MAX_BUFFER_STATICS_SIZE) {
                return;
            }
        }

        SwitchBitrate(true, averageSpeed, maxSpeed);
    }
}

void AbrBufferAlgoStrategy::SwitchBitrate(bool up, int64_t speed, int64_t maxSpeed)
{
    int currentIndex = -1;
    int count = (int) mBitRates.size();

    for (int i = 0; i < count; i++) {
        if (mBitRates[i] == mCurrentBitrate) {
            currentIndex = i;
            break;
        }
    }
    assert(currentIndex >= 0 && currentIndex < count);

    int bitrate = -1;

    if (up) {
        if (currentIndex >= (count - 1)) {
            updateSwitchStatus(Status::Highest_Already, false);
            return;
        }
        bitrate = mBitRates[currentIndex + 1];

        if (!mIsUpHistory.empty() && speed > 0) {
            // last BA down
            if (!mIsUpHistory.back() && maxSpeed < bitrate) {
                AF_LOGI("last BA down, maxSpeed:%lld, nextBitrate:%d", maxSpeed, bitrate);
                return;
            }
        }

        for (int i = currentIndex + 2; i < count; ++i) {
            if (speed >= mBitRates[i]) {
                bitrate = mBitRates[i];
            }
        }
    } else {
        if (currentIndex == 0) {
            // lowest already
            updateSwitchStatus(Status::Lowest_Already, false);
            return;
        }

        if (maxSpeed == 0) {
            bitrate = mBitRates[currentIndex - 1];
        } else {
            for (int i = currentIndex - 1; i >= 0; --i) {
                if (speed >= mBitRates[i]) {
                    bitrate = mBitRates[i];
                    break;
                }
            }

            if (-1 == bitrate) {
                bitrate = mBitRates[0];
            }
        }
    }

    if (bitrate != -1 && mCurrentBitrate != bitrate) {
        AF_LOGI("BA switch to bitrate:%d", bitrate);
        mPreBitrate = mCurrentBitrate;
        mCurrentBitrate = bitrate;
        auto iter = mStreamIndexBitrateMap.find(mCurrentBitrate);

        if (iter != mStreamIndexBitrateMap.end()) {
            mSwitching = true;
            int index = iter->second;
            mBufferStatics.clear();
            mIsUpHistory.push_back(up);

            if (!up) {
                mUpSpan = 60 * 1000;
            } else {
                mUpSpan = (MAX_BUFFER_STATICS_SIZE - 1) * 1000;
            }

            if (mIsUpHistory.size() > 3) {
                mIsUpHistory.pop_front();
            }

            updateSwitchStatus(Status::Switch, true);
            mFunc(index);
        }
    }
}

void AbrBufferAlgoStrategy::ProcessAbrAlgo()
{
    if (mRefererData == nullptr || mCurrentBitrate == -1 || mBitRates.size() < 2) {
        return;
    }
    ComputeBufferTrend(af_getsteady_ms());
}

void AbrBufferAlgoStrategy::updateSwitchStatus(Status newStatus, bool forceCb)
{
    AF_LOGD("BA switch status:%d", newStatus);

    Status oldStatus = mSwitchStatus;
    mSwitchStatus = newStatus;
    if (oldStatus != newStatus || forceCb) {
        if (mStatusCallback) {
            mStatusCallback(newStatus);
        }
    }
}

void AbrBufferAlgoStrategy::GetOption(const std::string &key, std::string &value)
{

    if (key == "switchInfo") {
        CicadaJSONItem result{};
        result.addValue("fb", (int) mPreBitrate);
        result.addValue("tb", (int) mCurrentBitrate);

        CicadaJSONArray speedInfos{};
        for (auto &speedItem : mDownloadSpeed) {
            speedInfos.addInt64(speedItem);
        }
        result.addArray("spd", speedInfos);

        CicadaJSONArray bufferInfos{};
        for (auto &bufferItem : mBufferStatics) {
            bufferInfos.addInt64(bufferItem);
        }
        result.addArray("buf", bufferInfos);

        value = result.printJSON();
    }
}
