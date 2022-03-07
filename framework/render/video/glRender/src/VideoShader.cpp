#define LOG_TAG "VideoShader"

#include "VideoShader.h"
#include "OpenGLHelper.h"
#include "VideoShader_p.h"
#include <cassert>
#include <cmath>
#define YUVA_DONE 0

extern "C" {
#include <libavutil/pixfmt.h>
#include <libavutil/frame.h>
}
/*
 * AVColorSpace:
 * libav11 libavutil54.3.0 pixfmt.h, ffmpeg2.1*libavutil52.48.101 frame.h
 * ffmpeg2.5 pixfmt.h. AVFrame.colorspace
 * earlier versions: avcodec.h, avctx.colorspace
 */
ColorSpace colorSpaceFromFFmpeg(AVColorSpace cs)
{
    switch (cs) {
        // from ffmpeg: order of coefficients is actually GBR
        case AVCOL_SPC_RGB:
            return ColorSpace_GBR;
        case AVCOL_SPC_BT709:
            return ColorSpace_BT709;
        case AVCOL_SPC_BT470BG:
            return ColorSpace_BT601;
        case AVCOL_SPC_SMPTE170M:
            return ColorSpace_BT601;
        default:
            return ColorSpace_Unknown;
    }
}

ColorRange colorRangeFromFFmpeg(AVColorRange cr)
{
    switch (cr) {
        case AVCOL_RANGE_MPEG:
            return ColorRange_Limited;
        case AVCOL_RANGE_JPEG:
            return ColorRange_Full;
        default:
            return ColorRange_Unknown;
    }
}

static const char *planar_f_glsl = R"(
// u_TextureN: yuv. use array?
uniform sampler2D u_Texture0;
uniform sampler2D u_Texture1;
uniform sampler2D u_Texture2;
#ifdef HAS_ALPHA
uniform sampler2D u_Texture3;
#endif //HAS_ALPHA
varying vec2 v_TexCoords0;
#ifdef MULTI_COORD
varying vec2 v_TexCoords1;
varying vec2 v_TexCoords2;
#ifdef HAS_ALPHA
varying vec2 v_TexCoords3;
#endif
#else
#define v_TexCoords1 v_TexCoords0
#define v_TexCoords2 v_TexCoords0
#define v_TexCoords3 v_TexCoords0
#endif //MULTI_COORD
uniform float u_opacity;
uniform mat4 u_colorMatrix;
#ifdef CHANNEL16_TO8
uniform vec2 u_to8;
#endif
/***User header code***%userHeader%***/
// matrixCompMult for convolution
/***User sampling function here***%userSample%***/
#ifndef USER_SAMPLER
vec4 sample2d(sampler2D tex, vec2 pos, int plane)
{
    return texture(tex, pos);
}
#endif

// 10, 16bit: http://msdn.microsoft.com/en-us/library/windows/desktop/bb970578%28v=vs.85%29.aspx
void main()
{
    gl_FragColor = clamp(u_colorMatrix
                         * vec4(
#ifdef CHANNEL16_TO8
#ifdef USE_RG
                             dot(sample2d(u_Texture0, v_TexCoords0, 0).rg, u_to8),
                             dot(sample2d(u_Texture1, v_TexCoords1, 1).rg, u_to8),
                             dot(sample2d(u_Texture2, v_TexCoords2, 2).rg, u_to8),
#else
                             dot(sample2d(u_Texture0, v_TexCoords0, 0).ra, u_to8),
                             dot(sample2d(u_Texture1, v_TexCoords1, 1).ra, u_to8),
                             dot(sample2d(u_Texture2, v_TexCoords2, 2).ra, u_to8),
#endif //USE_RG
#else
#ifdef USE_RG
                             sample2d(u_Texture0, v_TexCoords0, 0).r,
                             sample2d(u_Texture1, v_TexCoords1, 1).r,
#ifdef IS_BIPLANE
                             sample2d(u_Texture2, v_TexCoords2, 2).g,
#else
                             sample2d(u_Texture2, v_TexCoords2, 2).r,
#endif //IS_BIPLANE
#else
// use r, g, a to work for both yv12 and nv12. idea from xbmc
                             sample2d(u_Texture0, v_TexCoords0, 0).r,
                             sample2d(u_Texture1, v_TexCoords1, 1).g,
                             sample2d(u_Texture2, v_TexCoords2, 2).a,
#endif //USE_RG
#endif //CHANNEL16_TO8
                             1.0
                            )
                         , 0.0, 1.0) * u_opacity;
#ifdef HAS_ALPHA
    float a =
#ifdef CHANNEL16_TO8
#ifdef USE_RG
    dot(sample2d(u_Texture3, v_TexCoords3, 3).rg, u_to8);
#else
    dot(sample2d(u_Texture3, v_TexCoords3, 3).ra, u_to8);
#endif
#else
#ifdef USE_RG
    sample2d(u_Texture3, v_TexCoords3, 3).r;
#else
    sample2d(u_Texture3, v_TexCoords3, 3).a;
#endif
#endif
    gl_FragColor.rgb = gl_FragColor.rgb*a;
    gl_FragColor.a = a;
#endif //HAS_ALPHA
/***User post processing here***%userPostProcess%***/
}
)";

static const char *packed_f_glsl = R"(
uniform sampler2D u_Texture0;
varying vec2 v_TexCoords0;
uniform mat4 u_colorMatrix;
uniform float u_opacity;
uniform mat4 u_c;
/***User header code***%userHeader%***/
/***User sampling function here***%userSample%***/
#ifndef USER_SAMPLER
vec4 sample2d(sampler2D tex, vec2 pos, int plane)
{
    return texture(tex, pos);
}
#endif

void main() {
    vec4 c = sample2d(u_Texture0, v_TexCoords0, 0);
    c = u_c * c;
#ifndef HAS_ALPHA
    c.a = 1.0; // before color mat transform!
#endif //HAS_ALPHA
#ifdef XYZ_GAMMA
    c.rgb = pow(c.rgb, vec3(2.6));
#endif // XYZ_GAMMA
    c = u_colorMatrix * c;
#ifdef XYZ_GAMMA
    c.rgb = pow(c.rgb, vec3(1.0/2.2));
#endif //XYZ_GAMMA
    gl_FragColor = clamp(c, 0.0, 1.0) * u_opacity;
    /***User post processing here***%userPostProcess%***/
}
)";

