#include "GiftEffectRender.h"

GiftEffectRender::GiftEffectRender(opengl_vtable_t *vt, video_format_t *fmt, const std::string &vapInfo, IVideoRender::MaskMode mode,
                                   const std::string &data)
    : vt(vt), fmt(fmt), mVapInfo(vapInfo), mMode(mode), mMaskVapData(data)
{
    vt->GenFramebuffers(1, &videoFrameFbo);
    vt->BindFramebuffer(GL_FRAMEBUFFER, videoFrameFbo);

    vt->GenTextures(1, &videoFrameTexture);
    vt->BindTexture(GL_TEXTURE_2D, videoFrameTexture);
    vt->TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fmt->i_visible_width, fmt->i_visible_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    vt->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    vt->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    vt->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, videoFrameTexture, 0);
    GLenum status = vt->CheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        printf("GLError BERender::drawFace \n");
    }

    vt->BindFramebuffer(GL_FRAMEBUFFER, 0);

    initPrgm();
    updateMaskInfo();
}

GiftEffectRender::~GiftEffectRender()
{
    vt->DeleteTextures(1, &videoFrameTexture);
    vt->DeleteFramebuffers(1, &videoFrameFbo);
    vt->DeleteProgram(prgm);
}

void GiftEffectRender::bindFbo()
{
    vt->GetIntegerv(GL_FRAMEBUFFER_BINDING, &last_fbo);
    vt->BindFramebuffer(GL_FRAMEBUFFER, videoFrameFbo);
    vt->Viewport(0, 0, fmt->i_visible_width, fmt->i_visible_height);
}

void GiftEffectRender::releaseFbo()
{
    vt->BindFramebuffer(GL_FRAMEBUFFER, last_fbo);
}

void GiftEffectRender::draw()
{
    updateTransform();

    vt->Viewport(0, 0, viewWidth, viewHeight);

    vt->BindBuffer(GL_ARRAY_BUFFER, 0);//在不使用vbo的情况下，这里需要执行这句话，否则绘制不出来。

    vt->ActiveTexture(GL_TEXTURE0);
    vt->BindTexture(GL_TEXTURE_2D, videoFrameTexture);

    vt->UseProgram(prgm);

    vt->UniformMatrix4fv(projection, 1, GL_FALSE, (GLfloat *) mUProjection);
    vt->Uniform2f(uflip, mFlipCoords[0], mFlipCoords[1]);
    vt->VertexAttribPointer(position, 2, GL_FLOAT, GL_FALSE, 0, mDrawRegion);
    vt->VertexAttribPointer(RGBTexCoord, 2, GL_FLOAT, GL_FALSE, 0, mTextureCoords);
    vt->VertexAttribPointer(alphaTexCoord, 2, GL_FLOAT, GL_FALSE, 0, mAlphaTextureCoords);

    vt->DrawArrays(GL_TRIANGLE_STRIP, 0, 4);


    vt->UseProgram(0);
}

