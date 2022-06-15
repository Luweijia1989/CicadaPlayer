//
// Created by lifujun on 2020/8/10.
//

#include "QueryListener.h"

using namespace Cicada;

QueryListener::QueryListener(MediaPlayer *player) {
    mPlayer = player;
}

void QueryListener::setMediaPlayer(MediaPlayer *player) {
    mPlayer = player;
}

// analytics query interface
int64_t QueryListener::OnAnalyticsGetCurrentPosition() {
    if (mPlayer) {
        return mPlayer->GetCurrentPosition();

    }

    return -1;
}

std::string QueryListener::OnAnalyticsGetNetworkSpeed(int64_t from, int64_t to)
{
    if (mPlayer) {
        CicadaJSONItem param{};
        param.addValue("from", (long) from);
        param.addValue("to", (long) to);
        return mPlayer->GetPropertyString(PROPERTY_KEY_NETWORK_SPEED, param);
    }
    return "";
}

std::string QueryListener::OnAnalyticsGetBufferInfo(int64_t from, int64_t to)
{
    if (mPlayer) {
        CicadaJSONItem param{};
        param.addValue("from", (long) from);
        param.addValue("to", (long) to);
        return mPlayer->GetPropertyString(PROPERTY_KEY_BUFFER_INFO, param);
    }
    return "";
}

std::string QueryListener::OnAnalyticsGetRequestInfos(int64_t from, int64_t to)
{
    if (mPlayer) {
        CicadaJSONItem param{};
        param.addValue("from", (long) from);
        param.addValue("to", (long) to);
        return mPlayer->GetPropertyString(PROPERTY_KEY_NETWORK_REQUEST_LIST, param);
    }
    return "";
}

int64_t QueryListener::OnAnalyticsGetBufferedPosition() {
    if (mPlayer) {
        return mPlayer->GetBufferedPosition();

    }

    return -1;
}

int64_t QueryListener::OnAnalyticsGetDuration() {
    if (mPlayer) {
        return mPlayer->GetDuration();

    }

    return -1;
}

std::string QueryListener::OnAnalyticsGetPropertyString(PropertyKey key) {
    if (mPlayer) {
        return mPlayer->GetPropertyString(key);
    }

    return "";
}