static const char *vertex_glsl = R"(
attribute vec4 a_Position;
attribute vec2 a_TexCoords0;
uniform mat4 u_Matrix;
varying vec2 v_TexCoords0;
#ifdef MULTI_COORD
attribute vec2 a_TexCoords1;
attribute vec2 a_TexCoords2;
varying vec2 v_TexCoords1;
varying vec2 v_TexCoords2;
#ifdef HAS_ALPHA
attribute vec2 a_TexCoords3;
varying vec2 v_TexCoords3;
#endif
#endif //MULTI_COORD
/***User header code***%userHeader%***/

void main() {
    gl_Position = u_Matrix * a_Position;
    v_TexCoords0 = a_TexCoords0;
#ifdef MULTI_COORD
    v_TexCoords1 = a_TexCoords1;
    v_TexCoords2 = a_TexCoords2;
#ifdef HAS_ALPHA
    v_TexCoords3 = a_TexCoords3;
#endif
#endif //MULTI_COORD
}
)";

extern std::vector<Uniform> ParseUniforms(const std::string &text, GLuint programId = 0);

VideoShader::VideoShader() : d(new VideoShaderPrivate)
{}

VideoShader::~VideoShader()
{
    delete d;
}

/*
 * use gl api to get active attributes/uniforms
 * use glsl parser to get attributes?
 */
char const *const *VideoShader::attributeNames() const
{
    static const char *names[] = {"a_Position", "a_TexCoords0", 0};
    if (textureTarget() == GL_TEXTURE_2D) return names;
    static const char *names_multicoord[] = {"a_Position", "a_TexCoords0", "a_TexCoords1", "a_TexCoords2", 0};
#if YUVA_DONE
    static const char *names_multicoord_4[] = {"a_Position", "a_TexCoords0", "a_TexCoords1", "a_TexCoords2", "a_TexCoords3", 0};
    if (d->).video_format.planeCount() == 4) return names_multicoord_4;
#endif
    // TODO: names_multicoord_4planes
    return d->video_format.isPlanar() ? names_multicoord : names;
}

const char *VideoShader::vertexShader() const
{
    // because we have to modify the shader, and shader source must be kept, so read the origin
    d->vert = vertex_glsl;
    std::string &vert = d->vert;
    if (vert.length() == 0) {
        AF_LOGW("Empty vertex shader!");
        return 0;
    }
    if (textureTarget() == GL_TEXTURE_RECTANGLE && d->video_format.isPlanar()) {
        vert.insert(0, "#define MULTI_COORD\n");
#if YUVA_DONE
        if (d.video_format.hasAlpha()) vert.insert(0, "#define HAS_ALPHA\n");
#endif
    }
    vert.insert(0, OpenGLHelper::compatibleShaderHeader(OpenGLHelper::Vertex));

    if (userShaderHeader(OpenGLHelper::Vertex)) {
        std::string header("*/");
        header.append(userShaderHeader(OpenGLHelper::Vertex));
        header += "/*";
        replaceAll(vert, "%userHeader%", header);
    }

    return vert.c_str();
}

const char *VideoShader::fragmentShader() const
{
    // because we have to modify the shader, and shader source must be kept, so read the origin
    if (d->video_format.isPlanar()) {
        d->planar_frag = planar_f_glsl;
    } else {
        d->packed_frag = packed_f_glsl;
    }
    std::string &frag = d->video_format.isPlanar() ? d->planar_frag : d->packed_frag;
    if (frag.length() == 0) {
        AF_LOGW("Empty fragment shader!");
        return 0;
    }
    const int nb_planes = d->video_format.planeCount();
    if (nb_planes == 2)//TODO: nv21 must be swapped
        frag.insert(0, "#define IS_BIPLANE\n");
    if (OpenGLHelper::hasRG() && !OpenGLHelper::useDeprecatedFormats()) frag.insert(0, "#define USE_RG\n");
    const bool has_alpha = d->video_format.hasAlpha();
    if (d->video_format.isPlanar()) {
        const int bpc = d->video_format.bitsPerComponent();
        if (bpc > 8) {
            //// has and use 16 bit texture (r16 for example): If channel depth is 16 bit, no range convertion required. Otherwise, must convert to color.r*(2^16-1)/(2^bpc-1)
            if (OpenGLHelper::depth16BitTexture() < 16 || !OpenGLHelper::has16BitTexture() || d->video_format.isBigEndian())
                frag.insert(0, "#define CHANNEL16_TO8\n");
        }
#if YUVA_DONE
        if (has_alpha) frag.prepend("#define HAS_ALPHA\n");
#endif
    } else {
        if (has_alpha) frag.insert(0, "#define HAS_ALPHA\n");
        if (d->video_format.isXYZ()) frag.insert(0, "#define XYZ_GAMMA\n");
    }

    if (d->texture_target == GL_TEXTURE_RECTANGLE) {
        frag.insert(0, "#extension GL_ARB_texture_rectangle : enable\n"
                       "#define sampler2D sampler2DRect\n");
        if (OpenGLHelper::GLSLVersion() < 140)
            frag.insert(0, "#undef texture\n"
                           "#define texture texture2DRect\n");
        frag.insert(0, "#define MULTI_COORD\n");
    }
    frag.insert(0, OpenGLHelper::compatibleShaderHeader(OpenGLHelper::Fragment));

    std::string header("*/");
    if (userShaderHeader(OpenGLHelper::Fragment)) header += userShaderHeader(OpenGLHelper::Fragment);
    header += "\n";
    header += "uniform vec2 u_texelSize[" + std::to_string(nb_planes) + "];\n";
    header += "uniform vec2 u_textureSize[" + std::to_string(nb_planes) + "];\n";
    header += "/*";
    replaceAll(frag, "%userHeader%", header);

    if (userSample()) {
        std::string sample_code("*/\n#define USER_SAMPLER\n");
        sample_code += userSample();
        sample_code += "/*";
        replaceAll(frag, "%userSample%", sample_code);
    }

    if (userPostProcess()) {
        std::string pp_code("*/");
        pp_code += userPostProcess();//why the content is wrong sometimes if no ctor?
        pp_code += "/*";
        replaceAll(frag, "%userPostProcess%", pp_code);
    }
    replaceAll(frag, "%planes%", std::to_string(nb_planes));
    return frag.c_str();
}

