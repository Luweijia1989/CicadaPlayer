#pragma once

#include "../IVideoRender.h"
#include "VAP.h"
#include "gl_common.h"
#include "vlc_es.h"

class GiftEffectRender {
public:
    GiftEffectRender(opengl_vtable_t *vt, video_format_t *fmt, const std::string &vapInfo, IVideoRender::MaskMode mode,
                     const std::string &data);
    ~GiftEffectRender();

    bool maskInfoChanged(const std::string &vapInfo, IVideoRender::MaskMode mode, const std::string &data);

    void setViewSize(int w, int h)
    {
        if (viewWidth != w || viewHeight != h) {
            mProjectionChanged = true;
            mRegionChanged = true;
            viewWidth = w;
            viewHeight = h;
        }
    }

    void setTransformInfo(IVideoRender::Rotate rotate, IVideoRender::Scale scale, IVideoRender::Flip flip)
    {
        if (mScale != scale) {
            mScale = scale;
            mRegionChanged = true;
        }

        if (mRotate != rotate) {
            mRotate = rotate;
            mRegionChanged = true;
        }

        if (mFlip != flip) {
            mFlip = flip;
            mFlipChanged = true;
        }
    }

    void bindFbo();
    void releaseFbo();

    void draw();

private:
    void updateMaskInfo();
    void initPrgm();
    void updateProjection();
    void updateDrawRegion();
    void updateTextureCoords();
    void updateAlphaTextureCoords();
    void updateFlipCoords();
    void updateTransform();

private:
    opengl_vtable_t *vt = nullptr;
    video_format_t *fmt = nullptr;

    IVideoRender::MaskMode mMode = IVideoRender::Mask_None;
    std::string mMaskVapData;
    std::string mVapInfo;

    int viewWidth = 0;
    int viewHeight = 0;

    IVideoRender::Rotate mRotate = IVideoRender::Rotate_None;
    IVideoRender::Scale mScale = IVideoRender::Scale_AspectFit;
    IVideoRender::Flip mFlip = IVideoRender::Flip_None;

    float off_x = 0.0f;
    float off_y = 0.0f;
    float mScaleWidth = 0.0f;
    float mScaleHeight = 0.0f;

    GLint last_fbo = -1;

    GLuint videoFrameFbo;
    GLuint videoFrameTexture;

    GLuint prgm;

    bool mProjectionChanged = true;
    GLfloat mUProjection[4][4];
    bool mRegionChanged = true;
    GLfloat mDrawRegion[8] = {0.0f};
    bool mTextureCoordsChanged = true;
    GLfloat mTextureCoords[8] = {0.0f};
    GLfloat mAlphaTextureCoords[8] = {0.0f};
    bool mFlipChanged = true;
    GLfloat mFlipCoords[2] = {0.0f};

    GLuint projection;
    GLuint uflip;
    GLuint position;
    GLuint RGBTexCoord;
    GLuint alphaTexCoord;
    GLuint SamplerImage;

    std::unique_ptr<VapAnimateConfig> mVapConfig;
};

class FBOBindHelper {
public:
    FBOBindHelper(GiftEffectRender *render) : mRender(render)
    {
        mRender->bindFbo();
    }

    ~FBOBindHelper()
    {
        mRender->releaseFbo();
        mRender->draw();
    }

private:
    GiftEffectRender *mRender = nullptr;
};