//
// Created by yuyuan on 2021/03/18.
//

#ifndef DEMUXER_DASH_DASH_SEGMENT_TRACKER_H
#define DEMUXER_DASH_DASH_SEGMENT_TRACKER_H

#include "data_source/IDataSource.h"
#include "demuxer/IDemuxer.h"
#include "utils/afThread.h"
#include <base/OptionOwner.h>
#include <mutex>

namespace Cicada {

    class AlivcDataSource;
    class AdaptationSet;
    class Representation;
    class playList;

    namespace Dash {
        class DashSegment;
        typedef struct SidxBox_t SidxBox;
    }

    class DashSegmentTracker : public OptionOwner {
    public:
        explicit DashSegmentTracker(AdaptationSet *adapt, Representation *rep, const IDataSource::SourceConfig &sourceConfig);

        ~DashSegmentTracker();

        int init();

        Dash::DashSegment *getNextSegment();

        Dash::DashSegment *getStartSegment();

        Dash::DashSegment *getInitSegment();

        Dash::DashSegment *getIndexSegment();

        int getStreamType() const;

        const std::string getBaseUri();

        int getStreamInfo(int *width, int *height, uint64_t *bandwidth, std::string &language);

        std::string getDescriptionInfo();

        bool getSegmentNumberByTime(uint64_t &time, uint64_t &num);

        void print();

        int64_t getDuration();

        int reLoadPlayList();

        bool isLive() const;

        void interrupt(int inter);

        bool isInited() const;

        void setCurSegNum(uint64_t num);

        uint64_t getCurSegNum();

        uint64_t getCurSegPosition();

        void setCurSegPosition(uint64_t position);

        uint64_t getSegSize();

        std::string getPlayListUri();

        int GetRemainSegmentCount();

        bool isRealTimeStream() { return mRealtime; }

        uint64_t getLastSegNum();

        Representation *getNextRepresentation(AdaptationSet *adaptSet, Representation *rep) const;
        Representation *getCurrentRepresentation();
        bool bufferingAvailable() const;
        int64_t getMinAheadTime() const;
        int64_t getLiveDelay() const;
        int64_t getDurationToStartStream() const;
        int64_t getSegmentDuration() const;
        int64_t getStreamStartTime() const;
        vector<mediaSegmentListEntry> getSegmentList();

    private:
        int loadPlayList();

        int threadFunction();

        uint64_t getStartSegmentNumber(Representation *rep) const;
        uint64_t getLiveStartSegmentNumber(Representation *rep) const;
        int64_t getBufferingOffset(const playList *p) const;
        int64_t getLiveDelay(const playList *p) const;
        int64_t getMaxBuffering(const playList *p) const;
        int64_t getMinBuffering(const playList *p) const;
        bool isLowLatency(const playList *p) const;
        bool parseIndex(const Dash::SidxBox &sidx, const std::string& uri, int64_t startByte, int64_t endByte);

    private:
        AdaptationSet *mAdapt = nullptr;
        Representation *mRep = nullptr;
        playList *mPPlayList = nullptr;

        // all segment tracker share same reload interval
        static std::atomic<int64_t> mLastLoadTime;
        std::atomic<int64_t> mMinUpdatePeriod{24ll * 3600 * 1000000};

        std::atomic_bool mInited{false};

        std::atomic_bool mNeedUpdate{false};
        std::atomic_bool mStopLoading{false};

        std::mutex mSegMutex;
        std::condition_variable mSegCondition;
        afThread *mThread = nullptr;

        IDataSource *mPDataSource = nullptr;
        bool mInterrupted = false;

        IDataSource::SourceConfig mSourceConfig;
        std::recursive_mutex mMutex;

        std::atomic_int mPlayListStatus{0};

        bool mRealtime = false;

        uint64_t mCurrentSegNumber = (std::numeric_limits<uint64_t>::max)();
    };
}


#endif //DEMUXER_DASH_DASH_SEGMENT_TRACKER_H