void VideoShader::initialize()
{
    if (!textureLocationCount()) return;

    build();

    d->u_Matrix = glGetUniformLocation(d->program, "u_Matrix");
    // fragment shader
    d->u_colorMatrix = glGetUniformLocation(d->program, "u_colorMatrix");
    d->u_to8 = glGetUniformLocation(d->program, "u_to8");
    d->u_opacity = glGetUniformLocation(d->program, "u_opacity");
    d->u_c = glGetUniformLocation(d->program, "u_c");
    d->u_texelSize = glGetUniformLocation(d->program, "u_texelSize");
    d->u_textureSize = glGetUniformLocation(d->program, "u_textureSize");
    d->u_Texture.resize(textureLocationCount());
    AF_LOGD("uniform locations:");
    for (int i = 0; i < d->u_Texture.size(); ++i) {
        const std::string tex_var = string_format("u_Texture%d", i);
        d->u_Texture[i] = glGetUniformLocation(d->program, tex_var.c_str());
        AF_LOGD("%s: %d", tex_var.c_str(), d->u_Texture[i]);
    }
    AF_LOGD("u_Matrix: %d", d->u_Matrix);
    AF_LOGD("u_colorMatrix: %d", d->u_colorMatrix);
    AF_LOGD("u_opacity: %d", d->u_opacity);
    if (d->u_c >= 0) AF_LOGD("u_c: %d", d->u_c);
    if (d->u_to8 >= 0) AF_LOGD("u_to8: %d", d->u_to8);
    if (d->u_texelSize >= 0) AF_LOGD("u_texelSize: %d", d->u_texelSize);
    if (d->u_textureSize >= 0) AF_LOGD("u_textureSize: %d", d->u_textureSize);

    d->user_uniforms[OpenGLHelper::Vertex].clear();
    d->user_uniforms[OpenGLHelper::Fragment].clear();
    if (userShaderHeader(OpenGLHelper::Vertex)) {
        AF_LOGD("user uniform locations in vertex shader:");
        d->user_uniforms[OpenGLHelper::Vertex] = ParseUniforms(userShaderHeader(OpenGLHelper::Vertex), d->program);
    }
    if (userShaderHeader(OpenGLHelper::Fragment)) {
        AF_LOGD("user uniform locations in fragment shader:");
        d->user_uniforms[OpenGLHelper::Fragment] = ParseUniforms(userShaderHeader(OpenGLHelper::Fragment), d->program);
    }
    d->rebuild_program = false;
    d->update_builtin_uniforms = true;
    programReady();// program and uniforms are ready
}

int VideoShader::textureLocationCount() const
{
    // TODO: avoid accessing video_format.
    if (!d->video_format.isPlanar()) return 1;
    return d->video_format.channels();
}

int VideoShader::textureLocation(int channel) const
{
    assert(channel < d->u_Texture.size());
    return d->u_Texture[channel];
}

int VideoShader::matrixLocation() const
{
    return d->u_Matrix;
}

int VideoShader::colorMatrixLocation() const
{
    return d->u_colorMatrix;
}

int VideoShader::opacityLocation() const
{
    return d->u_opacity;
}

int VideoShader::channelMapLocation() const
{
    return d->u_c;
}

int VideoShader::texelSizeLocation() const
{
    return d->u_texelSize;
}

int VideoShader::textureSizeLocation() const
{
    return d->u_textureSize;
}

int VideoShader::uniformLocation(const char *name) const
{
    if (!d->program) return -1;
    return glGetUniformLocation(d->program, name);
}

int VideoShader::textureTarget() const
{
    return d->texture_target;
}

void VideoShader::setTextureTarget(int type)
{
    d->texture_target = type;
}

void VideoShader::setMaterialType(int value)
{
    d->material_type = value;
}

void VideoShader::setVideoFormat(const VideoFormat &format)
{
    d->video_format = format;
}

