#pragma once
#include "../IVideoRender.h"
#include "gl_common.h"
extern "C" {
#include <libavutil/frame.h>
#include <vlc_es.h>
}

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#define SPHERE_RADIUS 1.f

/* FIXME: GL_ASSERT_NOERROR disabled for now because:
 * Proper GL error handling need to be implemented
 * glClear(GL_COLOR_BUFFER_BIT) throws a GL_INVALID_FRAMEBUFFER_OPERATION on macOS
 * assert fails on vout_display_opengl_Delete on iOS
 */
#if 0
#define HAVE_GL_ASSERT_NOERROR
#endif

#ifdef HAVE_GL_ASSERT_NOERROR
#define GL_ASSERT_NOERROR()                                                                                                                \
    do {                                                                                                                                   \
        GLenum glError = vgl->vt.GetError();                                                                                               \
        switch (glError) {                                                                                                                 \
            case GL_NO_ERROR:                                                                                                              \
                break;                                                                                                                     \
            case GL_INVALID_ENUM:                                                                                                          \
                assert(!"GL_INVALID_ENUM");                                                                                                \
            case GL_INVALID_VALUE:                                                                                                         \
                assert(!"GL_INVALID_VALUE");                                                                                               \
            case GL_INVALID_OPERATION:                                                                                                     \
                assert(!"GL_INVALID_OPERATION");                                                                                           \
            case GL_INVALID_FRAMEBUFFER_OPERATION:                                                                                         \
                assert(!"GL_INVALID_FRAMEBUFFER_OPERATION");                                                                               \
            case GL_OUT_OF_MEMORY:                                                                                                         \
                assert(!"GL_OUT_OF_MEMORY");                                                                                               \
            default:                                                                                                                       \
                assert(!"GL_UNKNOWN_ERROR");                                                                                               \
        }                                                                                                                                  \
    } while (0)
#else
#define GL_ASSERT_NOERROR()
#endif

#define PICTURE_PLANE_MAX 5

struct GLProgram {
    GLuint id;

    struct {
        GLfloat OrientationMatrix[16];
        GLfloat ProjectionMatrix[16];
        GLfloat ZRotMatrix[16];
        GLfloat YRotMatrix[16];
        GLfloat XRotMatrix[16];
        GLfloat ZoomMatrix[16];
        GLfloat AspectRatio[2];
    } var;

    struct { /* UniformLocation */
        GLint OrientationMatrix;
        GLint ProjectionMatrix;
        GLint ZRotMatrix;
        GLint YRotMatrix;
        GLint XRotMatrix;
        GLint ZoomMatrix;
        GLint AspectRatio;
    } uloc;
    struct { /* AttribLocation */
        GLint MultiTexCoord[3];
        GLint VertexPosition;
    } aloc;
};

class GLBase;
class OpenGLTextureConverter;
class GiftEffectRender;
class GLRender {
public:
    GLRender(video_format_t *format);
    ~GLRender();

    bool videoFormatChanged(video_format_t *format);
    bool initGL();
    void clearScreen(uint32_t color);
    int displayGLFrame(const std::string &vapInfo, IVideoRender::MaskMode mode, const std::string &data, AVFrame *frame, int frameIndex,
                       IVideoRender::Rotate rotate, IVideoRender::Scale scale, IVideoRender::Flip flip, const video_format_t *source,
                       int viewWidth, int viewHeight);
	void setExternalFboId(unsigned int id) { external_fbo_id = id; }

private:
    void resetGLState();
    void ResizeFormatToGLMaxTexSize(unsigned int max_tex_size);
    void getViewpointMatrixes();

    int initShaderProgram();
    void destroyShaderProgram();
    int linkShaderProgram();

    GLuint BuildVertexShader(unsigned plane_count);
    int GenTextures();

    void GetTextureCropParamsForStereo(unsigned i_nbTextures, const float *stereoCoefs, const float *stereoOffsets, float *left, float *top,
                                       float *right, float *bottom);
    void DrawWithShaders(bool flipV);
    int SetupCoords(const float *left, const float *top, const float *right, const float *bottom);
    int BuildRectangle(unsigned nbPlanes, GLfloat **vertexCoord, GLfloat **textureCoord, unsigned *nbVertices, GLushort **indices,
                       unsigned *nbIndices, const float *left, const float *top, const float *right, const float *bottom);

    int prepareGLFrame(AVFrame *frame);
    int displayGLFrameInternal(const video_format_t *source, int viewWidth, int viewHeight, bool flipV);
    void updateOutParam(IVideoRender::Rotate rotate, IVideoRender::Scale scale, IVideoRender::Flip flip, int viewWidth, int viewHeight);

    GLBase *glBase = nullptr;
    bool gpu_decoded = false;
	bool gl_inited = false;
    OpenGLTextureConverter *textureConvter = nullptr;
    GiftEffectRender *giftEffectRender = nullptr;

    opengl_vtable_t vt = {0};
    video_format_t fmt = {0};
    const char *extensions;

    GLsizei tex_width[PICTURE_PLANE_MAX] = {0};
    GLsizei tex_height[PICTURE_PLANE_MAX] = {0};

    GLuint texture[PICTURE_PLANE_MAX] = {0};

	GLuint external_fbo_id = -1;

    /* One YUV program and one RGBA program (for subpics) */
    GLProgram prgm = {0}; /* Main program */

    unsigned nb_indices = 0;
    GLuint vertex_buffer_object = 0;
    GLuint index_buffer_object = 0;
    GLuint texture_buffer_object[PICTURE_PLANE_MAX] = {0};

    struct {
        unsigned int i_x_offset = 0;
        unsigned int i_y_offset = 0;
        unsigned int i_visible_width = 0;
        unsigned int i_visible_height = 0;
    } last_source;

    /* Non-power-of-2 texture size support */
    bool supports_npot = false;

    /* View point */
    float f_teta;
    float f_phi;
    float f_roll;
    float f_fovx; /* f_fovx and f_fovy are linked but we keep both */
    float f_fovy; /* to avoid recalculating them when needed.      */
    float f_z;    /* Position of the camera on the shpere radius vector */
    float f_z_min;
    float f_sar;

    IVideoRender::Rotate last_rotate = IVideoRender::Rotate_None;
    IVideoRender::Scale last_scale = IVideoRender::Scale_Fill;
    IVideoRender::Flip last_flip = IVideoRender::Flip_None;
    int last_view_width = 0;
    int last_view_height = 0;
};