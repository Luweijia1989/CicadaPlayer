#define LOG_TAG "OpenGLHelper"
#include "OpenGLHelper.h"
#include <regex>
#include <set>
#include <string.h>//strstr
#include <utils/frame_work_log.h>
#include <vector>

#define BUG_GLES3_ANDROID 1//FIXME: N7 android6 gles3 displays red images, only rgb32 is correct

namespace OpenGLHelper {

    // glGetTexParameteriv is supported by es2 does not support GL_TEXTURE_INTERNAL_FORMAT.
    /// 16bit (R16 e.g.) texture does not support >8bit a BE channel, fallback to 2 channel texture
    int depth16BitTexture()
    {
        static int depth = 16;
        return depth;
    }

    bool useDeprecatedFormats()
    {
        static bool v = false;
        return v;
    }

    std::string removeComments(const std::string &code)
    {
        std::string c(code);
        return std::regex_replace(c, std::regex("(/\\*([^*]|(\\*+[^*/]))*\\*+/)|(//[^\r^\n]*)"), "");
    }

    /// current shader works fine for gles 2~3 only with commonShaderHeader(). It's mainly for desktop core profile

    static std::string commonShaderHeader(ShaderType type)
    {
        // TODO: check useDeprecatedFormats() or useDeprecated()?
        std::string h;
        if (isOpenGLES()) {
            h += "precision mediump int;\n"
                 "precision mediump float;\n";
        } else {
            h += "#define highp\n"
                 "#define mediump\n"
                 "#define lowp\n";
        }
        if (type == ShaderType::Fragment) {
            // >=1.30: texture(sampler2DRect,...). 'texture' is defined in header
            // we can't check GLSLVersion() here because it the actually version used can be defined by "#version"
            h += "#if __VERSION__ < 130\n"
                 "#define texture texture2D\n"
                 "#else\n"
                 "#define texture2D texture\n"
                 "#endif // < 130\n";
        }
        return h;
    }

    std::string compatibleShaderHeader(ShaderType type)
    {
#if BUG_GLES3_ANDROID
        if (isOpenGLES()) return commonShaderHeader(type);
#endif//BUG_GLES3_ANDROID
        std::string h;
        // #version directive must occur in a compilation unit before anything else, except for comments and white spaces. Default is 100 if not set
        h.append("#version ").append(std::to_string(GLSLVersion()));
        if (isOpenGLES() && GLMajorVersion() > 2) h += " es";
        h += "\n";
        h += commonShaderHeader(type);
        if (GLSLVersion() >= 130) {// gl(es) 3
            if (type == ShaderType::Vertex) {
                h += "#define attribute in\n"
                     "#define varying out\n";
            } else if (type == ShaderType::Fragment) {
                h += "#define varying in\n"
                     "#define gl_FragColor out_color\n"//can not starts with 'gl_'
                     "out vec4 gl_FragColor;\n";
            }
        }
        return h;
    }

    int GLSLVersion()
    {
        static int v = -1;
        if (v >= 0) return v;
        if (!glloader_current_gl_ctx()) {
            return 0;
        }
        const char *vs = (const char *) glGetString(GL_SHADING_LANGUAGE_VERSION);
        int major = 0, minor = 0;
        // es: "OpenGL ES GLSL ES 1.00 (ANGLE 2.1.99...)" can use ""%*[ a-zA-Z] %d.%d" in sscanf, desktop: "2.1"
        //QRegExp rx("(\\d+)\\.(\\d+)");
        if (strncmp(vs, "OpenGL ES GLSL ES ", 18) == 0) vs += 18;
        if (sscanf(vs, "%d.%d", &major, &minor) == 2) {
            v = major * 100 + minor;
        } else {
            v = 110;
            if (isOpenGLES()) v = GLMajorVersion() >= 3 ? 300 : 100;
        }
        return v;
    }

    int GLMajorVersion()
    {
        static int v = -1;
        glGetIntegerv(GL_MAJOR_VERSION, &v);
        return v;
    }

    bool isEGL()
    {
        static int is_egl = -1;
        if (is_egl >= 0) return !!is_egl;

#ifdef GLLOADER_EGL
        if (eglGetCurrentDisplay() != EGL_NO_DISPLAY) {//egl can be loaded but glx is used
            is_egl = 1;
            return true;
        }
#endif

        return false;
    }