bool VideoShader::update(VideoMaterial *material)
{
    const int mt = material->type();
    if (mt != d->material_type || d->rebuild_program) {
        // TODO: use shader program cache (per shader), check shader type
        AF_LOGD("Rebuild shader program requested: %d. Material type %d=>%d", d->rebuild_program, d->material_type, mt);
        d->removeAllShaders();//not linked
        // initialize shader, the same as VideoMaterial::createShader
        setVideoFormat(material->currentFormat());
        setTextureTarget(material->textureTarget());
        setMaterialType(material->type());

        initialize();
    }
    //material->unbind();
    const VideoFormat fmt(material->currentFormat());//FIXME: maybe changed in setCurrentFrame(
    //format is out of date because we may use the same shader for different formats
    setVideoFormat(fmt);
    // uniforms begin
    glUseProgram(d->program);
    if (!setUserUniformValues()) {
        if (d->user_uniforms[OpenGLHelper::Vertex].size() != 0) {
            for (int i = 0; i < d->user_uniforms[OpenGLHelper::Vertex].size(); ++i) {
                Uniform &u = d->user_uniforms[OpenGLHelper::Vertex][i];
                setUserUniformValue(u);
                if (u.dirty) u.setGL();
            }
        }
        if (d->user_uniforms[OpenGLHelper::Fragment].size() != 0) {
            for (int i = 0; i < d->user_uniforms[OpenGLHelper::Fragment].size(); ++i) {
                Uniform &u = d->user_uniforms[OpenGLHelper::Fragment][i];
                setUserUniformValue(u);
                if (u.dirty) u.setGL();
            }
        }
    }
    // shader type changed, eq mat changed, or other material properties changed (e.g. texture, 8bit=>10bit)
    if (!d->update_builtin_uniforms && !material->isDirty()) return true;
    d->update_builtin_uniforms = false;
    // all texture ids should be binded when renderering even for packed plane!
    const int nb_planes = fmt.planeCount();//number of texture id
    // TODO: sample2D array
    for (int i = 0; i < nb_planes; ++i) {
        // use glUniform1i to swap planes. swap uv: i => (3-i)%3
        glUniform1i(textureLocation(i), (GLint) i);
    }
    if (nb_planes < textureLocationCount()) {
        for (int i = nb_planes; i < textureLocationCount(); ++i) {
            glUniform1i(textureLocation(i), (GLint)(nb_planes - 1));
        }
    }
    glUniformMatrix4fv(colorMatrixLocation(), 1, GL_FALSE, material->colorMatrix().constData());
    glUniform1f(opacityLocation(), (GLfloat) 1.0);
    if (d->u_to8 >= 0) glUniform2fv(d->u_to8, 1, reinterpret_cast<const GLfloat *>(&material->vectorTo8bit()));
    if (channelMapLocation() >= 0) glUniformMatrix4fv(channelMapLocation(), 1, GL_FALSE, material->channelMap().constData());
    //program()->setUniformValue(matrixLocation(), ); //what about sgnode? state.combindMatrix()?
    if (texelSizeLocation() >= 0)
        glUniform2fv(texelSizeLocation(), nb_planes, reinterpret_cast<const GLfloat *>(material->texelSize().data()));
    if (textureSizeLocation() >= 0)
        glUniform2fv(textureSizeLocation(), nb_planes, reinterpret_cast<const GLfloat *>(material->textureSize().data()));
    // uniform end. attribute begins
    return true;
}

bool VideoShader::build()
{
    if (d->linked) {
        AF_LOGW("Shader program is already linked");
    }
    d->removeAllShaders();
    d->addShaderFromSourceCode(OpenGLHelper::Vertex, vertexShader());
    d->addShaderFromSourceCode(OpenGLHelper::Fragment, fragmentShader());
    int maxVertexAttribs = 0;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
    char const *const *attr = attributeNames();
    for (int i = 0; attr[i]; ++i) {
        if (i >= maxVertexAttribs) {
            AF_LOGE("List of attribute names is either too long or not null-terminated.\n"
                    "Maximum number of attributes on this hardware is %i.\n"
                    "Vertex shader:\n%s\n"
                    "Fragment shader:\n%s\n",
                    maxVertexAttribs, vertexShader(), fragmentShader());
        }
        // why must min location == 0?
        if (*attr[i]) {
            glBindAttribLocation(d->program, i, attr[i]);
            AF_LOGD("bind attribute: %s => %d", attr[i], i);
        }
    }

    if (!d->link()) {
        AF_LOGW("QSGMaterialShader: Shader compilation failed:");
        return false;
    }
    return true;
}

void VideoShader::rebuildLater()
{
    d->rebuild_program = true;
}

VideoMaterial::VideoMaterial() : d(new VideoMaterialPrivate)
{}

VideoMaterial::~VideoMaterial()
{
    delete d;
}

void VideoMaterial::setCurrentFrame(std::unique_ptr<IAFFrame> &frame)
{
    d->update_texure = true;
    // TODO: move to another function before rendering?
    d->width = frame->getInfo().video.width;
    d->height = frame->getInfo().video.height;
    GLenum new_target = GL_TEXTURE_2D;// not d.target. because metadata "target" is not always set
    //QByteArray t = frame.metaData(QStringLiteral("target")).toByteArray().toLower();
    //if (t == QByteArrayLiteral("rect")) new_target = GL_TEXTURE_RECTANGLE;
    //if (new_target != d.target) {
    //    AF_LOGD("texture target: %#x=>%#x", d.target, new_target);
    //    // FIXME: not thread safe (in qml)
    //    d.target = new_target;
    //    d.init_textures_required = true;
    //}
    // TODO: check hw interop change. if change from an interop owns texture to not owns texture, VideoShader must recreate textures because old textures are deleted by previous interop
    const VideoFormat fmt(frame->getInfo().video.format);
    const int bpc_old = d->bpc;
    d->bpc = fmt.bitsPerComponent();
    if (d->bpc > 8 && (d->bpc != bpc_old || d->video_format.isBigEndian() != fmt.isBigEndian())) {
        //FIXME: Assume first plane has 1 channel. So not work with NV21
        const int range = (1 << d->bpc) - 1;
        // FFmpeg supports 9, 10, 12, 14, 16 bits
        // 10p in little endian: yyyyyyyy yy000000 => (L, L, L, A)  //(yyyyyyyy, 000000yy)?
        if (OpenGLHelper::depth16BitTexture() < 16 || !OpenGLHelper::has16BitTexture() || fmt.isBigEndian()) {
            if (fmt.isBigEndian())
                d->vec_to8 = QVector2D(256.0, 1.0) * 255.0 / (float) range;
            else
                d->vec_to8 = QVector2D(1.0, 256.0) * 255.0 / (float) range;
            d->colorTransform.setChannelDepthScale(1.0);
        } else {
            /// 16bit (R16 e.g.) texture does not support >8bit be channels
            /// 10p be: R2 R1(Host) = R1*2^8+R2 = 000000rr rrrrrrrr ->(GL) R=R2*2^8+R1
            /// 10p le: R1 R2(Host) = rrrrrrrr rr000000
            //d.vec_to8 = QVector2D(1.0, 0.0)*65535.0/(float)range;
            d->colorTransform.setChannelDepthScale(65535.0 / (double) range, YUVA_DONE && fmt.hasAlpha());
        }
    } else {
        if (d->bpc <= 8) d->colorTransform.setChannelDepthScale(1.0);
    }
    // http://forum.doom9.org/archive/index.php/t-160211.html
    ColorSpace cs = ColorSpace_Unknown; //colorSpaceFromFFmpeg(ColorSpace_Unknown /*av_frame_get_colorspace(frame)*/);//todo ÕâÀïÐ´ËÀÁË
    if (cs == ColorSpace_Unknown) {
        if (fmt.isRGB()) {
            if (fmt.isPlanar())
                cs = ColorSpace_GBR;
            else
                cs = ColorSpace_RGB;
        } else if (fmt.isXYZ()) {
            cs = ColorSpace_XYZ;
        } else {
            if (d->width >= 1280 || d->height > 576)//values from mpv
                cs = ColorSpace_BT709;
            else
                cs = ColorSpace_BT601;
        }
    }
    d->colorTransform.setInputColorSpace(cs);
    d->colorTransform.setInputColorRange(ColorRange_Limited /*av_frame_get_color_range(frame)*/);
    static const ColorRange kRgbDispRange = true ? ColorRange_Limited : ColorRange_Full;
    d->colorTransform.setOutputColorRange(kRgbDispRange);
    if (fmt != d->video_format) {
        d->video_format = fmt;
        d->init_textures_required = true;
    }
}

