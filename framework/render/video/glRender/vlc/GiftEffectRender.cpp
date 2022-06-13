#include "GiftEffectRender.h"
#include <utils/CicadaUtils.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <unknwn.h>
#include <gdiplus.h>
#endif// Win32


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

MixRenderer::MixRenderer(opengl_vtable_t *vt) : vt(vt)
{
    m_mixVertexShader = R"(
			attribute vec2 a_Position;
			attribute vec4 a_TextureSrcCoordinates;
			varying vec2 v_TextureSrcCoordinates;
			varying vec2 v_TextureMaskCoordinates;
			uniform mat4 u_MixMatrix;
			void main()
			{
				v_TextureSrcCoordinates = a_TextureSrcCoordinates.xy;
				v_TextureMaskCoordinates = a_TextureSrcCoordinates.zw;
				gl_Position = u_MixMatrix * vec4(a_Position, 0.0, 1.0) * vec4(1, -1, 1, 1);
			}
	)";

    m_mixFragmentShader = R"(
			uniform sampler2D u_TextureMaskUnitImage;
			uniform sampler2D u_TextureSrcUnit;
			uniform int u_isFill;
			uniform vec4 u_Color;

			varying vec2 v_TextureSrcCoordinates;
			varying vec2 v_TextureMaskCoordinates;
			uniform mat3 colorConversionMatrix;

			void main()
			{
				vec3 rgb_rgb;
				vec4 srcRgba = texture2D(u_TextureSrcUnit, v_TextureSrcCoordinates);
				rgb_rgb = texture2D(u_TextureMaskUnitImage, v_TextureMaskCoordinates).rgb;

				float isFill = step(0.5, float(u_isFill));
				vec4 srcRgbaCal = isFill * vec4(u_Color.r, u_Color.g, u_Color.b, srcRgba.a) + (1.0 - isFill) * srcRgba;
				gl_FragColor = vec4(srcRgbaCal.r, srcRgbaCal.g, srcRgbaCal.b, srcRgba.a * (rgb_rgb.r - 16.0/255.0));
			}
	)";

    compileMixShader();
}

MixRenderer::~MixRenderer()
{
    clearMixSrc();

    vt->DeleteProgram(m_MixshaderProgram);
}