    bool isOpenGLES()
    {
#ifdef GLLOADER_GL
        return false;
#endif

#ifdef GLLOADER_GLES
        return true;
#endif//  GLLOADER_GLES
    }

    bool hasExtensionEGL(const char *exts[])
    {
        if (!isEGL()) return false;
#ifdef GLLOADER_EGL
        static std::vector<std::string> supported;
        if (supported.size() == 0) {
            EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
            eglInitialize(display, NULL, NULL);
            split(eglQueryString(display, EGL_EXTENSIONS), supported, " ");
        }

        for (int i = 0; exts[i]; ++i) {
            for (auto iter = supported.begin(); iter != supported.end(); iter++) {
                const std::string &s = *iter;
                if (s == exts[i]) return true;
            }
        }
#endif
        return false;
    }

    bool hasExtension(const char *exts[])
    {
        if (!glloader_current_gl_ctx()) {
            AF_LOGD("no gl context for hasExtension");
            return false;
        }

        static std::set<std::string> supported;
        if (supported.size() == 0) {
            int NumberOfExtensions;
            glGetIntegerv(GL_NUM_EXTENSIONS, &NumberOfExtensions);
            for (int i = 0; i < NumberOfExtensions; i++) {
                supported.insert((char *) glGetStringi(GL_EXTENSIONS, i));
            }
        }

        for (int i = 0; exts[i]; ++i) {
            for (auto iter = supported.begin(); iter != supported.end(); iter++) {
                const std::string &it = *iter;
                if (strstr(it.c_str(), exts[i])) return true;
            }
        }
        return false;
    }

    bool isPBOSupported()
    {
        // check pbo support
        static bool support = false;
        static bool pbo_checked = false;
        if (pbo_checked) return support;
        if (!glloader_current_gl_ctx()) return false;
        const char *exts[] = {"GL_ARB_pixel_buffer_object", "GL_EXT_pixel_buffer_object",
                              "GL_NV_pixel_buffer_object",//OpenGL ES
                              NULL};
        support = hasExtension(exts);
        if (GLMajorVersion() > 2) support = true;
        pbo_checked = true;
        return support;
    }

    typedef struct {
        GLint internal_format;
        GLenum format;
        GLenum type;
    } gl_param_t;

