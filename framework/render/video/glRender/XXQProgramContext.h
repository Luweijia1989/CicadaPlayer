#ifndef SOURCE_XXQYUVPROGRAMCONTEXT_H
#define SOURCE_XXQYUVPROGRAMCONTEXT_H


#include "VAP.h"
#include "XXQGLRender.h"
#include <map>

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
        GLuint *videoTexture;
        IVideoRender::Rotate rotate;
    };

    MixRenderer();
    ~MixRenderer();

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
    bool mReady = false;
    GLuint mixColorConversionMatrix;
    std::string m_mixVertexShader;
    std::string m_mixFragmentShader;
    GLuint m_MixshaderProgram;
    GLuint mVertShader;
    GLuint mFragmentShader;
    // Uniform locations
    GLuint uTextureSrcUnitLocation;
    GLuint uTextureMaskUnitLocationY;
    GLuint uTextureMaskUnitLocationU;
    GLuint uTextureMaskUnitLocationV;
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

class XXQYUVProgramContext : public IProgramContext {

public:
    XXQYUVProgramContext();
    ~XXQYUVProgramContext() override;

private:
    int initProgram() override;

    void useProgram() override;

    void updateScale(IVideoRender::Scale scale) override;

    void updateFlip(IVideoRender::Flip flip) override;

    void updateRotate(IVideoRender::Rotate rotate) override;

    void updateBackgroundColor(uint32_t color) override;

    void updateWindowSize(int width, int height, bool windowChanged) override;

    int updateFrame(std::unique_ptr<IAFFrame> &frame) override;

    void updateMaskInfo(const std::string &vapInfo, IVideoRender::MaskMode mode, const std::string &data) override;

private:
    void createYUVTextures();

    void fillDataToYUVTextures(uint8_t **data, int *pLineSize, int format);

    void bindYUVTextures();

    void getShaderLocations();

    void updateUProjection();

    void updateDrawRegion();

    void updateTextureCoords();
    void updateAlphaTextureCoords();

    void updateFlipCoords();

    void updateColorRange();

    void updateColorSpace();

    void updateVapConfig();

private:
    IVideoRender::Rotate mRotate = IVideoRender::Rotate_None;
    IVideoRender::Scale mScale = IVideoRender::Scale_AspectFit;
    IVideoRender::Flip mFlip = IVideoRender::Flip_None;

    Rect mCropRect{0, 0, 0, 0};
    int mYLineSize = 0;

    GLuint mProgram = 0;
    GLuint mVertShader = 0;
    GLuint mFragmentShader = 0;

    GLint mProjectionLocation;
    GLint mColorSpaceLocation;
    GLint mColorRangeLocation;
    GLuint mPositionLocation;
    GLuint mTexCoordLocation;
    GLuint mAlphaTexCoordLocation;
    GLuint mFlipCoordsLocation;
    GLuint mYUVTextures[3];
    GLint mYTexLocation;
    GLint mUTexLocation;
    GLint mVTexLocation;
    GLint mAlphaBlendLocation;

    bool mProjectionChanged = false;
    GLfloat mUProjection[4][4];
    bool mRegionChanged = false;
    GLfloat mDrawRegion[8] = {0.0f};
    bool mTextureCoordsChanged = false;
    GLfloat mTextureCoords[8] = {0.0f};
    GLfloat mAlphaTextureCoords[8] = {0.0f};
    bool mFlipChanged = false;
    GLfloat mFlipCoords[2] = {0.0f};

    int mWindowWidth = 0;
    int mWindowHeight = 0;

    double mDar = 1;
    int mFrameWidth = 0;
    int mFrameHeight = 0;

    float mScaleWidth = 0.0f;
    float mScaleHeight = 0.0f;
    float off_x = 0.0f;
    float off_y = 0.0f;

    GLfloat mUColorSpace[9] = {0.0f};
    int mColorSpace = 0;
    GLfloat mUColorRange[3] = {0.0f};
    int mColorRange = 0;

    uint32_t mBackgroundColor = 0xff000000;
    bool mBackgroundColorChanged = true;

    IVideoRender::MaskMode mMode = IVideoRender::Mask_None;
    std::string mMaskVapData;
    std::string mVapInfo;
    bool mMaskInfoChanged = false;

    std::unique_ptr<VapAnimateConfig> mVapConfig;
    std::unique_ptr<MixRenderer> mMixRender;
};


#endif//SOURCE_XXQYUVPROGRAMCONTEXT_H