void MixRenderer::compileMixShader()
{
    m_MixshaderProgram = vt->CreateProgram();
    int mInitRet = compileShader(&mVertShader, m_mixVertexShader.c_str(), GL_VERTEX_SHADER);

    if (mInitRet != 0) {
        return;
    }

    mInitRet = compileShader(&mFragmentShader, m_mixFragmentShader.c_str(), GL_FRAGMENT_SHADER);

    if (mInitRet != 0) {
        return;
    }

    vt->AttachShader(m_MixshaderProgram, mVertShader);
    vt->AttachShader(m_MixshaderProgram, mFragmentShader);
    vt->LinkProgram(m_MixshaderProgram);
    GLint status;
    vt->GetProgramiv(m_MixshaderProgram, GL_LINK_STATUS, &status);

    if (status != GL_TRUE) {
        int length = 0;
        GLchar glchar[256] = {0};
        vt->GetProgramInfoLog(m_MixshaderProgram, 256, &length, glchar);
        return;
    }

    vt->DeleteShader(mVertShader);
    vt->DeleteShader(mFragmentShader);

    uTextureSrcUnitLocation = vt->GetUniformLocation(m_MixshaderProgram, "u_TextureSrcUnit");
    uTextureMaskUnitLocationImage = vt->GetUniformLocation(m_MixshaderProgram, "u_TextureMaskUnitImage");
    uMixMatrix = vt->GetUniformLocation(m_MixshaderProgram, "u_MixMatrix");
    uIsFillLocation = vt->GetUniformLocation(m_MixshaderProgram, "u_isFill");
    uColorLocation = vt->GetUniformLocation(m_MixshaderProgram, "u_Color");

    aPositionLocation = vt->GetAttribLocation(m_MixshaderProgram, "a_Position");
    aTextureSrcCoordinatesLocation = vt->GetAttribLocation(m_MixshaderProgram, "a_TextureSrcCoordinates");

    vt->EnableVertexAttribArray(aPositionLocation);
    vt->EnableVertexAttribArray(aTextureSrcCoordinatesLocation);

    vt->UseProgram(m_MixshaderProgram);
    vt->Uniform1i(uTextureMaskUnitLocationImage, 0);
    vt->Uniform1i(uTextureSrcUnitLocation, 1);
    vt->UseProgram(0);

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
            ret[2] = right + p.offsetX;
            ret[3] = bottom + p.offsetY;
            ret[4] = left + p.offsetX;
            ret[5] = top + p.offsetY;
            ret[6] = left + p.offsetX;
            ret[7] = bottom + p.offsetY;
        }
    };

    if (src.srcTexture == 0) return;

    vt->UseProgram(m_MixshaderProgram);

    vt->UniformMatrix4fv(uMixMatrix, 1, GL_FALSE, (GLfloat *) param.matrix);

    GLfloat vertexArray[8] = {0.0};
    createVertexArray(frame.frame, vertexArray, param);
    vt->VertexAttribPointer(aPositionLocation, 2, GL_FLOAT, GL_FALSE, 0, vertexArray);

    GLfloat mixSrcArray[8] = {0.0};
    GLfloat mixMaskArray[8] = {0.0};
    genSrcCoordsArray(mixSrcArray, frame.frame.width(), frame.frame.height(), src.w, src.h, src.fitType);

    createTexCoordsArray(param.videoW, param.videoH, frame.mFrame, mixMaskArray);
    if (frame.mt == 90) rotate90(mixMaskArray);

    GLfloat textureCords[16] = {0.0};
    for (size_t i = 0; i < 4; i++) {
        memcpy(textureCords + i * 4, mixSrcArray + i * 2, sizeof(GLfloat) * 2);
        memcpy(textureCords + i * 4 + 2, mixMaskArray + i * 2, sizeof(GLfloat) * 2);
    }

    vt->VertexAttribPointer(aTextureSrcCoordinatesLocation, 4, GL_FLOAT, GL_FALSE, 0, textureCords);

    vt->ActiveTexture(GL_TEXTURE0);
    vt->BindTexture(GL_TEXTURE_2D, param.videoTexture);

    vt->ActiveTexture(GL_TEXTURE1);
    vt->BindTexture(GL_TEXTURE_2D, src.srcTexture);

    if (src.srcType == MixSrc::TXT) {
        vt->Uniform1i(uIsFillLocation, 1);
        vt->Uniform4f(uColorLocation, src.color[0], src.color[1], src.color[2], src.color[3]);
    } else {
        vt->Uniform1i(uIsFillLocation, 0);
        vt->Uniform4f(uColorLocation, 0.f, 0.f, 0.f, 0.f);
    }

    vt->Enable(GL_BLEND);
    vt->BlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    vt->DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    vt->Disable(GL_BLEND);
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
    } else {
        createTexCoordsArray(fw, fh, Rect(0, 0, fw, fh), array);
    }
}