VideoFormat VideoMaterial::currentFormat() const
{
    return d->video_format;
}

VideoShader *VideoMaterial::createShader() const
{
    VideoShader *shader = new VideoShader();
    // initialize shader
    shader->setVideoFormat(currentFormat());
    shader->setTextureTarget(textureTarget());
    shader->setMaterialType(type());
    //resize texture locations to avoid access format later
    return shader;
}

std::string VideoMaterial::typeName(int value)
{
    return string_format("gl material 16to8bit: %d, planar: %d, has alpha: %d, 2d texture: %d, 2nd plane rg: %d, xyz: %d", !!(value & 1),
                         !!(value & (1 << 1)), !!(value & (1 << 2)), !!(value & (1 << 3)), !!(value & (1 << 4)), !!(value & (1 << 5)));
}

int VideoMaterial::type() const
{
    const VideoFormat &fmt = d->video_format;
    const bool tex_2d = d->target == GL_TEXTURE_2D;
    // 2d,alpha,planar,8bit
    const int rg_biplane = fmt.planeCount() == 2 && !OpenGLHelper::useDeprecatedFormats() && OpenGLHelper::hasRG();
    const int channel16_to8 =
            d->bpc > 8 && (OpenGLHelper::depth16BitTexture() < 16 || !OpenGLHelper::has16BitTexture() || fmt.isBigEndian());
    return (fmt.isXYZ() << 5) | (rg_biplane << 4) | (tex_2d << 3) | (fmt.hasAlpha() << 2) | (fmt.isPlanar() << 1) | (channel16_to8);
}

bool VideoMaterial::bind()
{
    if (!d->ensureResources()) return false;
    const int nb_planes = d->textures.size();//number of texture id
    if (nb_planes <= 0) return false;
    if (nb_planes > 4)//why?
        return false;
    d->ensureTextures();
    for (int i = 0; i < nb_planes; ++i) {
        const int p = (i + 1) % nb_planes;//0 must active at last?
        d->uploadPlane(p, d->update_texure);
    }
#if 0//move to unbind should be fine
    if (d.update_texure) {
        d.update_texure = false;
        d.frame = VideoFrame(); //FIXME: why need this? we must unmap correctly before frame is reset.
    }
#endif
    return true;
}

// TODO: move bindPlane to d.uploadPlane
void VideoMaterialPrivate::uploadPlane(int p, bool updateTexture)
{
    //GLuint &tex = textures[p];
    //glActiveTexture(GL_TEXTURE0 + p);//0 must active?
    //if (!updateTexture) {
    //    glBindTexture(target, tex);
    //    return;
    //}
    //if (!frame.constBits(0)) {
    //    // try_pbo ? pbo_id : 0. 0= > interop.createHandle
    //    GLuint tex0 = tex;
    //    if (frame.map(GLTextureSurface, &tex, p)) {
    //        if (tex0 != tex) {
    //            if (owns_texture[tex0]) DYGL(glDeleteTextures(1, &tex0));
    //            owns_texture.remove(tex0);
    //            owns_texture[tex] = false;
    //        }
    //        DYGL(glBindTexture(target, tex));// glActiveTexture was called, but maybe bind to 0 in map
    //        return;
    //    }
    //    AF_LOGW("map hw surface error");
    //    return;
    //}
    //// FIXME: why happens on win?
    //if (frame.bytesPerLine(p) <= 0) return;
    //if (try_pbo) {
    //    //AF_LOGD("bind PBO %d", p);
    //    QOpenGLBuffer &pb = pbo[p];
    //    pb.bind();
    //    // glMapBuffer() causes sync issue.
    //    // Call glBufferData() with NULL pointer before glMapBuffer(), the previous data in PBO will be discarded and
    //    // glMapBuffer() returns a new allocated pointer or an unused block immediately even if GPU is still working with the previous data.
    //    // https://www.opengl.org/wiki/Buffer_Object_Streaming#Buffer_re-specification
    //    pb.allocate(pb.size());
    //    GLubyte *ptr = (GLubyte *) pb.map(QOpenGLBuffer::WriteOnly);
    //    if (ptr) {
    //        memcpy(ptr, frame.constBits(p), pb.size());
    //        pb.unmap();
    //    }
    //}
    ////AF_LOGD("bpl[%d]=%d width=%d", p, frame.bytesPerLine(p), frame.planeWidth(p));
    //DYGL(glBindTexture(target, tex));
    ////setupQuality();
    ////DYGL(glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    ////DYGL(glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    //// This is necessary for non-power-of-two textures
    ////glPixelStorei(GL_UNPACK_ALIGNMENT, get_alignment(stride)); 8, 4, 2, 1
    //// glPixelStorei(GL_UNPACK_ROW_LENGTH, stride/glbpp); // for stride%glbpp > 0?
    //DYGL(glTexSubImage2D(target, 0, 0, 0, texture_size[p].width(), texture_size[p].height(), data_format[p], data_type[p],
    //                     try_pbo ? 0 : frame.constBits(p)));
    //if (false) {//texture_size[].width()*gl_bpp != bytesPerLine[]
    //    for (int y = 0; y < plane0Size.height(); ++y)
    //        DYGL(glTexSubImage2D(target, 0, 0, y, texture_size[p].width(), 1, data_format[p], data_type[p],
    //                             try_pbo ? 0 : frame.constBits(p) + y * plane0Size.width()));
    //}
    ////DYGL(glBindTexture(target, 0)); // no bind 0 because glActiveTexture was called
    //if (try_pbo) {
    //    pbo[p].release();
    //}
}