void GiftEffectRender::initPrgm()
{
    auto m_vertexShader = vt->CreateShader(GL_VERTEX_SHADER);
    auto m_fragmentShader = vt->CreateShader(GL_FRAGMENT_SHADER);
    char *vstr = R"(
			attribute vec2 position;
			attribute vec2 RGBTexCoord;
			attribute vec2 alphaTexCoord;
 
			varying vec2 RGBTexCoordVarying;
			varying vec2 alphaTexCoordVarying;

			uniform mat4 u_projection;
			uniform vec2 u_flipMatrix;
 
			void main()
			{
				gl_Position = u_projection * vec4(position, 0.0, 1.0) * vec4(u_flipMatrix, 1.0, 1.0);
				RGBTexCoordVarying = RGBTexCoord;
				alphaTexCoordVarying = alphaTexCoord;
			}
			)";


    char *fstr = R"(
    		varying vec2 RGBTexCoordVarying;
    		varying vec2 alphaTexCoordVarying;
    
    		uniform sampler2D SamplerImage;
    
    		void main()
    		{
    			vec3 rgb_rgb;
    			vec3 rgb_alpha;

    			rgb_rgb = texture2D(SamplerImage, RGBTexCoordVarying).rgb;
    			rgb_alpha = texture2D(SamplerImage, alphaTexCoordVarying).rgb;
    
    			gl_FragColor = vec4(rgb_rgb,rgb_alpha.r);
    		}
    		)";

    vt->ShaderSource(m_vertexShader, 1, &vstr, NULL);
    vt->CompileShader(m_vertexShader);

    GLint success;
    GLchar infoLog[1024];
    vt->GetShaderiv(m_vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        vt->GetShaderInfoLog(m_vertexShader, 1024, NULL, infoLog);
    }

    vt->ShaderSource(m_fragmentShader, 1, &fstr, NULL);
    vt->CompileShader(m_fragmentShader);

    vt->GetShaderiv(m_fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        vt->GetShaderInfoLog(m_fragmentShader, 1024, NULL, infoLog);
    }

    prgm = vt->CreateProgram();
    vt->AttachShader(prgm, m_vertexShader);
    vt->AttachShader(prgm, m_fragmentShader);
    vt->LinkProgram(prgm);

    vt->GetProgramiv(prgm, GL_LINK_STATUS, &success);
    if (!success) {
        vt->GetProgramInfoLog(prgm, 1024, NULL, infoLog);
    }

    vt->DeleteShader(m_vertexShader);
    vt->DeleteShader(m_fragmentShader);

    position = vt->GetAttribLocation(prgm, "position");
    RGBTexCoord = vt->GetAttribLocation(prgm, "RGBTexCoord");
    alphaTexCoord = vt->GetAttribLocation(prgm, "alphaTexCoord");
    SamplerImage = vt->GetUniformLocation(prgm, "SamplerImage");
    projection = vt->GetUniformLocation(prgm, "u_projection");
    uflip = vt->GetUniformLocation(prgm, "u_flipMatrix");

    vt->UseProgram(prgm);
    vt->EnableVertexAttribArray(position);
    vt->EnableVertexAttribArray(RGBTexCoord);
    vt->EnableVertexAttribArray(alphaTexCoord);
    vt->UseProgram(0);
}