void MixRenderer::prepareMixResource()
{
    for (auto iter = m_srcMap.begin(); iter != m_srcMap.end(); iter++) {
        MixSrc &mixSrc = iter->second;
        if (m_mixResource.find(mixSrc.srcTag) == m_mixResource.end()) continue;

        auto resourceStr = m_mixResource[mixSrc.srcTag];
        if (mixSrc.srcType == MixSrc::IMG) {
            if (resourceStr.length() != 0) {
                vt->GenTextures(1, &mixSrc.srcTexture);
                int w, h, c;
                auto bytes = stbi_load(resourceStr.c_str(), &w, &h, &c, 4);//rgba
                vt->BindTexture(GL_TEXTURE_2D, mixSrc.srcTexture);
                vt->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                vt->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                vt->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                vt->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                vt->PixelStorei(GL_UNPACK_ALIGNMENT, 1);
                vt->TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);
                stbi_image_free(bytes);
            }
        } else if (mixSrc.srcType == MixSrc::TXT) {
#ifdef WIN32
            Gdiplus::GdiplusStartupInput gdiplusStartupInput;
            ULONG_PTR gdiplusToken;
            Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

            {
                std::unique_ptr<uint8_t> bits(new uint8_t[mixSrc.w * mixSrc.h * 4]);
                auto *bitmap = new Gdiplus::Bitmap(mixSrc.w, mixSrc.h, 4 * mixSrc.w, PixelFormat32bppARGB, bits.get());
                auto *graphics = new Gdiplus::Graphics(bitmap);

                graphics->Clear(Gdiplus::Color::Transparent);

                int fs = 65;
                int style = mixSrc.style == MixSrc::BOLD ? Gdiplus::FontStyleBold : Gdiplus::FontStyleRegular;
                std::wstring text = CicadaUtils::StringToWideString(resourceStr);
                Gdiplus::FontFamily fm(L"Microsoft YaHei");

                Gdiplus::StringFormat strformat(Gdiplus::StringFormat::GenericTypographic());
                strformat.SetAlignment(Gdiplus::StringAlignmentCenter);    //ˮƽ����
                strformat.SetLineAlignment(Gdiplus::StringAlignmentCenter);//��ֱ����
                graphics->SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
                graphics->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
                while (true) {
                    Gdiplus::RectF boundingBox;
                    Gdiplus::Font font(&fm, --fs, style, Gdiplus::UnitPixel);
                    graphics->MeasureString(text.c_str(), (int) text.size() + 1, &font, Gdiplus::PointF(0.0f, 0.0f), &strformat,
                                            &boundingBox);
                    if (boundingBox.Width <= (float) mixSrc.w && boundingBox.Height <= (float) mixSrc.h) break;

                    if (fs <= 1) break;
                }

                auto brush = new Gdiplus::SolidBrush(
                        Gdiplus::Color(mixSrc.color[3] * 255, mixSrc.color[0] * 255, mixSrc.color[1] * 255, mixSrc.color[2] * 255));
                auto pfont = new Gdiplus::Font(&fm, fs, style, Gdiplus::UnitPixel);
                graphics->DrawString(text.c_str(), -1, pfont, Gdiplus::RectF(0, 0, mixSrc.w, mixSrc.h), &strformat, brush);

                vt->GenTextures(1, &mixSrc.srcTexture);
                vt->BindTexture(GL_TEXTURE_2D, mixSrc.srcTexture);
                vt->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                vt->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                vt->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                vt->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                vt->PixelStorei(GL_UNPACK_ALIGNMENT, 1);
                vt->TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mixSrc.w, mixSrc.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, bits.get());

                delete pfont;
                delete bitmap;
                delete graphics;
                delete brush;
            }

            Gdiplus::GdiplusShutdown(gdiplusToken);
#endif
        }
    }
}

void MixRenderer::clearMixSrc()
{
    for (auto iter = m_srcMap.begin(); iter != m_srcMap.end(); iter++) {
        MixSrc &mixSrc = iter->second;
        if (mixSrc.srcTexture != 0) {
            vt->DeleteTextures(1, &mixSrc.srcTexture);
        }
    }
    m_srcMap.clear();
}


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
    vt->Uniform1i(uColorRangeFix, gpu_decoded ? 16 : 0);
    vt->VertexAttribPointer(position, 2, GL_FLOAT, GL_FALSE, 0, mDrawRegion);
    vt->VertexAttribPointer(RGBTexCoord, 2, GL_FLOAT, GL_FALSE, 0, mTextureCoords);
    vt->VertexAttribPointer(alphaTexCoord, 2, GL_FLOAT, GL_FALSE, 0, mAlphaTextureCoords);

    vt->DrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    vt->UseProgram(0);

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
                                    videoFrameTexture,
                                    mRotate};

        mMixRender->renderMix(frameIndex % mVapConfig->totalFrames, param);
    }
}