void VideoMaterial::unbind()
{
    //const int nb_planes = d->textures.size();//number of texture id
    //for (int i = 0; i < nb_planes; ++i) {
    //    // unbind planes in the same order as bind. GPU frame's unmap() can be async works, assume the work finished earlier if it started in map() earlier, thus unbind order matter
    //    const int p = (i + 1) % nb_planes;//0 must active at last?
    //    d->frame.unmap(&d.textures[p]);
    //}
    //if (d->update_texure) {
    //    d->update_texure = false;
    //    d->frame = VideoFrame();//FIXME: why need this? we must unmap correctly before frame is reset.
    //}
    //setDirty(false);
}

int VideoMaterial::compare(const VideoMaterial *other) const
{
    for (int i = 0; i < d->textures.size(); ++i) {
        const int diff = d->textures[i] - other->d->textures[i];//TODO
        if (diff) return diff;
    }
    return d->bpc - other->bitsPerComponent();
}

int VideoMaterial::textureTarget() const
{
    return d->target;
}

bool VideoMaterial::isDirty() const
{
    return d->dirty;
}

void VideoMaterial::setDirty(bool value)
{
    d->dirty = value;
}

const QMatrix4x4 &VideoMaterial::colorMatrix() const
{
    return d->colorTransform.matrixRef();
}

const QMatrix4x4 &VideoMaterial::channelMap() const
{
    return d->channel_map;
}

int VideoMaterial::bitsPerComponent() const
{
    return d->bpc;
}

QVector2D VideoMaterial::vectorTo8bit() const
{
    return d->vec_to8;
}

int VideoMaterial::planeCount() const
{
    //todo ===================
    return 2;
    //return d->frame.planeCount();
}

double VideoMaterial::brightness() const
{
    return d->colorTransform.brightness();
}

void VideoMaterial::setBrightness(double value)
{
    d->colorTransform.setBrightness(value);
    d->dirty = true;
}

double VideoMaterial::contrast() const
{
    return d->colorTransform.contrast();
}

void VideoMaterial::setContrast(double value)
{
    d->colorTransform.setContrast(value);
    d->dirty = true;
}

double VideoMaterial::hue() const
{
    return d->colorTransform.hue();
}

void VideoMaterial::setHue(double value)
{
    d->colorTransform.setHue(value);
    d->dirty = true;
}

double VideoMaterial::saturation() const
{
    return d->colorTransform.saturation();
}

void VideoMaterial::setSaturation(double value)
{
    d->colorTransform.setSaturation(value);
    d->dirty = true;
}

double VideoMaterial::validTextureWidth() const
{
    return d->effective_tex_width_ratio;
}

QSize VideoMaterial::frameSize() const
{
    return QSize(d->width, d->height);
}

QSizeF VideoMaterial::texelSize(int plane) const
{
    return QSizeF(1.0 / (double) d->texture_size[plane].width(), 1.0 / (double) d->texture_size[plane].height());
}

std::vector<QVector2D> VideoMaterial::texelSize() const
{
    return d->v_texel_size;
}

QSize VideoMaterial::textureSize(int plane) const
{
    return d->texture_size[plane];
}

std::vector<QVector2D> VideoMaterial::textureSize() const
{
    return d->v_texture_size;
}

QRectF VideoMaterial::normalizedROI(const QRectF &roi) const
{
    return mapToTexture(0, roi, 1);
}

QPointF VideoMaterial::mapToTexture(int plane, const QPointF &p, int normalize) const
{
    if (p.isNull()) return p;
    if (d->texture_size.size() == 0) {//It should not happen if it's called in QtAV
        AF_LOGD("textures not ready");
        return p;
    }
    float x = p.x();
    float y = p.y();
    const double tex0W = d->texture_size[0].width();
    const double s = tex0W / double(d->width);// only apply to unnormalized input roi
    if (normalize < 0) normalize = d->target != GL_TEXTURE_RECTANGLE;
    if (normalize) {
        if (std::abs(x) > 1) {
            x /= (float) tex0W;
            x *= s;
        }
        if (std::abs(y) > 1) y /= (float) d->height;
    } else {
        if (std::abs(x) <= 1)
            x *= (float) tex0W;
        else
            x *= s;
        if (std::abs(y) <= 1) y *= (float) d->height;
    }
    // multiply later because we compare with 1 before it
    x *= d->effective_tex_width_ratio;
    const double pw = d->video_format.normalizedWidth(plane);
    const double ph = d->video_format.normalizedHeight(plane);
    return QPointF(x * pw, y * ph);
}