void GiftEffectRender::updateMaskInfo()
{
    mVapConfig = std::make_unique<VapAnimateConfig>();

    if (mVapInfo.length() == 0) {
        mVapConfig->videoWidth = fmt->i_visible_width;
        mVapConfig->videoHeight = fmt->i_visible_height;
        switch (mMode) {
            case IVideoRender::Mask_None:
                mVapConfig->width = fmt->i_visible_width;
                mVapConfig->height = fmt->i_visible_height;
                mVapConfig->rgbPointRect = Rect(0, 0, fmt->i_visible_width, fmt->i_visible_height);
                break;
            case IVideoRender::Mask_Left:
                mVapConfig->width = fmt->i_visible_width / 2;
                mVapConfig->height = fmt->i_visible_height;
                mVapConfig->alphaPointRect = Rect(0, 0, fmt->i_visible_width / 2, fmt->i_visible_height);
                mVapConfig->rgbPointRect = Rect(fmt->i_visible_width / 2, 0, fmt->i_visible_width, fmt->i_visible_height);
                break;
            case IVideoRender::Mask_Right:
                mVapConfig->width = fmt->i_visible_width / 2;
                mVapConfig->height = fmt->i_visible_height;
                mVapConfig->alphaPointRect = Rect(fmt->i_visible_width / 2, 0, fmt->i_visible_width, fmt->i_visible_height);
                mVapConfig->rgbPointRect = Rect(0, 0, fmt->i_visible_width / 2, fmt->i_visible_height);
                break;
            case IVideoRender::Mask_Top:
                mVapConfig->width = fmt->i_visible_width;
                mVapConfig->height = fmt->i_visible_height / 2;
                mVapConfig->alphaPointRect = Rect(0, 0, fmt->i_visible_width, fmt->i_visible_height / 2);
                mVapConfig->rgbPointRect = Rect(0, fmt->i_visible_height / 2, fmt->i_visible_width, fmt->i_visible_height);
                break;
            case IVideoRender::Mask_Down:
                mVapConfig->width = fmt->i_visible_width;
                mVapConfig->height = fmt->i_visible_height / 2;
                mVapConfig->alphaPointRect = Rect(0, fmt->i_visible_height / 2, fmt->i_visible_width, fmt->i_visible_height);
                mVapConfig->rgbPointRect = Rect(0, 0, fmt->i_visible_width, fmt->i_visible_height / 2);
                break;
            default:
                break;
        }

        updateTextureCoords();
        return;
    }

    CicadaJSONItem json(mVapInfo);
    CicadaJSONItem info = json.getItem("info");
    mVapConfig->version = info.getInt("v", 0);
    mVapConfig->totalFrames = info.getInt("f", 0);
    mVapConfig->width = info.getInt("w", 0);
    mVapConfig->height = info.getInt("h", 0);
    mVapConfig->videoWidth = info.getInt("videoW", 0);
    mVapConfig->videoHeight = info.getInt("videoH", 0);
    mVapConfig->orien = info.getInt("orien", 0);
    mVapConfig->fps = info.getInt("fps", 0);
    mVapConfig->isMix = info.getInt("isVapx", 0) != 0;

    CicadaJSONArray rgbArray = info.getItem("rgbFrame");
    assert(rgbArray.getSize() == 4);
    mVapConfig->rgbPointRect =
            Rect(rgbArray.getItem(0).getInt(), rgbArray.getItem(1).getInt(), rgbArray.getItem(0).getInt() + rgbArray.getItem(2).getInt(),
                 rgbArray.getItem(1).getInt() + rgbArray.getItem(3).getInt());

    CicadaJSONArray alphaArray = info.getItem("aFrame");
    assert(alphaArray.getSize() == 4);
    mVapConfig->alphaPointRect = Rect(alphaArray.getItem(0).getInt(), alphaArray.getItem(1).getInt(),
                                      alphaArray.getItem(0).getInt() + alphaArray.getItem(2).getInt(),
                                      alphaArray.getItem(1).getInt() + alphaArray.getItem(3).getInt());

    updateTextureCoords();
    //mMixRender = std::make_unique<MixRenderer>(mPixelFormat);
    //mMixRender->setMixResource(mMaskVapData);
    //mMixRender->setVapxInfo(json);
}

void GiftEffectRender::updateTextureCoords()
{
    int leftX, rightX, topY, bottomY;

    leftX = mVapConfig->rgbPointRect.left;
    rightX = mVapConfig->rgbPointRect.right;
    topY = mVapConfig->rgbPointRect.top;
    bottomY = mVapConfig->rgbPointRect.bottom;

    updateAlphaTextureCoords();

    mTextureCoords[0] = (float) leftX / (float) fmt->i_width;
    mTextureCoords[1] = (float) topY / (float) fmt->i_visible_height;

    mTextureCoords[2] = (float) rightX / (float) fmt->i_width;
    mTextureCoords[3] = (float) topY / (float) fmt->i_visible_height;

    mTextureCoords[4] = (float) leftX / (float) fmt->i_width;
    mTextureCoords[5] = (float) bottomY / (float) fmt->i_visible_height;

    mTextureCoords[6] = (float) rightX / (float) fmt->i_width;
    mTextureCoords[7] = (float) bottomY / (float) fmt->i_visible_height;
}