void GiftEffectRender::createAttributes()
{
	vt->GenVertexArrays(1, &vao);
	vt->GenBuffers(1, &vbo);
	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	vt->BindVertexArray(vao);

	vt->BindBuffer(GL_ARRAY_BUFFER, vbo);
	vt->BufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 8 * 3, NULL, GL_DYNAMIC_DRAW);

	GLint lo = vt->GetAttribLocation(prgm, "position");
	vt->VertexAttribPointer(lo, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	vt->EnableVertexAttribArray(lo);

	GLint ro = vt->GetAttribLocation(prgm, "RGBTexCoord");
	vt->VertexAttribPointer(ro, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
	vt->EnableVertexAttribArray(ro);

	GLint ao = vt->GetAttribLocation(prgm, "alphaTexCoord");
	vt->VertexAttribPointer(ao, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(4 * sizeof(float)));
	vt->EnableVertexAttribArray(ao);

	vt->BindBuffer(GL_ARRAY_BUFFER, 0); 

	vt->BindVertexArray(0); 
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
				gl_Position = u_projection * vec4(position, 0.0, 1.0) * vec4(u_flipMatrix, 1.0, 1.0) * vec4(1, -1, 1, 1);
				RGBTexCoordVarying = RGBTexCoord;
				alphaTexCoordVarying = alphaTexCoord;
			}
			)";


    char *fstr = R"(
    		varying vec2 RGBTexCoordVarying;
    		varying vec2 alphaTexCoordVarying;
    
    		uniform sampler2D SamplerImage;
			uniform int colorRangeFix;
    
    		void main()
    		{
    			vec3 rgb_rgb;
    			vec3 rgb_alpha;

    			rgb_rgb = texture2D(SamplerImage, RGBTexCoordVarying).rgb;
    			rgb_alpha = texture2D(SamplerImage, alphaTexCoordVarying);
    
    			gl_FragColor = vec4(rgb_rgb - colorRangeFix/255.0, rgb_alpha.r - 16.0/255.0);

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

    SamplerImage = vt->GetUniformLocation(prgm, "SamplerImage");
    projection = vt->GetUniformLocation(prgm, "u_projection");
    uflip = vt->GetUniformLocation(prgm, "u_flipMatrix");
    uColorRangeFix = vt->GetUniformLocation(prgm, "colorRangeFix");

    createAttributes();
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
    mMixRender = std::make_unique<MixRenderer>(vt);
    mMixRender->setMixResource(mMaskVapData);
    mMixRender->setVapxInfo(json);
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

	int stride = 6;
	int index = 0;
    if (mRotate == IVideoRender::Rotate::Rotate_None) {
        mAttributeData[stride * index] = (GLfloat)(off_x);
        mAttributeData[stride * index + 1] = (GLfloat)(off_y);

		index++;
        mAttributeData[stride * index] = (GLfloat)(off_x + draw_width);
        mAttributeData[stride * index + 1] = (GLfloat)(off_y);

		index++;
        mAttributeData[stride * index] = (GLfloat)(off_x);
        mAttributeData[stride * index + 1] = (GLfloat)(off_y + draw_height);

		index++;
        mAttributeData[stride * index] = (GLfloat)(off_x + draw_width);
        mAttributeData[stride * index + 1] = (GLfloat)(off_y + draw_height);
    } else if (mRotate == IVideoRender::Rotate::Rotate_90) {
        mAttributeData[stride * index] = (GLfloat)(off_x);
        mAttributeData[stride * index + 1] = (GLfloat)(off_y + draw_height);

		index++;
        mAttributeData[stride * index] = (GLfloat)(off_x);
        mAttributeData[stride * index + 1] = (GLfloat)(off_y);
		
		index++;
        mAttributeData[stride * index] = (GLfloat)(off_x + draw_width);
        mAttributeData[stride * index + 1] = (GLfloat)(off_y + draw_height);
		
		index++;
        mAttributeData[stride * index] = (GLfloat)(off_x + draw_width);
        mAttributeData[stride * index + 1] = (GLfloat)(off_y);
    } else if (mRotate == IVideoRender::Rotate::Rotate_180) {
        mAttributeData[stride * index] = (GLfloat)(off_x + draw_width);
        mAttributeData[stride * index + 1] = (GLfloat)(off_y + draw_height);
		
		index++;
        mAttributeData[stride * index] = (GLfloat)(off_x);
        mAttributeData[stride * index + 1] = (GLfloat)(off_y + draw_height);
		
		index++;
        mAttributeData[stride * index] = (GLfloat)(off_x + draw_width);
        mAttributeData[stride * index + 1] = (GLfloat)(off_y);
		
		index++;
        mAttributeData[stride * index] = (GLfloat)(off_x);
        mAttributeData[stride * index + 1] = (GLfloat)(off_y);
    } else if (mRotate == IVideoRender::Rotate::Rotate_270) {
        mAttributeData[stride * index] = (GLfloat)(off_x + draw_width);
        mAttributeData[stride * index + 1] = (GLfloat)(off_y);
		
		index++;
        mAttributeData[stride * index] = (GLfloat)(off_x + draw_width);
        mAttributeData[stride * index + 1] = (GLfloat)(off_y + draw_height);
		
		index++;
        mAttributeData[stride * index] = (GLfloat)(off_x);
        mAttributeData[stride * index + 1] = (GLfloat)(off_y);
		
		index++;
        mAttributeData[stride * index] = (GLfloat)(off_x);
        mAttributeData[stride * index + 1] = (GLfloat)(off_y + draw_height);
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