    // es formats:  ALPHA, RGB, RGBA, LUMINANCE, LUMINANCE_ALPHA
    // es types:  UNSIGNED_BYTE, UNSIGNED_SHORT_5_6_5, UNSIGNED_SHORT_4_4_4_4, GL_UNSIGNED_SHORT_5_5_5_1 (NO UNSIGNED_SHORT)
    /*!
  c: number of channels(components) in the plane
  b: componet size
    result is gl_param_compat[(c-1)+4*(b-1)]
*/
    static const gl_param_t gl_param_compat[] = {
            // it's legacy
            {GL_LUMINANCE, GL_LUMINANCE, GL_UNSIGNED_BYTE},
            {GL_LUMINANCE_ALPHA, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE},
            {GL_RGB, GL_RGB, GL_UNSIGNED_BYTE},
            {GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE},
            {GL_LUMINANCE_ALPHA, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE},//2 x 8 fallback to ra
            {0, 0, 0},
    };
    static const gl_param_t gl_param_3r16[] = {
            {GL_R8, GL_RED, GL_UNSIGNED_BYTE},      // 1 x 8
            {GL_RG8, GL_RG, GL_UNSIGNED_BYTE},      // 2 x 8
            {GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE},    // 3 x 8
            {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE},  // 4 x 8
            {GL_R16, GL_RED, GL_UNSIGNED_SHORT},    // 1 x 16
            {GL_RG16, GL_RG, GL_UNSIGNED_SHORT},    // 2 x 16
            {GL_RGB16, GL_RGB, GL_UNSIGNED_SHORT},  // 3 x 16
            {GL_RGBA16, GL_RGBA, GL_UNSIGNED_SHORT},// 4 x 16
            {0, 0, 0},
    };
    static const gl_param_t gl_param_desktop_fallback[] = {
            {GL_RED, GL_RED, GL_UNSIGNED_BYTE},  // 1 x 8
            {GL_RG, GL_RG, GL_UNSIGNED_BYTE},    // 2 x 8
            {GL_RGB, GL_RGB, GL_UNSIGNED_BYTE},  // 3 x 8
            {GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE},// 4 x 8
            {GL_RG, GL_RG, GL_UNSIGNED_BYTE},    // 2 x 8
            {0, 0, 0},
    };
    static const gl_param_t gl_param_es3rg8[] = {
            {GL_R8, GL_RED, GL_UNSIGNED_BYTE},    // 1 x 8
            {GL_RG8, GL_RG, GL_UNSIGNED_BYTE},    // 2 x 8
            {GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE},  // 3 x 8
            {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE},// 4 x 8
            {GL_RG8, GL_RG, GL_UNSIGNED_BYTE},    // 2 x 8 fallback to rg
            {0, 0, 0},
    };
    //https://www.khronos.org/registry/gles/extensions/EXT/EXT_texture_rg.txt
    // supported by ANGLE+D3D11
    static const gl_param_t gl_param_es2rg[] = {
            {GL_RED, GL_RED, GL_UNSIGNED_BYTE},  // 1 x 8 //es2: GL_EXT_texture_rg. R8, RG8 are for render buffer
            {GL_RG, GL_RG, GL_UNSIGNED_BYTE},    // 2 x 8
            {GL_RGB, GL_RGB, GL_UNSIGNED_BYTE},  // 3 x 8
            {GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE},// 4 x 8
            {GL_RG, GL_RG, GL_UNSIGNED_BYTE},    // 2 x 8 fallback to rg
            {0, 0, 0},
    };