// mapToTexture
QRectF VideoMaterial::mapToTexture(int plane, const QRectF &roi, int normalize) const
{
    if (d->texture_size.size() == 0) {//It should not happen if it's called in QtAV
        AF_LOGD("textures not ready");
        return QRectF();
    }
    const double tex0W = d->texture_size[0].width();
    const double s = tex0W / double(d->width);// only apply to unnormalized input roi
    const double pw = d->video_format.normalizedWidth(plane);
    const double ph = d->video_format.normalizedHeight(plane);
    if (normalize < 0) normalize = d->target != GL_TEXTURE_RECTANGLE;
    if (!roi.isValid()) {
        if (normalize) return QRectF(0, 0, d->effective_tex_width_ratio, 1);//NOTE: not (0, 0, 1, 1)
        return QRectF(0, 0, tex0W * pw, d->height * ph);
    }
    float x = roi.x();
    float w = roi.width();//TODO: texturewidth
    float y = roi.y();
    float h = roi.height();
    if (normalize) {
        if (std::abs(x) > 1) {
            x /= tex0W;
            x *= s;
        }
        if (std::abs(y) > 1) y /= (float) d->height;
        if (std::abs(w) > 1) {
            w /= tex0W;
            w *= s;
        }
        if (std::abs(h) > 1) h /= (float) d->height;
    } else {//FIXME: what about ==1?
        if (std::abs(x) <= 1)
            x *= tex0W;
        else
            x *= s;
        if (std::abs(y) <= 1) y *= (float) d->height;
        if (std::abs(w) <= 1)
            w *= tex0W;
        else
            w *= s;
        if (std::abs(h) <= 1) h *= (float) d->height;
    }
    // multiply later because we compare with 1 before it
    x *= d->effective_tex_width_ratio;
    w *= d->effective_tex_width_ratio;
    return QRectF(x * pw, y * ph, w * pw, h * ph);
}

bool VideoMaterialPrivate::initPBO(int plane, int size)
{
    //QOpenGLBuffer &pb = pbo[plane];
    //if (!pb.isCreated()) {
    //    AF_LOGD("Creating PBO for plane %d, size: %d...", plane, size);
    //    pb.create();
    //}
    //if (!pb.bind()) {
    //    AF_LOGW("Failed to bind PBO for plane %d!!!!!!", plane);
    //    try_pbo = false;
    //    return false;
    //}
    ////pb.setUsagePattern(QOpenGLBuffer::DynamicCopy);
    //AF_LOGD("Allocate PBO size %d", size);
    //pb.allocate(size);
    //pb.release();//bind to 0
    return true;
}

bool VideoMaterialPrivate::initTexture(GLuint tex, GLint internal_format, GLenum format, GLenum dataType, int width, int height)
{
    glBindTexture(target, tex);
    setupQuality();
    // This is necessary for non-power-of-two textures
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(target, 0, internal_format, width, height, 0 /*border, ES not support*/, format, dataType, NULL);
    glBindTexture(target, 0);
    return true;
}

VideoMaterialPrivate::~VideoMaterialPrivate()
{
    if (textures.size() != 0) {
        for (int i = 0; i < textures.size(); ++i) {
            GLuint &tex = textures[i];
            if (owns_texture[tex]) glDeleteTextures(1, &tex);
        }
    }
    owns_texture.clear();
    textures.clear();
    pbo.clear();
}

bool VideoMaterialPrivate::updateTextureParameters(const VideoFormat &fmt)
{
    // isSupported(pixfmt)
    if (!fmt.isValid()) return false;
    //http://www.berkelium.com/OpenGL/GDC99/internalformat.html
    //NV12: UV is 1 plane. 16 bits as a unit. GL_LUMINANCE4, 8, 16, ... 32?
    //GL_LUMINANCE, GL_LUMINANCE_ALPHA are deprecated in GL3, removed in GL3.1
    //replaced by GL_RED, GL_RG, GL_RGB, GL_RGBA? for 1, 2, 3, 4 channel image
    //http://www.gamedev.net/topic/634850-do-luminance-textures-still-exist-to-opengl/
    //https://github.com/kivy/kivy/issues/1738: GL_LUMINANCE does work on a Galaxy Tab 2. LUMINANCE_ALPHA very slow on Linux
    //ALPHA: vec4(1,1,1,A), LUMINANCE: (L,L,L,1), LUMINANCE_ALPHA: (L,L,L,A)
    const int nb_planes = fmt.planeCount();
    internal_format.resize(nb_planes);
    data_format.resize(nb_planes);
    data_type.resize(nb_planes);
    if (!OpenGLHelper::videoFormatToGL(fmt, (GLint *) internal_format.data(), (GLenum *) data_format.data(), (GLenum *) data_type.data(),
                                       &channel_map)) {
        return false;
    }

    for (int i = 0; i < nb_planes; ++i) {
        const int bpp_gl = OpenGLHelper::bytesOfGLFormat(data_format[i], data_type[i]);
        const int pad = std::ceil((double) (texture_size[i].width() - effective_tex_width[i]) / (double) bpp_gl);
        texture_size[i].setWidth(std::ceil((double) texture_size[i].width() / (double) bpp_gl));
        effective_tex_width[i] /= bpp_gl;//fmt.bytesPerPixel(i);
        v_texture_size[i] = QVector2D(texture_size[i].width(), texture_size[i].height());
        //effective_tex_width_ratio =
        AF_LOGD("texture width: %d - %d = pad: %d. bpp(gl): %d", texture_size[i].width(), effective_tex_width[i], pad, bpp_gl);
        if (target == GL_TEXTURE_RECTANGLE)
            v_texel_size[i] = QVector2D(1.0, 1.0);
        else
            v_texel_size[i] = QVector2D(1.0 / (float) texture_size[i].width(), 1.0 / (float) texture_size[i].height());
    }
    /*
     * there are 2 fragment shaders: rgb and yuv.
     * only 1 texture for packed rgb. planar rgb likes yuv
     * To support both planar and packed yuv, and mixed yuv(NV12), we give a texture sample
     * for each channel. For packed, each (channel) texture sample is the same. For planar,
     * packed channels has the same texture sample.
     * But the number of actural textures we upload is plane count.
     * Which means the number of texture id equals to plane count
     */
    // always delete old textures otherwise old textures are not initialized with correct parameters
    if (textures.size() > nb_planes) {//TODO: why check this?
        const int nb_delete = textures.size() - nb_planes;
        AF_LOGD("try to delete %d textures", nb_delete);
        if (textures.size() != 0) {
            for (int i = 0; i < nb_delete; ++i) {
                GLuint &t = textures[nb_planes + i];
                AF_LOGD("try to delete texture[%d]: %u. can delete: %d", nb_planes + i, t, owns_texture[t]);
                if (owns_texture[t]) glDeleteTextures(1, &t);
            }
            //DYGL(glDeleteTextures(nb_delete, textures.data() + nb_planes));
        }
        owns_texture.clear();
    }
    textures.resize(nb_planes);
    init_textures_required = true;
    return true;
}

