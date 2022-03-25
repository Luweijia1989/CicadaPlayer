#pragma once

#include "../IVideoRender.h"
#include "VAP.h"
#include "gl_common.h"
#include "vlc_es.h"

class MixRenderer {
public:
    struct MixParam {
        float offsetX;
        float offsetY;

        float scaleW;
        float scaleH;

        int videoW;
        int videoH;

        int ow;
        int oh;

        GLfloat **matrix;
        GLuint videoTexture;
        IVideoRender::Rotate rotate;
    };

    MixRenderer(opengl_vtable_t *vt);
    ~MixRenderer();

    int compileShader(GLuint *shader, const char *src, GLenum type)
    {
        GLuint shaderId = vt->CreateShader(type);
        vt->ShaderSource(shaderId, 1, &src, nullptr);
        vt->CompileShader(shaderId);
        GLint status;
        vt->GetShaderiv(shaderId, GL_COMPILE_STATUS, &status);

        if (status != GL_TRUE) {
            int length = 0;
            GLchar glchar[256] = {0};
            vt->GetShaderInfoLog(shaderId, 256, &length, glchar);
            vt->DeleteShader(shaderId);
            return -1;
        }

        *shader = shaderId;
        return 0;
    }

    void renderMix(int index, const MixParam &param);
    void setVapxInfo(const CicadaJSONItem &json);
    void setMixResource(const std::string &rcStr);
    bool ready()
    {
        return mReady;
    }

private:
    void compileMixShader();
    void genSrcCoordsArray(GLfloat *array, int fw, int fh, int sw, int sh, MixSrc::FitType fitType);
    void prepareMixResource();
    void clearMixSrc();
    void renderMixPrivate(const VAPFrame &frame, const MixSrc &src, const MixParam &param);

private:
    opengl_vtable_t *vt;
    bool mReady = false;
    std::string m_mixVertexShader;
    std::string m_mixFragmentShader;
    GLuint m_MixshaderProgram;
    GLuint mVertShader;
    GLuint mFragmentShader;
    // Uniform locations
    GLuint uTextureSrcUnitLocation;
    GLuint uTextureMaskUnitLocationImage;
    GLuint uIsFillLocation;
    GLuint uColorLocation;
    GLuint uMixMatrix;

    // Attribute locations
    GLuint aPositionLocation;
    GLuint aTextureSrcCoordinatesLocation;
    GLuint aTextureMaskCoordinatesLocation;

    GLfloat m_mixVertexArray[8];
    GLfloat m_mixSrcArray[8];
    GLfloat m_maskArray[8];

    std::map<std::string, MixSrc> m_srcMap;
    std::map<std::string, std::string> m_mixResource;
    std::map<int, FrameSet> m_allFrames;
};

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

    void setFrameIndex(int index)
    {
        frameIndex = index;
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

    int frameIndex = 0;

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
    std::unique_ptr<MixRenderer> mMixRender;
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