#ifndef SOURCE_XXQYUVPROGRAMCONTEXT_H
#define SOURCE_XXQYUVPROGRAMCONTEXT_H


#include "VAP.h"
#include "XXQGLRender.h"
#include "src/math3d/qmatrix4x4.h"
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

    MixRenderer(AFPixelFormat format);
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
    AFPixelFormat mPixelFormat;
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

class OpenGLVideo;
class XXQYUVProgramContext : public IProgramContext {

public:
    XXQYUVProgramContext();
    ~XXQYUVProgramContext() override;

private:
    void updateScale(IVideoRender::Scale scale) override;

    void updateFlip(IVideoRender::Flip flip) override;

    void updateRotate(IVideoRender::Rotate rotate) override;

    void updateBackgroundColor(uint32_t color) override;

    void updateWindowSize(int width, int height, bool windowChanged) override;

    int updateFrame(std::unique_ptr<IAFFrame> &frame) override;

    void updateMaskInfo(const std::string &vapInfo, IVideoRender::MaskMode mode, const std::string &data) override;

	void clearScreen(uint32_t color) override;

private:
    void updateVapConfig();

private:
    bool mDrawRectChanged = true;
    int mWindowWidth, mWindowHeight, mFrameWidth, mFrameHeight;
    int mBackgroundColor;
    bool mBackgroundColorChanged = true;
    IVideoRender::Rotate mRotate = IVideoRender::Rotate_None;
    IVideoRender::Scale mScale = IVideoRender::Scale_AspectFit;
    IVideoRender::Flip mFlip = IVideoRender::Flip_None;


    IVideoRender::MaskMode mMode = IVideoRender::Mask_None;
    std::string mMaskVapData;
    std::string mVapInfo;
    bool mMaskInfoChanged = false;

    std::unique_ptr<VapAnimateConfig> mVapConfig;
    std::unique_ptr<MixRenderer> mMixRender;

    OpenGLVideo *mOpenGL = nullptr;
    QMatrix4x4 matrix;
};


#endif//SOURCE_XXQYUVPROGRAMCONTEXT_H
