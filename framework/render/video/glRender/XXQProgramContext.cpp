#define LOG_TAG "XXQYUVProgramContext"

#include "XXQProgramContext.h"
#include <render/video/glRender/base/utils.h>
#include <utils/AFMediaType.h>
#include <utils/CicadaJSON.h>
#include <utils/frame_work_log.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

const GLfloat kColorConversion601FullRange[3][3] = {{1.0, 1.0, 1.0}, {0.0, -0.343, 1.765}, {1.4, -0.711, 0.0}};

static void createTexCoordsArray(int width, int height, Rect pointRect, GLfloat *ret)
{
    ret[0] = (float) pointRect.left / (float) width;
    ret[1] = (float) pointRect.top / (float) height;

    ret[2] = (float) pointRect.right / (float) width;
    ret[3] = (float) pointRect.top / (float) height;

    ret[4] = (float) pointRect.left / (float) width;
    ret[5] = (float) pointRect.bottom / (float) height;

    ret[6] = (float) pointRect.right / (float) width;
    ret[7] = (float) pointRect.bottom / (float) height;
};

static void rotate90(GLfloat *array)
{
    // 0->2 1->0 3->1 2->3
    float tx = array[0];
    float ty = array[1];

    // 1->0
    array[0] = array[2];
    array[1] = array[3];

    // 3->1
    array[2] = array[6];
    array[3] = array[7];

    // 2->3
    array[6] = array[4];
    array[7] = array[5];

    // 0->2
    array[4] = tx;
    array[5] = ty;
}

MixRenderer::MixRenderer()
{
    m_mixVertexShader = R"(
			attribute vec2 a_Position;
			attribute vec2 a_TextureSrcCoordinates;
			attribute vec2 a_TextureMaskCoordinates;
			varying vec2 v_TextureSrcCoordinates;
			varying vec2 v_TextureMaskCoordinates;
			uniform mat4 u_MixMatrix;
			void main()
			{
				v_TextureSrcCoordinates = a_TextureSrcCoordinates;
				v_TextureMaskCoordinates = a_TextureMaskCoordinates;
				gl_Position = u_MixMatrix * vec4(a_Position, 0.0, 1.0) * vec4(1.0, -1.0, 1.0, 1.0);
			}
	)";

    m_mixFragmentShader = R"(
			uniform sampler2D u_TextureMaskUnitY;
			uniform sampler2D u_TextureMaskUnitU;
			uniform sampler2D u_TextureMaskUnitV;
			uniform sampler2D u_TextureSrcUnit;
			uniform int u_isFill;
			uniform vec4 u_Color;
			varying vec2 v_TextureSrcCoordinates;
			varying vec2 v_TextureMaskCoordinates;
			uniform mat3 colorConversionMatrix;
			void main()
			{
				vec3 yuv_rgb;
				vec3 rgb_rgb;
				vec4 srcRgba = texture2D(u_TextureSrcUnit, v_TextureSrcCoordinates);

				yuv_rgb.x = (texture2D(u_TextureMaskUnitY, v_TextureMaskCoordinates).r) - 16.0/255.0;
				yuv_rgb.y = (texture2D(u_TextureMaskUnitU, v_TextureMaskCoordinates).r - 0.5);
			    yuv_rgb.z = (texture2D(u_TextureMaskUnitV, v_TextureMaskCoordinates).r - 0.5);
				rgb_rgb = colorConversionMatrix * yuv_rgb;

				float isFill = step(0.5, float(u_isFill));
				vec4 srcRgbaCal = isFill * vec4(u_Color.r, u_Color.g, u_Color.b, srcRgba.a) + (1.0 - isFill) * srcRgba;
				gl_FragColor = vec4(srcRgbaCal.r, srcRgbaCal.g, srcRgbaCal.b, srcRgba.a * rgb_rgb.r);
			}
	)";

    compileMixShader();
}

MixRenderer::~MixRenderer()
{
    clearMixSrc();

    glDisableVertexAttribArray(aPositionLocation);
    glDisableVertexAttribArray(aTextureSrcCoordinatesLocation);
    glDisableVertexAttribArray(aTextureMaskCoordinatesLocation);
    glDetachShader(m_MixshaderProgram, mVertShader);
    glDetachShader(m_MixshaderProgram, mFragmentShader);
    glDeleteShader(mVertShader);
    glDeleteShader(mFragmentShader);
    glDeleteProgram(m_MixshaderProgram);
}

