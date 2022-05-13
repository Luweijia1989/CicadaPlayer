//
// Created by lifujun on 2019/7/23.
//

#ifndef FRAMEWORK_IVideoRender_H
#define FRAMEWORK_IVideoRender_H


#include <base/media/IAFPacket.h>
#include <functional>
#include <utils/CicadaJSON.h>

typedef bool (*videoRenderingFrameCB)(void *userData, IAFFrame *frame, const CicadaJSONItem &params);

class GLRender;
class IVideoRender {

public:
    static const uint64_t FLAG_HDR = (1 << 0);
    static const uint64_t FLAG_DUMMY = (1 << 1);

    enum Rotate {
        Rotate_None = 0,
        Rotate_90 = 90,
        Rotate_180 = 180,
        Rotate_270 = 270
    };

    enum Flip {
        Flip_None,
        Flip_Horizontal,
        Flip_Vertical,
        Flip_Both,
    };

    enum Scale {
        Scale_AspectFit,
        Scale_AspectFill,
        Scale_Fill
    };

	enum MaskMode {
		Mask_None,
		Mask_Left,
		Mask_Right,
		Mask_Top,
		Mask_Down,
	};

	struct RenderInfo {
		std::unique_ptr<GLRender> render = nullptr;
		std::function<void(void* vo_opaque)> cb = nullptr;
		std::shared_ptr<IAFFrame> frame = nullptr;
		int surfaceWidth = 0;
		int surfaceHeight = 0;
		bool surfaceSizeChanged = 0;
		bool clearScreen = false;

		void reset();
	};

    static Rotate getRotate(int value)
    {
        switch (value) {
            case 90:
                return Rotate_90;
            case 180:
                return Rotate_180;
            case 270:
                return Rotate_270;
            default:
                return Rotate_None;
        }
    };

    class ScreenShotInfo {
    public:
        enum Format {
            UNKNOWN, RGB888,
        };
    public:
        Format format = UNKNOWN;

        int width = 0;
        int height = 0;

        int64_t bufLen = 0;
        char *buf = nullptr;

        ~ScreenShotInfo()
        {
            if (buf != nullptr) {
                free(buf);
                buf = nullptr;
            }
        }
    };

    class IVideoRenderFilter {
    public:
        virtual ~IVideoRenderFilter() = default;

        // a fbo or a frame
        virtual int push(void *) = 0;

        virtual int pull(void *) = 0;

    };

    class IVideoRenderListener {
    public:
        virtual void onFrameInfoUpdate(IAFFrame::AFFrameInfo &info, bool rendered) = 0;
        virtual ~IVideoRenderListener() = default;
    };

public:
    virtual ~IVideoRender() = default;

    /**
     * init render
     * @return
     */
    virtual int init() = 0;

    /**
     * clear screen to black.
     */
    virtual int clearScreen() = 0;

    /*
     * set background color
     */
    virtual void setBackgroundColor(uint32_t color) = 0;

    /**
     * set want draw frame.
     * @param frame
     */
    virtual int renderFrame(std::unique_ptr<IAFFrame> &frame) = 0;

    virtual void setListener(IVideoRenderListener *listener)
    {
        mListener = listener;
    }

    /**
     * set render rotate.
     * @param rotate
     */
    virtual int setRotate(Rotate rotate) = 0;

    /**
     * set render flip.
     * @param flip
     */
    virtual int setFlip(Flip flip) = 0;

    /**
     * set render scale.
     * @param scale
     */
    virtual int setScale(Scale scale) = 0;

    /**
     * set the playback speed, rend use it to improve render smooth
     * @param speed
     */
    virtual void setSpeed(float speed) = 0;

    /**
     * set window size when window size changed.
     * @param windWith
     * @param windHeight
     */
    virtual void setWindowSize(int windWith, int windHeight) {

    }

    virtual void surfaceChanged() {

    }

    virtual void setFilter(IVideoRenderFilter *filter)
    {
        mFilter = filter;
    }

    /**
     * set display view
     * @param view
     */
    virtual int setDisPlay(void *view)
    {
        return 0;
    }


    virtual void captureScreen(std::function<void(uint8_t *, int, int)> func)
    {
        func(nullptr,0,0);
    }


    virtual void *getSurface(bool cached)
    {
        return nullptr;
    }

    virtual float getRenderFPS() = 0;

    virtual void invalid(bool invalid)
    {
        mInvalid = invalid;
    }
    virtual uint64_t getFlags() = 0;

    virtual void setVideoRenderingCb(videoRenderingFrameCB cb, void *userData)
    {
        mRenderingCb = cb;
        mRenderingCbUserData = userData;
    }

	virtual void renderVideo(void *vo) {}
	virtual void setVideoSurfaceSize(int width, int height, void *vo) {}
	virtual void setRenderCallback(std::function<void(void* vo_opaque)> cb, void *vo) {}
	virtual void setMaskMode(MaskMode mode, const std::string& data) {}
	virtual void setVapInfo(const std::string& info) {}
	virtual void clearGLResource(void *vo) {}
	static void foreignGLContextDestroyed(void *vo);

protected:
    IVideoRenderFilter *mFilter{};
    bool mInvalid{false};
    IVideoRenderListener *mListener{nullptr};

    videoRenderingFrameCB mRenderingCb{nullptr};
    void *mRenderingCbUserData{nullptr};

	static std::mutex renderMutex;
	static std::map<void *, RenderInfo> mRenders;
};


#endif //FRAMEWORK_IVideoRender_H
