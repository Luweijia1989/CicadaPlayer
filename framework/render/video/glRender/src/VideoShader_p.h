#pragma once

#include "ColorTransform.h"
#include "OpenGLTypes.h"
#include "base/media/VideoFormat.h"
#include "platform/platform_gl.h"
#include "qvector2d.h"
#include "qvector4d.h"
#include <map>
#include <vector>

class VideoShader;
class VideoShaderPrivate {
public:
    VideoShaderPrivate()
        : rebuild_program(false), update_builtin_uniforms(true), program(0), linked(false), vertex_shader(0), fragment_shader(0),
          u_Matrix(-1), u_colorMatrix(-1), u_to8(-1), u_opacity(-1), u_c(-1), material_type(0), texture_target(GL_TEXTURE_2D)
    {
        program = glCreateProgram();
    }
    virtual ~VideoShaderPrivate()
    {
        if (glloader_current_gl_ctx()) {
            // FIXME: may be not called from renderering thread. so we still have to detach shaders
            removeAllShaders();
        }
        glDeleteProgram(program);
        program = 0;
    }
    void removeAllShaders()
    {
        if (vertex_shader > 0) {
            glDetachShader(program, vertex_shader);
            glDeleteShader(vertex_shader);
            vertex_shader = -1;
        }

        if (fragment_shader > 0) {
            glDetachShader(program, fragment_shader);
            glDeleteShader(fragment_shader);
            fragment_shader = -1;
        }

        linked = false;
    }

    void addShaderFromSourceCode(OpenGLHelper::ShaderType type, const char *source)
    {
        GLuint shader = glCreateShader(type == OpenGLHelper::Vertex ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
        glShaderSource(shader, 1, &source, NULL);
        glCompileShader(shader);

        int success;
        char infoLog[512];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            AF_LOGD("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n %s", infoLog);

            glDeleteShader(shader);
            return;
        }

		glAttachShader(program, shader);
        if (type == OpenGLHelper::Vertex)
            vertex_shader = shader;
        else
            fragment_shader = shader;
    }

    bool link()
    {
        glLinkProgram(program);
        int value = -1;
        glGetProgramiv(program, GL_LINK_STATUS, &value);
        linked = (value != 0);

        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &value);
        if (value > 1) {
            char *logbuf = new char[value];
            GLint len;
            glGetProgramInfoLog(program, value, &len, logbuf);
            if (!linked) AF_LOGD("QOpenGLShader::link: %ls", logbuf);

            delete[] logbuf;
        }
        return linked;
    }

    bool rebuild_program;
    bool update_builtin_uniforms;//builtin uniforms are static, set the values once is enough if no change
    GLuint program;
    bool linked;
    GLuint vertex_shader;
    GLuint fragment_shader;
    int u_Matrix;
    int u_colorMatrix;
    int u_to8;
    int u_opacity;
    int u_c;
    int u_texelSize;
    int u_textureSize;
    int material_type;
    std::vector<int> u_Texture;
    GLenum texture_target;
    VideoFormat video_format;
    mutable std::string planar_frag, packed_frag;
    mutable std::string vert;
    std::vector<Uniform> user_uniforms[2];
};

class VideoMaterialPrivate {
public:
    VideoMaterialPrivate()
        : update_texure(true), init_textures_required(true), bpc(0), width(0), height(0), video_format(VideoFormat::Format_Invalid),
          plane1_linesize(0), effective_tex_width_ratio(1.0), target(GL_TEXTURE_2D), dirty(true), try_pbo(true)
    {
        v_texel_size.reserve(4);
        textures.reserve(4);
        texture_size.reserve(4);
        effective_tex_width.reserve(4);
        internal_format.reserve(4);
        data_format.reserve(4);
        data_type.reserve(4);
        pbo.reserve(4);
        colorTransform.setOutputColorSpace(ColorSpace_RGB);
    }
    ~VideoMaterialPrivate();
    bool initPBO(int plane, int size);
    bool initTexture(GLuint tex, GLint internal_format, GLenum format, GLenum dataType, int width, int height);
    bool updateTextureParameters(const VideoFormat &fmt);
    void uploadPlane(int p, bool updateTexture = true);
    bool ensureResources();
    bool ensureTextures();
    void setupQuality();

    bool update_texure;         // reduce upload/map times. true: new frame not bound. false: current frame is bound
    bool init_textures_required;// e.g. target changed
    int bpc;
    int width, height;//avoid accessing frame(need lock)
    /*
     *  old format. used to check whether we have to update textures. set to current frame's format after textures are updated.
     * TODO: only VideoMaterial.type() is enough to check and update shader. so remove it
     */
    VideoFormat video_format;
    QSize plane0Size;
    // width is in bytes. different alignments may result in different plane 1 linesize even if plane 0 are the same
    int plane1_linesize;

    // textures.d in updateTextureParameters() changed. happens in qml. why?
    unsigned char workaround_vector_crash_on_linux[8];//TODO: remove
    std::vector<GLuint> textures;                     //texture ids. size is plane count
    std::map<GLuint, bool> owns_texture;
    std::vector<QSize> texture_size;

    std::vector<int> effective_tex_width;//without additional width for alignment
    double effective_tex_width_ratio;
    GLenum target;
    std::vector<GLint> internal_format;
    std::vector<GLenum> data_format;
    std::vector<GLenum> data_type;

    bool dirty;
    ColorTransform colorTransform;
    bool try_pbo;
    std::vector<GLuint> pbo;
    QVector2D vec_to8;//TODO: vec3 to support both RG and LA (.rga, vec_to8)
    QMatrix4x4 channel_map;
    std::vector<QVector2D> v_texel_size;
    std::vector<QVector2D> v_texture_size;
};