bool VideoMaterialPrivate::ensureResources()
{
    //if (!update_texure)//video frame is already uploaded and displayed
    //    return true;
    //const VideoFormat &fmt = video_format;
    //if (!fmt.isValid()) return false;
    //// update textures if format, texture target, valid texture width(normalized), plane 0 size or plane 1 line size changed
    //bool update_textures = init_textures_required;
    //const int nb_planes = fmt.planeCount();
    //// effective size may change even if plane size not changed
    //bool effective_tex_width_ratio_changed = true;
    //for (int i = 0; i < nb_planes; ++i) {
    //    if ((double) frame.effectiveBytesPerLine(i) / (double) frame.bytesPerLine(i) == effective_tex_width_ratio) {
    //        effective_tex_width_ratio_changed = false;
    //        break;
    //    }
    //}
    //const int linsize0 = frame.bytesPerLine(0);
    //if (update_textures || effective_tex_width_ratio_changed || linsize0 != plane0Size.width() || frame.height() != plane0Size.height() ||
    //    (plane1_linesize > 0 && frame.bytesPerLine(1) != plane1_linesize)) {// no need to check height if plane 0 sizes are equal?
    //    update_textures = true;
    //    dirty = true;
    //    v_texel_size.resize(nb_planes);
    //    v_texture_size.resize(nb_planes);
    //    texture_size.resize(nb_planes);
    //    effective_tex_width.resize(nb_planes);
    //    effective_tex_width_ratio = 1.0;
    //    for (int i = 0; i < nb_planes; ++i) {
    //        AF_LOGD("plane linesize %d: padded = %d, effective = %d. theoretical plane size: %dx%d", i, frame.bytesPerLine(i),
    //                frame.effectiveBytesPerLine(i), frame.planeWidth(i), frame.planeHeight(i));
    //        // we have to consider size of opengl format. set bytesPerLine here and change to width later
    //        texture_size[i] = QSize(frame.bytesPerLine(i), frame.planeHeight(i));
    //        effective_tex_width[i] = frame.effectiveBytesPerLine(i);//store bytes here, modify as width later
    //        // usually they are the same. If not, the difference is small. min value can avoid rendering the invalid data.
    //        effective_tex_width_ratio =
    //                qMin(effective_tex_width_ratio, (double) frame.effectiveBytesPerLine(i) / (double) frame.bytesPerLine(i));
    //    }
    //    plane1_linesize = 0;
    //    if (nb_planes > 1) {
    //        // height? how about odd?
    //        plane1_linesize = frame.bytesPerLine(1);
    //    }
    //    /*
    //      let wr[i] = valid_bpl[i]/bpl[i], frame from avfilter maybe wr[1] < wr[0]
    //      e.g. original frame plane 0: 720/768; plane 1,2: 360/384,
    //      filtered frame plane 0: 720/736, ... (16 aligned?)
    //     */
    //    AF_LOGD("effective_tex_width_ratio=%f", effective_tex_width_ratio);
    //    plane0Size.setWidth(linsize0);
    //    plane0Size.setHeight(frame.height());
    //}
    //if (update_textures) {
    //    updateTextureParameters(fmt);
    //    // check pbo support
    //    try_pbo = try_pbo && OpenGLHelper::isPBOSupported();
    //    // check PBO support with bind() is fine, no need to check extensions
    //    if (try_pbo) {
    //        pbo.resize(nb_planes);
    //        for (int i = 0; i < nb_planes; ++i) {
    //            AF_LOGD("Init PBO for plane %d", i);
    //            pbo[i] = QOpenGLBuffer(QOpenGLBuffer::PixelUnpackBuffer);//QOpenGLBuffer is shared, must initialize 1 by 1 but not use fill
    //            if (!initPBO(i, frame.bytesPerLine(i) * frame.planeHeight(i))) {
    //                AF_LOGW("Failed to init PBO for plane %d", i);
    //                break;
    //            }
    //        }
    //    }
    //}
    return true;
}

bool VideoMaterialPrivate::ensureTextures()
{
    //if (!init_textures_required) return true;
    //// create in bindPlane loop will cause wrong texture binding
    //const int nb_planes = video_format.planeCount();
    //for (int p = 0; p < nb_planes; ++p) {
    //    GLuint &tex = textures[p];
    //    if (tex) {// can be 0 if resized to a larger size
    //        AF_LOGD("try to delete texture for plane %d (id=%u). can delete: %d", p, tex, owns_texture[tex]);
    //        if (owns_texture[tex]) DYGL(glDeleteTextures(1, &tex));
    //        owns_texture.remove(tex);
    //        tex = 0;
    //    }
    //    if (!tex) {
    //        AF_LOGD("creating texture for plane %d", p);
    //        GLuint *handle = (GLuint *) frame.createInteropHandle(&tex, GLTextureSurface, p);// take the ownership
    //        if (handle) {
    //            tex = *handle;
    //            owns_texture[tex] = true;
    //        } else {
    //            DYGL(glGenTextures(1, &tex));
    //            owns_texture[tex] = true;
    //            initTexture(tex, internal_format[p], data_format[p], data_type[p], texture_size[p].width(), texture_size[p].height());
    //        }
    //        AF_LOGD("texture for plane %d is created (id=%u)", p, tex);
    //    }
    //}
    //init_textures_required = false;
    return true;
}

void VideoMaterialPrivate::setupQuality()
{
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

VideoShader *ShaderManager::prepareMaterial(VideoMaterial *material, int materialType)
{
    const int type = materialType != -1 ? materialType : material->type();
    auto iter = shader_cache.find(type);
    if (iter != shader_cache.end()) return iter->second;

    auto shader = material->createShader();
    shader->initialize();
    shader_cache[type] = shader;
    return shader;
}

ShaderManager::~ShaderManager()
{
    for (auto iter = shader_cache.begin(); iter != shader_cache.end(); iter++) {
        delete iter->second;
    }

    shader_cache.clear();
}
