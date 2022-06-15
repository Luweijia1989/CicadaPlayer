//
//  MediaPlayerUtil.cpp
//  ApsaraPlayer
//
//  Created by huang_jiafa on 2010/01/30.
//  Copyright (c) 2019 Aliyun. All rights reserved.
//

#ifndef ApsaraPlayerUtil_h
#define ApsaraPlayerUtil_h

#include <atomic>
#include <string>
//#include "render_engine/math/geometry.h"

class CicadaJSONItem;

class CicadaJSONArray;

namespace Cicada {
    class MediaPlayerUtil {
    public:
        enum readEvent {
            readEvent_Again,
            readEvent_Got,
            readEvent_timeOut,
            readEvent_Loop,
            readEvent_Network,
        };

        MediaPlayerUtil() = default;

        ~MediaPlayerUtil() = default;

        void notifyPlayerLoop(int64_t time);

        void notifyRead(enum readEvent event, uint64_t size);

        void videoRendered(bool rendered);

        void reset();

        void getVideoDroppedInfo(uint64_t &total, uint64_t &dropped)
        {
            total = mTotalRenderCount;
            dropped = mDroppedRenderCount;
        }

        float getVideoRenderFps()
        {
            return mVideoRenderFps;
        }

        float getCurrentDownloadSpeed() const
        {
            return mCurrentDownloadSpeed;
        }

		int getCurrentFrameIndex() 
		{
			return mCurrentFrameIndex++;
		}

    private:
        std::atomic<uint64_t> mTotalRenderCount{0};
        std::atomic<uint64_t> mDroppedRenderCount{0};
        uint64_t mLastRenderCount = 0;
        int64_t mFirstRenderTime = 0;
        int64_t mLastRenderTime = 0;

        int64_t mLastLoopTime = 0;
        int64_t mLoopIndex = 0;
        int64_t mReadIndex = 0;
        int64_t mReadLoopIndex = 0;
        int64_t mReadAgainIndex = 0;
        int64_t mReadGotIndex = 0;
        int64_t mReadTimeOutIndex = 0;
        int64_t mLastReadTime = 0;
        std::atomic<uint64_t> mReadGotSize{0};
        float mCurrentDownloadSpeed{0};
        float mVideoRenderFps = 0;

		int64_t mCurrentFrameIndex = 0;
    };
}// namespace Cicada

#endif /* ApsaraPlayerUtil_h */