    bool test_gl_param(const gl_param_t &gp, bool *has_16 = 0)
    {
        if (!glloader_current_gl_ctx()) {
            return false;
        }
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        while (glGetError() != GL_NO_ERROR) {
        }
        glTexImage2D(GL_TEXTURE_2D, 0, gp.internal_format, 64, 64, 0, gp.format, gp.type, NULL);
        if (glGetError() != GL_NO_ERROR) {
            glDeleteTextures(1, &tex);
            return false;
        }
        if (!glGetTexLevelParameteriv) {
            AF_LOGD("Do not support glGetTexLevelParameteriv. test_gl_param returns false");
            glDeleteTextures(1, &tex);
            return false;
        }
        GLint param = 0;
        //GL_PROXY_TEXTURE_2D and no glGenTextures?
#ifndef GL_TEXTURE_INTERNAL_FORMAT//only in desktop
#define GL_TEXTURE_INTERNAL_FORMAT 0x1003
#endif
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &param);
        if (param != gp.internal_format) {
            AF_LOGD("Do not support texture internal format: %#x (result %#x)", gp.internal_format, param);
            glDeleteTextures(1, &tex);
            return false;
        }
        if (!has_16) {
            glDeleteTextures(1, &tex);
            return true;
        }
        *has_16 = false;
        GLenum pname = 0;
#ifndef GL_TEXTURE_RED_SIZE
#define GL_TEXTURE_RED_SIZE 0x805C
#endif
#ifndef GL_TEXTURE_LUMINANCE_SIZE
#define GL_TEXTURE_LUMINANCE_SIZE 0x8060
#endif
        switch (gp.format) {
            case GL_RED:
                pname = GL_TEXTURE_RED_SIZE;
                break;
            case GL_LUMINANCE:
                pname = GL_TEXTURE_LUMINANCE_SIZE;
                break;
        }
        param = 0;
        if (pname) glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, pname, &param);
        if (param) {
            AF_LOGD("16 bit texture depth: %d.\n", (int) param);
            *has_16 = (int) param == 16;
        }
        glDeleteTextures(1, &tex);
        return true;
    }

    bool hasRG()
    {
        static int has_rg = -1;
        if (has_rg >= 0) return !!has_rg;
        AF_LOGD("check gl3 rg: %#X", gl_param_3r16[1].internal_format);
        if (test_gl_param(gl_param_3r16[1])) {
            has_rg = 1;
            return true;
        }
        AF_LOGD("check es3 rg: %#X", gl_param_es3rg8[1].internal_format);
        if (test_gl_param(gl_param_es3rg8[1])) {
            has_rg = 1;
            return true;
        }
        AF_LOGD("check GL_EXT_texture_rg");
        static const char *ext[] = {"GL_EXT_texture_rg", 0};//RED, RG, R8, RG8
        if (hasExtension(ext)) {
            AF_LOGD("has extension GL_EXT_texture_rg");
            has_rg = 1;
            return true;
        }
        AF_LOGD("check gl es>=3 rg");
        if (glloader_current_gl_ctx()) has_rg = isOpenGLES() && GLMajorVersion() > 2;// Mesa GLES3 does not support (from qt)
        return has_rg;
    }

    static int has_16_tex = -1;
    static const gl_param_t *get_gl_param()
    {
        if (!glloader_current_gl_ctx()) {
            AF_LOGD("%s: current context is null", __FUNCTION__);
            return gl_param_compat;
        }
        static gl_param_t *gp = 0;
        if (gp) return gp;
        bool has_16 = false;
        // [4] is always available
        if (test_gl_param(gl_param_3r16[4], &has_16)) {
            if (has_16 && depth16BitTexture() == 16)
                gp = (gl_param_t *) gl_param_3r16;
            else
                gp = (gl_param_t *) gl_param_desktop_fallback;
            has_16_tex = has_16;
            if (!useDeprecatedFormats()) {
                AF_LOGD("using gl_param_%s", gp == gl_param_3r16 ? "3r16" : "desktop_fallback");
                return gp;
            }
        } else if (test_gl_param(gl_param_es3rg8[4], &has_16)) {//3.0 will fail because no glGetTexLevelParameteriv
            gp = (gl_param_t *) gl_param_es3rg8;
            has_16_tex = has_16;
            if (!useDeprecatedFormats()) {
                AF_LOGD("using gl_param_es3rg8");
                return gp;
            }
        } else if (isOpenGLES()) {
            if (GLMajorVersion() > 2)
                gp = (gl_param_t *) gl_param_es3rg8;//for 3.0
            else if (hasRG())
                gp = (gl_param_t *) gl_param_es2rg;
            has_16_tex = has_16;
            if (gp && !useDeprecatedFormats()) {
                AF_LOGD("using gl_param_%s", gp == gl_param_es3rg8 ? "es3rg8" : "es2rg");
                return gp;
            }
        }
        AF_LOGD("fallback to gl_param_compat");
        gp = (gl_param_t *) gl_param_compat;
        has_16_tex = false;
        return gp;
    }

    bool has16BitTexture()
    {
        if (has_16_tex >= 0) return !!has_16_tex;
        if (!glloader_current_gl_ctx()) {
            AF_LOGD("%s: current context is null", __FUNCTION__);
            return false;
        }
        get_gl_param();
        return !!has_16_tex;
    }

    typedef struct {
        VideoFormat::PixelFormat pixfmt;
        unsigned char channels[4];
    } reorder_t;
    // use with gl_param_compat
    static const reorder_t gl_channel_maps[] = {{VideoFormat::Format_ARGB32, {1, 2, 3, 0}},
                                                {VideoFormat::Format_ABGR32, {3, 2, 1, 0}},// R->gl.?(a)->R
                                                {VideoFormat::Format_BGR24, {2, 1, 0, 3}},
                                                {VideoFormat::Format_BGR565, {2, 1, 0, 3}},
                                                {VideoFormat::Format_BGRA32, {2, 1, 0, 3}},
                                                {VideoFormat::Format_BGR32, {2, 1, 0, 3}},
                                                {VideoFormat::Format_BGR48LE, {2, 1, 0, 3}},
                                                {VideoFormat::Format_BGR48BE, {2, 1, 0, 3}},
                                                {VideoFormat::Format_BGR48, {2, 1, 0, 3}},
                                                {VideoFormat::Format_BGR555, {2, 1, 0, 3}},
                                                // TODO: rgb444le/be etc
                                                {VideoFormat::Format_Invalid, {1, 2, 3}}};

    static QMatrix4x4 channelMap(const VideoFormat &fmt)
    {
        if (fmt.isPlanar())//currently only for planar
            return QMatrix4x4();
        switch (fmt.pixelFormat()) {
            case VideoFormat::Format_UYVY:
                return QMatrix4x4(0.0f, 0.5f, 0.0f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            case VideoFormat::Format_YUYV:
                return QMatrix4x4(0.5f, 0.0f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            case VideoFormat::Format_VYUY:
                return QMatrix4x4(0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            case VideoFormat::Format_YVYU:
                return QMatrix4x4(0.5f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            case VideoFormat::Format_VYU:
                return QMatrix4x4(0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            default:
                break;
        }

        const unsigned char *channels = NULL;//{ 0, 1, 2, 3};
        for (int i = 0; gl_channel_maps[i].pixfmt != VideoFormat::Format_Invalid; ++i) {
            if (gl_channel_maps[i].pixfmt == fmt.pixelFormat()) {
                channels = gl_channel_maps[i].channels;
                break;
            }
        }
        QMatrix4x4 m;
        if (!channels) return m;
        m.fill(0);
        for (int i = 0; i < 4; ++i) {
            m(i, channels[i]) = 1;
        }

        return m;
    }

//template<typename T, size_t N> Q_CONSTEXPR size_t array_size(const T (&)[N]) { return N;} //does not support local type if no c++11
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
    bool videoFormatToGL(const VideoFormat &fmt, GLint *internal_format, GLenum *data_format, GLenum *data_type, QMatrix4x4 *mat)
    {
        typedef struct fmt_entry {
            VideoFormat::PixelFormat pixfmt;
            GLint internal_format;
            GLenum format;
            GLenum type;
        } fmt_entry;
        static const fmt_entry pixfmt_to_gles[] = {{VideoFormat::Format_BGRA32, GL_BGRA, GL_BGRA, GL_UNSIGNED_BYTE},//tested for angle
                                                   {VideoFormat::Format_RGB32, GL_BGRA, GL_BGRA, GL_UNSIGNED_BYTE},
                                                   {VideoFormat::Format_Invalid, 0, 0, 0}};
        (void) pixfmt_to_gles;
        static const fmt_entry pixfmt_to_desktop[] = {
                {VideoFormat::Format_BGRA32, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE},//bgra bgra works on win but not macOS
                {VideoFormat::Format_RGB32, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE}, //FIXMEL endian check
        //{VideoFormat::Format_BGRA32,  GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE }, //{2,1,0,3}
        //{VideoFormat::Format_BGR24,   GL_RGB,  GL_BGR,  GL_UNSIGNED_BYTE }, //{0,1,2,3}
#ifdef GL_UNSIGNED_SHORT_5_6_5_REV
                {VideoFormat::Format_BGR565, GL_RGB, GL_RGB, GL_UNSIGNED_SHORT_5_6_5_REV},// es error, use channel map
#endif
#ifdef GL_UNSIGNED_SHORT_1_5_5_5_REV
                {VideoFormat::Format_RGB555, GL_RGBA, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV},
#endif
#ifdef GL_UNSIGNED_SHORT_1_5_5_5_REV
                {VideoFormat::Format_BGR555, GL_RGBA, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV},
#endif
                // TODO: BE formats not implemeted
                {VideoFormat::Format_RGB48, GL_RGB, GL_RGB,
                 GL_UNSIGNED_SHORT},//TODO: they are not work for ANGLE, and rgb16 works on desktop gl, so remove these lines to use rgb16?
                {VideoFormat::Format_RGB48LE, GL_RGB, GL_RGB, GL_UNSIGNED_SHORT},
                {VideoFormat::Format_RGB48BE, GL_RGB, GL_RGB, GL_UNSIGNED_SHORT},
                {VideoFormat::Format_BGR48, GL_RGB, GL_BGR, GL_UNSIGNED_SHORT},//RGB16?
                {VideoFormat::Format_BGR48LE, GL_RGB, GL_BGR, GL_UNSIGNED_SHORT},
                {VideoFormat::Format_BGR48BE, GL_RGB, GL_BGR, GL_UNSIGNED_SHORT},
                {VideoFormat::Format_RGBA64LE, GL_RGBA, GL_RGBA, GL_UNSIGNED_SHORT},
                {VideoFormat::Format_RGBA64BE, GL_RGBA, GL_RGBA, GL_UNSIGNED_SHORT},
                {VideoFormat::Format_BGRA64LE, GL_RGBA, GL_BGRA, GL_UNSIGNED_SHORT},
                {VideoFormat::Format_BGRA64BE, GL_RGBA, GL_BGRA, GL_UNSIGNED_SHORT},
                {VideoFormat::Format_Invalid, 0, 0, 0}};
        (void) pixfmt_to_desktop;
        const fmt_entry *pixfmt_gl_entry = pixfmt_to_desktop;
        if (OpenGLHelper::isOpenGLES()) pixfmt_gl_entry = pixfmt_to_gles;
        // Very special formats, for which OpenGL happens to have direct support
        static const fmt_entry pixfmt_gl_base[] = {
                {VideoFormat::Format_RGBA32, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE},// only tested for macOS, win, angle
                {VideoFormat::Format_RGB24, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE},
                {VideoFormat::Format_RGB565, GL_RGB, GL_RGB, GL_UNSIGNED_SHORT_5_6_5},
                {VideoFormat::Format_BGR32, GL_BGRA, GL_BGRA, GL_UNSIGNED_BYTE},//rgba(tested) or abgr, depending on endian
        };
        const VideoFormat::PixelFormat pixfmt = fmt.pixelFormat();
        // can not use array size because pixfmt_gl_entry is set on runtime
        for (const fmt_entry *e = pixfmt_gl_entry; e->pixfmt != VideoFormat::Format_Invalid; ++e) {
            if (e->pixfmt == pixfmt) {
                *internal_format = e->internal_format;
                *data_format = e->format;
                *data_type = e->type;
                if (mat) *mat = QMatrix4x4();
                return true;
            }
        }
        for (size_t i = 0; i < ARRAY_SIZE(pixfmt_gl_base); ++i) {
            const fmt_entry &e = pixfmt_gl_base[i];
            if (e.pixfmt == pixfmt) {
                *internal_format = e.internal_format;
                *data_format = e.format;
                *data_type = e.type;
                if (mat) *mat = QMatrix4x4();
                return true;
            }
        }
        static const fmt_entry pixfmt_to_gl_swizzele[] = {
                {VideoFormat::Format_VYU, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE},
                {VideoFormat::Format_UYVY, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE},
                {VideoFormat::Format_YUYV, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE},
                {VideoFormat::Format_VYUY, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE},
                {VideoFormat::Format_YVYU, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE},
                {VideoFormat::Format_BGR565, GL_RGB, GL_RGB, GL_UNSIGNED_SHORT_5_6_5},    //swizzle
                {VideoFormat::Format_RGB555, GL_RGBA, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1},//not working
                {VideoFormat::Format_BGR555, GL_RGBA, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1},//not working
        };
        for (size_t i = 0; i < ARRAY_SIZE(pixfmt_to_gl_swizzele); ++i) {
            const fmt_entry &e = pixfmt_to_gl_swizzele[i];
            if (e.pixfmt == pixfmt) {
                *internal_format = e.internal_format;
                *data_format = e.format;
                *data_type = e.type;
                if (mat) *mat = channelMap(fmt);
                return true;
            }
        }
        GLint *i_f = internal_format;
        GLenum *d_f = data_format;
        GLenum *d_t = data_type;
        gl_param_t *gp = (gl_param_t *) get_gl_param();
        const int nb_planes = fmt.planeCount();
        if (gp == gl_param_3r16 && (
                                           //nb_planes == 2 || // nv12 UV plane is 16bit, but we use rg
                                           (OpenGLHelper::depth16BitTexture() == 16 && OpenGLHelper::has16BitTexture() &&
                                            fmt.isBigEndian() && fmt.bitsPerComponent() > 8)// 16bit texture does not support be channel now
                                           )) {
            gp = (gl_param_t *) gl_param_desktop_fallback;
            AF_LOGD("desktop_fallback for %s", nb_planes == 2 ? "bi-plane format" : "16bit big endian channel");
        }
        for (int p = 0; p < nb_planes; ++p) {
            // for packed rgb(swizzle required) and planar formats
            const int c = (fmt.channels(p) - 1) + 4 * ((fmt.bitsPerComponent() + 7) / 8 - 1);
            if (gp[c].format == 0) return false;
            const gl_param_t &f = gp[c];
            *(i_f++) = f.internal_format;
            *(d_f++) = f.format;
            *(d_t++) = f.type;
        }
        if (nb_planes > 2 && data_format[2] == GL_LUMINANCE &&
            fmt.bytesPerPixel(1) == 1) {// QtAV uses the same shader for planar and semi-planar yuv format
            internal_format[2] = data_format[2] = GL_ALPHA;
            if (nb_planes == 4) internal_format[3] = data_format[3] = data_format[2];// vec4(,,,A)
        }
        if (mat) *mat = channelMap(fmt);
        return true;
    }

    // TODO: format + datatype? internal format == format?
    //https://www.khronos.org/registry/gles/extensions/EXT/EXT_texture_format_BGRA8888.txt
    // TODO: special format size, or componentsize(dataType)*components(format)
    int bytesOfGLFormat(GLenum format, GLenum dataType)// TODO: rename bytesOfTexel
    {
        int component_size = 0;
        switch (dataType) {
#ifdef GL_UNSIGNED_INT_8_8_8_8_REV
            case GL_UNSIGNED_INT_8_8_8_8_REV:
                return 4;
#endif
#ifdef GL_UNSIGNED_BYTE_3_3_2
            case GL_UNSIGNED_BYTE_3_3_2:
                return 1;
#endif//GL_UNSIGNED_BYTE_3_3_2
#ifdef GL_UNSIGNED_BYTE_2_3_3_REV
            case GL_UNSIGNED_BYTE_2_3_3_REV:
                return 1;
#endif
            case GL_UNSIGNED_SHORT_5_5_5_1:
#ifdef GL_UNSIGNED_SHORT_1_5_5_5_REV
            case GL_UNSIGNED_SHORT_1_5_5_5_REV:
#endif//GL_UNSIGNED_SHORT_1_5_5_5_REV
#ifdef GL_UNSIGNED_SHORT_5_6_5_REV
            case GL_UNSIGNED_SHORT_5_6_5_REV:
#endif                                   //GL_UNSIGNED_SHORT_5_6_5_REV
            case GL_UNSIGNED_SHORT_5_6_5:// gles
#ifdef GL_UNSIGNED_SHORT_4_4_4_4_REV
            case GL_UNSIGNED_SHORT_4_4_4_4_REV:
#endif//GL_UNSIGNED_SHORT_4_4_4_4_REV
            case GL_UNSIGNED_SHORT_4_4_4_4:
                return 2;
            case GL_UNSIGNED_BYTE:
                component_size = 1;
                break;
                // mpv returns 2
#ifdef GL_UNSIGNED_SHORT_8_8_APPLE
            case GL_UNSIGNED_SHORT_8_8_APPLE:
            case GL_UNSIGNED_SHORT_8_8_REV_APPLE:
                return 2;
#endif
            case GL_UNSIGNED_SHORT:
                component_size = 2;
                break;
        }
        switch (format) {
            case GL_RED:
            case GL_LUMINANCE:
            case GL_ALPHA:
                return component_size;
            case GL_RG:
            case GL_LUMINANCE_ALPHA:
                return 2 * component_size;
#ifdef GL_YCBCR_422_APPLE
            case GL_YCBCR_422_APPLE:
                return 2;
#endif
#ifdef GL_RGB_422_APPLE
            case GL_RGB_422_APPLE:
                return 2;
#endif
#ifdef GL_BGR//ifndef GL_ES
            case GL_BGR:
#endif
            case GL_RGB:
                return 3 * component_size;
#ifdef GL_BGRA//ifndef GL_ES
            case GL_BGRA:
#endif
            case GL_RGBA:
                return 4 * component_size;
            default:
                AF_LOGD("bytesOfGLFormat - Unknown format %u", format);
                return 1;
        }
    }

}//namespace OpenGLHelper