void MixRenderer::compileMixShader()
{
    AF_LOGD("create MixRenderer Program ");

    m_MixshaderProgram = glCreateProgram();
    int mInitRet = IProgramContext::compileShader(&mVertShader, m_mixVertexShader.c_str(), GL_VERTEX_SHADER);

    if (mInitRet != 0) {
        AF_LOGE("compileShader mVertShader failed. ret = %d ", mInitRet);
        return;
    }

    mInitRet = IProgramContext::compileShader(&mFragmentShader, m_mixFragmentShader.c_str(), GL_FRAGMENT_SHADER);

    if (mInitRet != 0) {
        AF_LOGE("compileShader mFragmentShader failed. ret = %d ", mInitRet);
        return;
    }

    glAttachShader(m_MixshaderProgram, mVertShader);
    glAttachShader(m_MixshaderProgram, mFragmentShader);
    glLinkProgram(m_MixshaderProgram);
    GLint status;
    glGetProgramiv(m_MixshaderProgram, GL_LINK_STATUS, &status);

    if (status != GL_TRUE) {
        int length = 0;
        GLchar glchar[256] = {0};
        glGetProgramInfoLog(m_MixshaderProgram, 256, &length, glchar);
        AF_LOGW("linkProgram  error is %s \n", glchar);
        return;
    }

    glUseProgram(m_MixshaderProgram);
    uTextureSrcUnitLocation = glGetUniformLocation(m_MixshaderProgram, "u_TextureSrcUnit");
    uTextureMaskUnitLocationY = glGetUniformLocation(m_MixshaderProgram, "u_TextureMaskUnitY");
    uTextureMaskUnitLocationU = glGetUniformLocation(m_MixshaderProgram, "u_TextureMaskUnitU");
    uTextureMaskUnitLocationV = glGetUniformLocation(m_MixshaderProgram, "u_TextureMaskUnitV");
    uMixMatrix = glGetUniformLocation(m_MixshaderProgram, "u_MixMatrix");
    uIsFillLocation = glGetUniformLocation(m_MixshaderProgram, "u_isFill");
    uColorLocation = glGetUniformLocation(m_MixshaderProgram, "u_Color");
    mixColorConversionMatrix = glGetUniformLocation(m_MixshaderProgram, "colorConversionMatrix");

    aPositionLocation = glGetAttribLocation(m_MixshaderProgram, "a_Position");
    aTextureSrcCoordinatesLocation = glGetAttribLocation(m_MixshaderProgram, "a_TextureSrcCoordinates");
    aTextureMaskCoordinatesLocation = glGetAttribLocation(m_MixshaderProgram, "a_TextureMaskCoordinates");

    glEnableVertexAttribArray(aPositionLocation);
    glEnableVertexAttribArray(aTextureSrcCoordinatesLocation);
    glEnableVertexAttribArray(aTextureMaskCoordinatesLocation);

    mReady = true;
}

void MixRenderer::renderMix(int index, const MixParam &param)
{
    if (m_allFrames.find(index) == m_allFrames.end()) return;

    const FrameSet &set = m_allFrames[index];
    for (auto iter = set.m_frames.begin(); iter != set.m_frames.end(); iter++) {
        const VAPFrame &frame = iter->second;
        renderMixPrivate(frame, m_srcMap[frame.srcId], param);
    }
}