void GiftEffectRender::updateAlphaTextureCoords()
{
    mAlphaTextureCoords[0] = (float) mVapConfig->alphaPointRect.left / (float) fmt->i_width;
    mAlphaTextureCoords[1] = (float) mVapConfig->alphaPointRect.top / (float) fmt->i_visible_height;

    mAlphaTextureCoords[2] = (float) mVapConfig->alphaPointRect.right / (float) fmt->i_width;
    mAlphaTextureCoords[3] = (float) mVapConfig->alphaPointRect.top / (float) fmt->i_visible_height;

    mAlphaTextureCoords[4] = (float) mVapConfig->alphaPointRect.left / (float) fmt->i_width;
    mAlphaTextureCoords[5] = (float) mVapConfig->alphaPointRect.bottom / (float) fmt->i_visible_height;

    mAlphaTextureCoords[6] = (float) mVapConfig->alphaPointRect.right / (float) fmt->i_width;
    mAlphaTextureCoords[7] = (float) mVapConfig->alphaPointRect.bottom / (float) fmt->i_visible_height;
}

void GiftEffectRender::updateProjection()
{
    mUProjection[0][0] = 2.0f;
    mUProjection[0][1] = 0.0f;
    mUProjection[0][2] = 0.0f;
    mUProjection[0][3] = 0.0f;
    mUProjection[1][0] = 0.0f;
    mUProjection[1][1] = 2.0f;
    mUProjection[1][2] = 0.0f;
    mUProjection[1][3] = 0.0f;
    mUProjection[2][0] = 0.0f;
    mUProjection[2][1] = 0.0f;
    mUProjection[2][2] = 0.0f;
    mUProjection[2][3] = 0.0f;
    mUProjection[3][0] = -1.0f;
    mUProjection[3][1] = -1.0f;
    mUProjection[3][2] = 0.0f;
    mUProjection[3][3] = 1.0f;

    if (viewWidth != 0 && viewHeight != 0) {
        mUProjection[0][0] = 2.0f / viewWidth;
        mUProjection[1][1] = 2.0f / viewHeight;
    }
}

