#ifndef CICADA_MEDIA_BUFFER_CONTROL_H
#define CICADA_MEDIA_BUFFER_CONTROL_H

#include <mutex>
#include <deque>
#include <utils/AFMediaType.h>
#include <base/media/IAFPacket.h>

namespace Cicada {
    class MediaPacketQueue {
    public:
        MediaPacketQueue();

        ~MediaPacketQueue();

        typedef std::unique_ptr<IAFPacket> mediaPacket;

        void SetOnePacketDuration(int64_t duration);

        int64_t GetOnePacketDuration();

    public:
        void ClearQueue();

        void AddPacket(mediaPacket frame);

        std::unique_ptr<IAFPacket> getPacket();

        int GetSize();

        int64_t GetDuration();

        int64_t GetPts();

        int64_t GetKeyTimePositionBefore(int64_t pts);

        int64_t GetKeyTimePositionBeforeUTCTime(int64_t time);

        int64_t GetFirstKeyPTS(int64_t pts);

        int64_t GetLastKeyTimePos();

        int64_t ClearPacketBeforeTimePos(int64_t pts);

        int64_t ClearPacketBeforePTS(int64_t pts);

        void Rewind();

        int64_t GetLastTimePos();

        int64_t GetFirstTimePos();

        int64_t GetLastPTS();

        int64_t FindSeamlessPointTimePosition(int &count);

        void ClearPacketAfterTimePosition(int64_t pts);

        void SetMaxBackwardDuration(uint64_t duration)
        {
            mMAXBackwardDuration = duration;
        }

        int mMediaType = 0;

    private:
        void PopFrontPacket();

    private:
        std::list<mediaPacket> mQueue;
        std::list<mediaPacket>::iterator mCurrent;
        std::recursive_mutex mMutex;
        int64_t mPacketDuration = 0;
        int64_t mDuration = 0;
        int64_t mTotalDuration = 0;
        uint64_t mMAXBackwardDuration{0};

        uint8_t *mDropedExtra_data{nullptr};
        int mDropedExtra_data_size{0};
    };

} // namespace Cicada
#endif // CICADA_MEDIA_BUFFER_CONTROL_H