void MixRenderer::renderMixPrivate(const VAPFrame &frame, const MixSrc &src, const MixParam &param)
{
    auto createVertexArray = [](Rect pointRect, GLfloat *ret, const MixParam &p) {
        if (p.rotate == IVideoRender::Rotate::Rotate_None) {
            float left = pointRect.left * p.scaleW;
            float top = pointRect.top * p.scaleH;
            float right = pointRect.right * p.scaleW;
            float bottom = pointRect.bottom * p.scaleH;

            ret[0] = left + p.offsetX;
            ret[1] = top + p.offsetY;
            ret[2] = right + p.offsetX;
            ret[3] = top + p.offsetY;
            ret[4] = left + p.offsetX;
            ret[5] = bottom + p.offsetY;
            ret[6] = right + p.offsetX;
            ret[7] = bottom + p.offsetY;
        } else if (p.rotate == IVideoRender::Rotate::Rotate_90) {
            float left = pointRect.top * p.scaleW;
            float top = (p.ow - pointRect.right) * p.scaleH;
            float right = pointRect.bottom * p.scaleW;
            float bottom = (p.ow - pointRect.left) * p.scaleH;

            ret[0] = left + p.offsetX;
            ret[1] = bottom + p.offsetY;
            ret[2] = left + p.offsetX;
            ret[3] = top + p.offsetY;
            ret[4] = right + p.offsetX;
            ret[5] = bottom + p.offsetY;
            ret[6] = right + p.offsetX;
            ret[7] = top + p.offsetY;

        } else if (p.rotate == IVideoRender::Rotate::Rotate_180) {
            float left = (p.ow - pointRect.right) * p.scaleW;
            float top = (p.oh - pointRect.bottom) * p.scaleH;
            float right = (p.ow - pointRect.left) * p.scaleW;
            float bottom = (p.oh - pointRect.top) * p.scaleH;

            ret[0] = right + p.offsetX;
            ret[1] = bottom + p.offsetY;
            ret[2] = left + p.offsetX;
            ret[3] = bottom + p.offsetY;
            ret[4] = right + p.offsetX;
            ret[5] = top + p.offsetY;
            ret[6] = left + p.offsetX;
            ret[7] = top + p.offsetY;
        } else if (p.rotate == IVideoRender::Rotate::Rotate_270) {
            float left = (p.oh - pointRect.bottom) * p.scaleW;
            float top = pointRect.left * p.scaleH;
            float right = (p.oh - pointRect.top) * p.scaleW;
            float bottom = pointRect.right * p.scaleH;

            ret[0] = right + p.offsetX;
            ret[1] = top + p.offsetY;
            ;
            ret[2] = right + p.offsetX;
            ret[3] = bottom + p.offsetY;
            ;
            ret[4] = left + p.offsetX;
            ret[5] = top + p.offsetY;
            ;
            ret[6] = left + p.offsetX;
            ret[7] = bottom + p.offsetY;
            ;
        }
    };

    if (src.srcTexture == 0) return;

    glUseProgram(m_MixshaderProgram);

    glUniformMatrix3fv(mixColorConversionMatrix, 1, GL_FALSE, (GLfloat *) kColorConversion601FullRange);
    glUniformMatrix4fv(uMixMatrix, 1, GL_FALSE, (GLfloat *) param.matrix);

    createVertexArray(frame.frame, m_mixVertexArray, param);
    glVertexAttribPointer(aPositionLocation, 2, GL_FLOAT, GL_FALSE, 0, m_mixVertexArray);

    genSrcCoordsArray(m_mixSrcArray, frame.frame.width(), frame.frame.height(), src.w, src.h, src.fitType);
    glVertexAttribPointer(aTextureSrcCoordinatesLocation, 2, GL_FLOAT, GL_FALSE, 0, m_mixSrcArray);

    createTexCoordsArray(param.videoW, param.videoH, frame.mFrame, m_maskArray);
    if (frame.mt == 90) rotate90(m_maskArray);

    glVertexAttribPointer(aTextureMaskCoordinatesLocation, 2, GL_FLOAT, GL_FALSE, 0, m_maskArray);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, param.videoTexture[0]);
    glUniform1i(uTextureMaskUnitLocationY, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, param.videoTexture[1]);
    glUniform1i(uTextureMaskUnitLocationU, 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, param.videoTexture[2]);
    glUniform1i(uTextureMaskUnitLocationV, 2);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, src.srcTexture);
    glUniform1i(uTextureSrcUnitLocation, 3);

    if (src.srcType == MixSrc::TXT) {
        glUniform1i(uIsFillLocation, 1);
        glUniform4f(uColorLocation, src.color[0], src.color[1], src.color[2], src.color[3]);
    } else {
        glUniform1i(uIsFillLocation, 0);
        glUniform4f(uColorLocation, 0.f, 0.f, 0.f, 0.f);
    }

    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisable(GL_BLEND);
}

void MixRenderer::setVapxInfo(const CicadaJSONItem &json)
{
    clearMixSrc();
    CicadaJSONArray arr = json.getItem("src");
    for (int i = 0; i < arr.getSize(); i++) {
        CicadaJSONItem obj = arr.getItem(i);
        m_srcMap.insert({obj.getString("srcId"), MixSrc(obj)});
    }

    m_allFrames.clear();
    CicadaJSONArray frameArr = json.getItem("frame");
    for (int i = 0; i < frameArr.getSize(); i++) {
        FrameSet s(frameArr.getItem(i));
        m_allFrames.insert({s.index, s});
    }

    prepareMixResource();
}

void MixRenderer::setMixResource(const std::string &rcStr)
{
    m_mixResource.clear();

    CicadaJSONItem json(rcStr);
    m_mixResource = json.getStringMap();
}

void MixRenderer::genSrcCoordsArray(GLfloat *array, int fw, int fh, int sw, int sh, MixSrc::FitType fitType)
{
    if (fitType == MixSrc::CENTER_FULL) {
        if (fw <= sw && fh <= sh) {
            // 中心对齐，不拉伸
            int gw = (sw - fw) / 2;
            int gh = (sh - fh) / 2;
            createTexCoordsArray(sw, sh, Rect(gw, gh, fw + gw, fh + gh), array);
        } else {// centerCrop
            float fScale = fw * 1.0f / fh;
            float sScale = sw * 1.0f / sh;
            Rect srcRect;
            if (fScale > sScale) {
                int w = sw;
                int x = 0;
                int h = (sw / fScale);
                int y = (sh - h) / 2;

                srcRect = Rect(x, y, w + x, h + y);
            } else {
                int h = sh;
                int y = 0;
                int w = (sh * fScale);
                int x = (sw - w) / 2;
                srcRect = Rect(x, y, w + x, h + y);
            }
            createTexCoordsArray(sw, sh, srcRect, array);
        }
    } else {// 默认 fitXY
        createTexCoordsArray(fw, fh, Rect(0, 0, fw, fh), array);
    }
}