void GiftEffectRender::updateDrawRegion()
{
    float windowWidth = viewWidth;
    float windowHeight = viewHeight;
    float draw_width = viewWidth;
    float draw_height = viewHeight;
    float realWidth = 0;
    float realHeight = 0;
    off_x = 0;
    off_y = 0;

    int targetHeight = mVapConfig ? mVapConfig->height : fmt->i_visible_height;
    double targetDar = mVapConfig ? (double) mVapConfig->width / (double) mVapConfig->height
                                  : (double) fmt->i_visible_width / (double) fmt->i_visible_height;

    if (mRotate == IVideoRender::Rotate::Rotate_90 || mRotate == IVideoRender::Rotate::Rotate_270) {
        realWidth = targetHeight;
        realHeight = static_cast<float>(targetHeight * targetDar);
    } else {
        realWidth = static_cast<float>(targetHeight * targetDar);
        realHeight = targetHeight;
    }

    mScaleWidth = windowWidth / realWidth;
    mScaleHeight = windowHeight / realHeight;

    if (mScale == IVideoRender::Scale::Scale_AspectFit) {
        if (mScaleWidth >= mScaleHeight) {
            draw_width = mScaleHeight * realWidth;
            off_x = (windowWidth - draw_width) / 2;
            mScaleWidth = mScaleHeight;
        } else {
            draw_height = mScaleWidth * realHeight;
            off_y = (windowHeight - draw_height) / 2;
            mScaleHeight = mScaleWidth;
        }
    } else if (mScale == IVideoRender::Scale::Scale_AspectFill) {
        if (mScaleWidth < mScaleHeight) {
            draw_width = mScaleHeight * realWidth;
            off_x = (windowWidth - draw_width) / 2;
            mScaleWidth = mScaleHeight;
        } else {
            draw_height = mScaleWidth * realHeight;
            off_y = (windowHeight - draw_height) / 2;
            mScaleHeight = mScaleWidth;
        }
    }

    if (mRotate == IVideoRender::Rotate::Rotate_None) {
        mDrawRegion[0] = (GLfloat)(off_x);
        mDrawRegion[1] = (GLfloat)(off_y);
        mDrawRegion[2] = (GLfloat)(off_x + draw_width);
        mDrawRegion[3] = (GLfloat)(off_y);
        mDrawRegion[4] = (GLfloat)(off_x);
        mDrawRegion[5] = (GLfloat)(off_y + draw_height);
        mDrawRegion[6] = (GLfloat)(off_x + draw_width);
        mDrawRegion[7] = (GLfloat)(off_y + draw_height);
    } else if (mRotate == IVideoRender::Rotate::Rotate_90) {
        mDrawRegion[0] = (GLfloat)(off_x);
        mDrawRegion[1] = (GLfloat)(off_y + draw_height);
        mDrawRegion[2] = (GLfloat)(off_x);
        mDrawRegion[3] = (GLfloat)(off_y);
        mDrawRegion[4] = (GLfloat)(off_x + draw_width);
        mDrawRegion[5] = (GLfloat)(off_y + draw_height);
        mDrawRegion[6] = (GLfloat)(off_x + draw_width);
        mDrawRegion[7] = (GLfloat)(off_y);
    } else if (mRotate == IVideoRender::Rotate::Rotate_180) {
        mDrawRegion[0] = (GLfloat)(off_x + draw_width);
        mDrawRegion[1] = (GLfloat)(off_y + draw_height);
        mDrawRegion[2] = (GLfloat)(off_x);
        mDrawRegion[3] = (GLfloat)(off_y + draw_height);
        mDrawRegion[4] = (GLfloat)(off_x + draw_width);
        mDrawRegion[5] = (GLfloat)(off_y);
        mDrawRegion[6] = (GLfloat)(off_x);
        mDrawRegion[7] = (GLfloat)(off_y);
    } else if (mRotate == IVideoRender::Rotate::Rotate_270) {
        mDrawRegion[0] = (GLfloat)(off_x + draw_width);
        mDrawRegion[1] = (GLfloat)(off_y);
        mDrawRegion[2] = (GLfloat)(off_x + draw_width);
        mDrawRegion[3] = (GLfloat)(off_y + draw_height);
        mDrawRegion[4] = (GLfloat)(off_x);
        mDrawRegion[5] = (GLfloat)(off_y);
        mDrawRegion[6] = (GLfloat)(off_x);
        mDrawRegion[7] = (GLfloat)(off_y + draw_height);
    }
}

void GiftEffectRender::updateTransform()
{
    if (mProjectionChanged) {
        updateProjection();
        mProjectionChanged = false;

        vt->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    if (mRegionChanged) {
        updateDrawRegion();
        mRegionChanged = false;
    }

    if (mTextureCoordsChanged) {
        updateTextureCoords();
        mTextureCoordsChanged = false;
    }

    if (mFlipChanged) {
        updateFlipCoords();
        mFlipChanged = false;
    }
}

void GiftEffectRender::updateFlipCoords()
{
    switch (mFlip) {
        case IVideoRender::Flip_None:
            mFlipCoords[0] = 1.0f;
            mFlipCoords[1] = 1.0f;
            break;
        case IVideoRender::Flip_Horizontal:
            mFlipCoords[0] = -1.0f;
            mFlipCoords[1] = 1.0f;
            break;
        case IVideoRender::Flip_Vertical:
            mFlipCoords[0] = 1.0f;
            mFlipCoords[1] = -1.0f;
            break;
        case IVideoRender::Flip_Both:
            mFlipCoords[0] = -1.0f;
            mFlipCoords[1] = -1.0f;
            break;
        default:
            break;
    }
}

bool GiftEffectRender::maskInfoChanged(const std::string &vapInfo, IVideoRender::MaskMode mode, const std::string &data)
{
    return vapInfo != mVapInfo || mode != mMode || data != mMaskVapData;
}