void MixRenderer::prepareMixResource()
{
    for (auto iter = m_srcMap.begin(); iter != m_srcMap.end(); iter++) {
        MixSrc &mixSrc = iter->second;
        auto resourceStr = m_mixResource[mixSrc.srcTag];
        if (mixSrc.srcType == MixSrc::IMG) {
            if (resourceStr.length() != 0) {
                //mixSrc.srcTexture = new QOpenGLTexture(QImage(resourceStr));
                glGenTextures(1, &mixSrc.srcTexture);
                int w, h, c;
                auto bytes = stbi_load(resourceStr.c_str(), &w, &h, &c, 4);//rgba
                glBindTexture(GL_TEXTURE_2D, mixSrc.srcTexture);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);
                stbi_image_free(bytes);
            }
        } else if (mixSrc.srcType == MixSrc::TXT) {
            /*QImage textImage(QSize(mixSrc.w, mixSrc.h), QImage::Format_RGBA8888);
            textImage.fill(Qt::transparent);
            QPainter p(&textImage);
            QFont f;
            int targetSize = 32;
            f.setFamily("Microsoft YaHei");
            f.setBold(mixSrc.style == MixSrc::BOLD);
            while (true) {
                f.setPixelSize(targetSize);
                QFontMetrics fm(f);
                QRect boundRect = fm.boundingRect(resourceStr);
                if (boundRect.width() <= mixSrc.w && boundRect.height() <= mixSrc.h) break;
                targetSize--;
            }
            QPen pen;
            pen.setColor(mixSrc.color);
            p.setPen(pen);
            p.setFont(f);
            QRect drawRect(0, 0, textImage.width(), textImage.height());
            p.drawText(drawRect, resourceStr, Qt::AlignHCenter | Qt::AlignVCenter);
            mixSrc.srcTexture = new QOpenGLTexture(textImage);*/
        }
    }
}

void MixRenderer::clearMixSrc()
{
    for (auto iter = m_srcMap.begin(); iter != m_srcMap.end(); iter++) {
        MixSrc &mixSrc = iter->second;
        if (mixSrc.srcTexture != 0) {
            glDeleteTextures(1, &mixSrc.srcTexture);
        }
    }
    m_srcMap.clear();
}

static const char YUV_VERTEX_SHADER[] = R"(
        attribute vec2 a_position;
        attribute vec2 a_texCoord;
		attribute vec2 a_alpha_texCoord;
        uniform mat4 u_projection;
		uniform vec2 u_flipMatrix;
        varying vec2 v_texCoord;
		varying vec2 v_alpha_texCoord;

        void main() {
            gl_Position = u_projection * vec4(a_position, 0.0, 1.0) * vec4(u_flipMatrix, 1.0, 1.0) * vec4(1.0, -1.0, 1.0, 1.0);
            v_texCoord  = a_texCoord;
			v_alpha_texCoord = a_alpha_texCoord;
        }
)";

static const char YUV_FRAGMENT_SHADER[] = R"(
#ifdef GL_ES
        precision mediump float;
#endif
        uniform sampler2D y_tex;
        uniform sampler2D u_tex;
        uniform sampler2D v_tex;

        uniform mat3      uColorSpace;
        uniform vec3      uColorRange;
		uniform int		  uAlphaBlend;

        varying vec2 v_texCoord;
		varying vec2 v_alpha_texCoord;

        void main() {
            vec3 yuv;
            vec3 rgb;
			vec3 yuv_alpha;
            vec3 rgb_alpha;
            yuv.x = (texture2D(y_tex, v_texCoord).r - uColorRange.x / 255.0) * 255.0 / uColorRange.y;
            yuv.y = (texture2D(u_tex, v_texCoord).r - 0.5) * 255.0 / uColorRange.z;
            yuv.z = (texture2D(v_tex, v_texCoord).r - 0.5) * 255.0 / uColorRange.z;
            rgb = uColorSpace * yuv;

			if (uAlphaBlend == 1) {
				yuv_alpha.x = (texture2D(y_tex, v_alpha_texCoord).r - uColorRange.x / 255.0) * 255.0 / uColorRange.y;
				yuv_alpha.y = (texture2D(u_tex, v_alpha_texCoord).r - 0.5) * 255.0 / uColorRange.z;
				yuv_alpha.z = (texture2D(v_tex, v_alpha_texCoord).r - 0.5) * 255.0 / uColorRange.z;
				rgb_alpha = uColorSpace * yuv_alpha;

				gl_FragColor = vec4(rgb, rgb_alpha.x);
			} else 
				gl_FragColor = vec4(rgb, 1);
			}
)";

XXQYUVProgramContext::XXQYUVProgramContext()
{
    AF_LOGD("YUVProgramContext");
    updateDrawRegion();
    updateTextureCoords();
    updateUProjection();
    updateColorRange();
    updateColorSpace();
    updateFlipCoords();
}

XXQYUVProgramContext::~XXQYUVProgramContext()
{
    AF_LOGD("~YUVProgramContext");
    glDisableVertexAttribArray(mPositionLocation);
    glDisableVertexAttribArray(mTexCoordLocation);
    glDisableVertexAttribArray(mAlphaTexCoordLocation);
    glDetachShader(mProgram, mVertShader);
    glDetachShader(mProgram, mFragmentShader);
    glDeleteShader(mVertShader);
    glDeleteShader(mFragmentShader);
    glDeleteProgram(mProgram);
    glDeleteTextures(3, mYUVTextures);
}

int XXQYUVProgramContext::initProgram()
{
    AF_LOGD("createProgram ");

    mProgram = glCreateProgram();
    int mInitRet = compileShader(&mVertShader, YUV_VERTEX_SHADER, GL_VERTEX_SHADER);

    if (mInitRet != 0) {
        AF_LOGE("compileShader mVertShader failed. ret = %d ", mInitRet);
        return mInitRet;
    }

    mInitRet = compileShader(&mFragmentShader, YUV_FRAGMENT_SHADER, GL_FRAGMENT_SHADER);

    if (mInitRet != 0) {
        AF_LOGE("compileShader mFragmentShader failed. ret = %d ", mInitRet);
        return mInitRet;
    }

    glAttachShader(mProgram, mVertShader);
    glAttachShader(mProgram, mFragmentShader);
    glLinkProgram(mProgram);
    GLint status;
    glGetProgramiv(mProgram, GL_LINK_STATUS, &status);

    if (status != GL_TRUE) {
        int length = 0;
        GLchar glchar[256] = {0};
        glGetProgramInfoLog(mProgram, 256, &length, glchar);
        AF_LOGW("linkProgram  error is %s \n", glchar);
        return -1;
    }

    glUseProgram(mProgram);
    getShaderLocations();

    glEnableVertexAttribArray(mPositionLocation);
    glEnableVertexAttribArray(mTexCoordLocation);
    glEnableVertexAttribArray(mAlphaTexCoordLocation);

    createYUVTextures();

    return 0;
}

void XXQYUVProgramContext::useProgram()
{
    glUseProgram(mProgram);
}

void XXQYUVProgramContext::updateScale(IVideoRender::Scale scale)
{
    if (mScale != scale) {
        mScale = scale;
        mRegionChanged = true;
    }
}

void XXQYUVProgramContext::updateRotate(IVideoRender::Rotate rotate)
{
    if (mRotate != rotate) {
        mRotate = rotate;
        mRegionChanged = true;
    }
}

void XXQYUVProgramContext::updateFlip(IVideoRender::Flip flip)
{
    if (mFlip != flip) {
        mFlip = flip;
        mFlipChanged = true;
    }
}

void XXQYUVProgramContext::updateMaskInfo(const std::string &vapInfo, IVideoRender::MaskMode mode, const std::string &data)
{
    if (mVapInfo != vapInfo || mMode != mode || mMaskVapData != data) {
        mVapInfo = vapInfo;
        mMode = mode;
        mMaskVapData = data;
        mMaskInfoChanged = true;
    }
}

int XXQYUVProgramContext::updateFrame(std::unique_ptr<IAFFrame> &frame)
{

    if (mProgram == 0) {
        return -1;
    }

    if (frame != nullptr) {
        IAFFrame::videoInfo &videoInfo = frame->getInfo().video;
        if (mFrameWidth != videoInfo.width || mFrameHeight != videoInfo.height || mDar != videoInfo.dar) {
            mDar = videoInfo.dar;
            mFrameWidth = videoInfo.width;
            mFrameHeight = videoInfo.height;
            mRegionChanged = true;
        }

        if (mCropRect.left != videoInfo.crop_left || mCropRect.right != videoInfo.crop_right || mCropRect.top != videoInfo.crop_top ||
            mCropRect.bottom != videoInfo.crop_bottom) {
            mCropRect = {videoInfo.crop_left, videoInfo.crop_right, videoInfo.crop_top, videoInfo.crop_bottom};
            mTextureCoordsChanged = true;
        }

        int *lineSize = frame->getLineSize();
        if (lineSize != nullptr && lineSize[0] != mYLineSize) {
            mYLineSize = lineSize[0];
            mTextureCoordsChanged = true;
        }

        if (mColorSpace != videoInfo.colorSpace) {
            updateColorSpace();
            mColorSpace = videoInfo.colorSpace;
        }

        if (mColorRange != videoInfo.colorRange) {
            updateColorRange();
            mColorRange = videoInfo.colorRange;
        }
    }

    if (frame == nullptr && !mProjectionChanged && !mRegionChanged && !mTextureCoordsChanged && !mBackgroundColorChanged && !mFlipChanged) {
        //frame is null and nothing changed , don`t need redraw. such as paused.
        return -1;
    }

    bool rendered = false;
    if (mRenderingCb) {
        CicadaJSONItem params{};
        rendered = mRenderingCb(mRenderingCbUserData, frame.get(), params);
    }

    if (rendered) {
        return -1;
    }

    if (mMaskInfoChanged) {
        updateVapConfig();
        mTextureCoordsChanged = true;
        mMaskInfoChanged = false;
    }

    if (mProjectionChanged) {
        updateUProjection();
        mProjectionChanged = false;
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

    glViewport(0, 0, mWindowWidth, mWindowHeight);
    if (mBackgroundColorChanged) {
        float color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        cicada::convertToGLColor(mBackgroundColor, color);
        glClearColor(color[0], color[1], color[2], color[3]);
        mBackgroundColorChanged = false;
    }
    glClear(GL_COLOR_BUFFER_BIT);

    if (frame != nullptr) {
        fillDataToYUVTextures(frame->getData(), frame->getLineSize(), frame->getInfo().video.format);
    }

    bindYUVTextures();

    glUniform1i(mAlphaBlendLocation, mVapConfig ? 1 : 0);
    glUniformMatrix4fv(mProjectionLocation, 1, GL_FALSE, (GLfloat *) mUProjection);
    glUniformMatrix3fv(mColorSpaceLocation, 1, GL_FALSE, (GLfloat *) mUColorSpace);
    glUniform2f(mFlipCoordsLocation, mFlipCoords[0], mFlipCoords[1]);
    glUniform3f(mColorRangeLocation, mUColorRange[0], mUColorRange[1], mUColorRange[2]);
    glVertexAttribPointer(mPositionLocation, 2, GL_FLOAT, GL_FALSE, 0, mDrawRegion);
    glVertexAttribPointer(mTexCoordLocation, 2, GL_FLOAT, GL_FALSE, 0, mTextureCoords);
    glVertexAttribPointer(mAlphaTexCoordLocation, 2, GL_FLOAT, GL_FALSE, 0, mAlphaTextureCoords);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    if (mVapConfig && mVapConfig->isMix) {
        MixRenderer::MixParam param{off_x,
                                    off_y,
                                    mScaleWidth,
                                    mScaleHeight,
                                    mVapConfig->videoWidth,
                                    mVapConfig->videoHeight,
                                    mVapConfig->width,
                                    mVapConfig->height,
                                    (GLfloat **) mUProjection,
                                    mYUVTextures,
                                    mRotate};

        mMixRender->renderMix(frame->getInfo().video.frameIndex % mVapConfig->totalFrames, param);
    }
    //mMixRender->renderMix(m_frame.frameIndex() % mVapConfig->totalFrames, &mVapConfig, m_textures, matrix);
    return 0;
}

void XXQYUVProgramContext::updateVapConfig()
{
    mVapConfig = std::make_unique<VapAnimateConfig>();

    if (mVapInfo.length() == 0) {
        mVapConfig->videoWidth = mFrameWidth;
        mVapConfig->videoHeight = mFrameHeight;
        switch (mMode) {
            case IVideoRender::Mask_None:
                mVapConfig->width = mFrameWidth;
                mVapConfig->height = mFrameHeight;
                break;
            case IVideoRender::Mask_Left:
                mVapConfig->width = mFrameWidth / 2;
                mVapConfig->height = mFrameHeight;
                mVapConfig->alphaPointRect = Rect(0, 0, mFrameWidth / 2, mFrameHeight);
                mVapConfig->rgbPointRect = Rect(mFrameWidth / 2, 0, mFrameWidth, mFrameHeight);
                break;
            case IVideoRender::Mask_Right:
                mVapConfig->width = mFrameWidth / 2;
                mVapConfig->height = mFrameHeight;
                mVapConfig->alphaPointRect = Rect(mFrameWidth / 2, 0, mFrameWidth, mFrameHeight);
                mVapConfig->rgbPointRect = Rect(0, 0, mFrameWidth / 2, mFrameHeight);
                break;
            case IVideoRender::Mask_Top:
                mVapConfig->width = mFrameWidth;
                mVapConfig->height = mFrameHeight / 2;
                mVapConfig->alphaPointRect = Rect(0, 0, mFrameWidth, mFrameHeight / 2);
                mVapConfig->rgbPointRect = Rect(0, mFrameHeight / 2, mFrameWidth, mFrameHeight);
                break;
            case IVideoRender::Mask_Down:
                mVapConfig->width = mFrameWidth;
                mVapConfig->height = mFrameHeight / 2;
                mVapConfig->alphaPointRect = Rect(0, mFrameHeight / 2, mFrameWidth, mFrameHeight);
                mVapConfig->rgbPointRect = Rect(0, 0, mFrameWidth, mFrameHeight / 2);
                break;
            default:
                break;
        }

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

    mMixRender = std::make_unique<MixRenderer>();
    mMixRender->setMixResource(mMaskVapData);
    mMixRender->setVapxInfo(json);
}

void XXQYUVProgramContext::updateWindowSize(int width, int height, bool windowChanged)
{
    (void) windowChanged;
    if (mWindowWidth == width && mWindowHeight == height) {
        return;
    }

    mWindowWidth = width;
    mWindowHeight = height;

    mProjectionChanged = true;
    mRegionChanged = true;
}

void XXQYUVProgramContext::updateUProjection()
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

    if (mWindowHeight != 0 && mWindowWidth != 0) {
        mUProjection[0][0] = 2.0f / mWindowWidth;
        mUProjection[1][1] = 2.0f / mWindowHeight;
    }
}

void XXQYUVProgramContext::updateDrawRegion()
{
    if (mWindowWidth == 0 || mWindowHeight == 0 || mFrameWidth == 0 || mFrameHeight == 0) {
        mDrawRegion[0] = (GLfloat) 0;
        mDrawRegion[1] = (GLfloat) 0;
        mDrawRegion[2] = (GLfloat) 0;
        mDrawRegion[3] = (GLfloat) 0;
        mDrawRegion[4] = (GLfloat) 0;
        mDrawRegion[5] = (GLfloat) 0;
        mDrawRegion[6] = (GLfloat) 0;
        mDrawRegion[7] = (GLfloat) 0;
        return;
    }

    float windowWidth = mWindowWidth;
    float windowHeight = mWindowHeight;
    float draw_width = mWindowWidth;
    float draw_height = mWindowHeight;
    float realWidth = 0;
    float realHeight = 0;
    off_x = 0;
    off_y = 0;

    int targetHeight = mVapConfig ? mVapConfig->height : mFrameHeight;
    double targetDar = mVapConfig ? (double) mVapConfig->width / (double) mVapConfig->height : mDar;

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


void XXQYUVProgramContext::updateTextureCoords()
{
    if (mYLineSize == 0 || mFrameHeight == 0) return;

    int leftX, rightX, topY, bottomY;
    if (mVapConfig) {
        leftX = mVapConfig->rgbPointRect.left;
        rightX = mVapConfig->rgbPointRect.right;
        topY = mVapConfig->rgbPointRect.top;
        bottomY = mVapConfig->rgbPointRect.bottom;

        updateAlphaTextureCoords();
    } else {
        leftX = mCropRect.left;
        rightX = mFrameWidth - mCropRect.right;
        topY = mCropRect.top;
        bottomY = mFrameHeight - mCropRect.bottom;
    }

    mTextureCoords[0] = (float) leftX / (float) mYLineSize;
    mTextureCoords[1] = (float) topY / (float) mFrameHeight;

    mTextureCoords[2] = (float) rightX / (float) mYLineSize;
    mTextureCoords[3] = (float) topY / (float) mFrameHeight;

    mTextureCoords[4] = (float) leftX / (float) mYLineSize;
    mTextureCoords[5] = (float) bottomY / (float) mFrameHeight;

    mTextureCoords[6] = (float) rightX / (float) mYLineSize;
    mTextureCoords[7] = (float) bottomY / (float) mFrameHeight;
}

void XXQYUVProgramContext::updateAlphaTextureCoords()
{
    if (!mVapConfig || mYLineSize == 0 || mFrameHeight == 0) return;

    mAlphaTextureCoords[0] = (float) mVapConfig->alphaPointRect.left / (float) mYLineSize;
    mAlphaTextureCoords[1] = (float) mVapConfig->alphaPointRect.top / (float) mFrameHeight;

    mAlphaTextureCoords[2] = (float) mVapConfig->alphaPointRect.right / (float) mYLineSize;
    mAlphaTextureCoords[3] = (float) mVapConfig->alphaPointRect.top / (float) mFrameHeight;

    mAlphaTextureCoords[4] = (float) mVapConfig->alphaPointRect.left / (float) mYLineSize;
    mAlphaTextureCoords[5] = (float) mVapConfig->alphaPointRect.bottom / (float) mFrameHeight;

    mAlphaTextureCoords[6] = (float) mVapConfig->alphaPointRect.right / (float) mYLineSize;
    mAlphaTextureCoords[7] = (float) mVapConfig->alphaPointRect.bottom / (float) mFrameHeight;
}

void XXQYUVProgramContext::updateFlipCoords()
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


void XXQYUVProgramContext::createYUVTextures()
{
    //    AF_LOGD("createYUVTextures ");

    glDeleteTextures(3, mYUVTextures);
    glGenTextures(3, mYUVTextures);
    //Y
    glBindTexture(GL_TEXTURE_2D, mYUVTextures[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    //U
    glBindTexture(GL_TEXTURE_2D, mYUVTextures[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    //V
    glBindTexture(GL_TEXTURE_2D, mYUVTextures[2]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

void XXQYUVProgramContext::fillDataToYUVTextures(uint8_t **data, int *pLineSize, int format)
{

    int uvHeight = mFrameHeight;
    if (format == AF_PIX_FMT_YUV422P || format == AF_PIX_FMT_YUVJ422P) {
        uvHeight = mFrameHeight;
    } else if (format == AF_PIX_FMT_YUV420P || format == AF_PIX_FMT_YUVJ420P) {
        uvHeight = mFrameHeight / 2;
    }

    int yWidth = pLineSize[0];
    int uvWidth = yWidth / 2;//uvWidth may not right in some iOS simulators.

    //use linesize to fill data with texture. some android phones which below 4.4 are not performed as excepted.
    // crop the extra data when draw.
    //update y
    glBindTexture(GL_TEXTURE_2D, mYUVTextures[0]);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, pLineSize[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, yWidth, mFrameHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data[0]);
    glUniform1i(mYTexLocation, 0);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    //update u
    glBindTexture(GL_TEXTURE_2D, mYUVTextures[1]);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, pLineSize[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, uvWidth, uvHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data[1]);
    glUniform1i(mUTexLocation, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    //update v
    glBindTexture(GL_TEXTURE_2D, mYUVTextures[2]);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, pLineSize[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, uvWidth, uvHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data[2]);
    glUniform1i(mVTexLocation, 2);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}

void XXQYUVProgramContext::bindYUVTextures()
{
    //    AF_LOGD("bindYUVTextures");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mYUVTextures[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mYUVTextures[1]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mYUVTextures[2]);
}

void XXQYUVProgramContext::updateColorRange()
{
    if (mColorRange == COLOR_RANGE_LIMITIED) {
        mUColorRange[0] = 16;
        mUColorRange[1] = 235 - 16;
        mUColorRange[2] = 240 - 16;
    } else if (mColorRange == COLOR_RANGE_FULL) {
        mUColorRange[0] = 0;
        mUColorRange[1] = 255;
        mUColorRange[2] = 255;
    } else {
        mUColorRange[0] = 16;
        mUColorRange[1] = 235 - 16;
        mUColorRange[2] = 240 - 16;
    }
}

void XXQYUVProgramContext::updateColorSpace()
{
    if (mColorSpace == COLOR_SPACE_BT601) {
        mUColorSpace[0] = 1.f;
        mUColorSpace[1] = 1.f;
        mUColorSpace[2] = 1.f;
        mUColorSpace[3] = 0.0f;
        mUColorSpace[4] = -0.344136f;
        mUColorSpace[5] = 1.772f;
        mUColorSpace[6] = 1.402f;
        mUColorSpace[7] = -0.714136f;
        mUColorSpace[8] = 0.0f;
    } else if (mColorSpace == COLOR_SPACE_BT709) {
        mUColorSpace[0] = 1.f;
        mUColorSpace[1] = 1.f;
        mUColorSpace[2] = 1.f;
        mUColorSpace[3] = 0.0f;
        mUColorSpace[4] = -0.187324f;
        mUColorSpace[5] = 1.8556f;
        mUColorSpace[6] = 1.5748f;
        mUColorSpace[7] = -0.468124f;
        mUColorSpace[8] = 0.0f;
    } else if (mColorSpace == COLOR_SPACE_BT2020) {
        mUColorSpace[0] = 1.f;
        mUColorSpace[1] = 1.f;
        mUColorSpace[2] = 1.f;
        mUColorSpace[3] = 0.0f;
        mUColorSpace[4] = -0.164553f;
        mUColorSpace[5] = 1.8814f;
        mUColorSpace[6] = 1.4746f;
        mUColorSpace[7] = -0.571353f;
        mUColorSpace[8] = 0.0f;
    } else {
        mUColorSpace[0] = 1.f;
        mUColorSpace[1] = 1.f;
        mUColorSpace[2] = 1.f;
        mUColorSpace[3] = 0.0f;
        mUColorSpace[4] = -0.344136f;
        mUColorSpace[5] = 1.772f;
        mUColorSpace[6] = 1.402f;
        mUColorSpace[7] = -0.714136f;
        mUColorSpace[8] = 0.0f;
    }
}

void XXQYUVProgramContext::updateBackgroundColor(uint32_t color)
{
    if (color != mBackgroundColor) {
        mBackgroundColorChanged = true;
        mBackgroundColor = color;
    }
}

void XXQYUVProgramContext::getShaderLocations()
{
    mProjectionLocation = glGetUniformLocation(mProgram, "u_projection");
    mColorSpaceLocation = glGetUniformLocation(mProgram, "uColorSpace");
    mColorRangeLocation = glGetUniformLocation(mProgram, "uColorRange");
    mFlipCoordsLocation = glGetUniformLocation(mProgram, "u_flipMatrix");
    mPositionLocation = static_cast<GLuint>(glGetAttribLocation(mProgram, "a_position"));
    mTexCoordLocation = static_cast<GLuint>(glGetAttribLocation(mProgram, "a_texCoord"));
    mAlphaTexCoordLocation = static_cast<GLuint>(glGetAttribLocation(mProgram, "a_alpha_texCoord"));
    mYTexLocation = glGetUniformLocation(mProgram, "y_tex");
    mUTexLocation = glGetUniformLocation(mProgram, "u_tex");
    mVTexLocation = glGetUniformLocation(mProgram, "v_tex");
    mAlphaBlendLocation = glGetUniformLocation(mProgram, "uAlphaBlend");
}
