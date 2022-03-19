#pragma once

#include "gl_base.h"
#include <vlc_common.h>
#include <vlc_es.h>
#include <string>

extern "C" {
#include <libavutil/frame.h>
}

struct pl_context;
struct pl_shader;
struct pl_shader_res;

#define PICTURE_PLANE_MAX 5

class OpenGLTextureConverter {
public:
    OpenGLTextureConverter();
    virtual ~OpenGLTextureConverter()
    {}

    virtual int init() = 0;

    /* Function pointer to the shader init command, set by the caller, see
     * opengl_fragment_shader_init() documentation. */
    GLuint pf_fragment_shader_init(GLenum, vlc_fourcc_t, video_color_space_t);

    /*
     * Callback to fetch locations of uniform or attributes variables
     *
     * This function pointer cannot be NULL. This callback is called one time
     * after init.
     *
     * \param fc OpenGL tex converter
     * \param program linked program that will be used by this tex converter
     * \return VLC_SUCCESS or a VLC error
     */
    virtual int pf_fetch_locations(GLuint program);

    /*
     * Callback to prepare the fragment shader
     *
     * This function pointer cannot be NULL. This callback can be used to
     * specify values of uniform variables.
     *
     * \param fc OpenGL tex converter
     * \param tex_width array of tex width (one per plane)
     * \param tex_height array of tex height (one per plane)
     * \param alpha alpha value, used only for RGBA fragment shader
     */
    virtual void pf_prepare_shader(const GLsizei *tex_width, const GLsizei *tex_height, float alpha);

    /*
     * Callback to allocate data for bound textures
     *
     * This function pointer can be NULL. Software converters should call
     * glTexImage2D() to allocate textures data (it will be deallocated by the
     * caller when calling glDeleteTextures()). Won't be called if
     * handle_texs_gen is true.
     *
     * \param fc OpenGL tex converter
     * \param textures array of textures to bind (one per plane)
     * \param tex_width array of tex width (one per plane)
     * \param tex_height array of tex height (one per plane)
     * \return VLC_SUCCESS or a VLC error
     */
    virtual int pf_allocate_textures(GLuint *textures, const GLsizei *tex_width, const GLsizei *tex_height) = 0;

    /*
     * Callback to update a picture
     *
     * This function pointer cannot be NULL. The implementation should upload
     * every planes of the picture.
     *
     * \param fc OpenGL tex converter
     * \param textures array of textures to bind (one per plane)
     * \param tex_width array of tex width (one per plane)
     * \param tex_height array of tex height (one per plane)
     * \param pic picture to update
     * \param plane_offset offsets of each picture planes to read data from
     * (one per plane, can be NULL)
     * \return VLC_SUCCESS or a VLC error
     */
    virtual int pf_update(GLuint *textures, const GLsizei *tex_width, const GLsizei *tex_height, AVFrame *pic) = 0;


    bool isAvailable()
    {
        return useable;
    }

private:
    bool base_shader = true;

    int GetTexFormatSize(int target, int tex_format, int tex_internal, int tex_type);
    GLuint xyz12_shader_init();
    int tc_yuv_base_init(GLenum tex_target, vlc_fourcc_t chroma, const vlc_chroma_description_t *desc, video_color_space_t yuv_space,
                         bool *swap_uv, const char *swizzle_per_tex[]);
    int tc_rgb_base_init(GLenum tex_target, vlc_fourcc_t chroma);

    int tc_base_fetch_locations(GLuint program);
    void tc_base_prepare_shader(const GLsizei *tex_width, const GLsizei *tex_height, float alpha);
    int tc_xyz12_fetch_locations(GLuint program);
    void tc_xyz12_prepare_shader(const GLsizei *tex_width, const GLsizei *tex_height, float alpha);

public:
    GLBase *gl = nullptr;

    /* libplacebo context, created by the caller (optional) */
    struct pl_context *pl_ctx = nullptr;

    /* Function pointers to OpenGL functions, set by the caller */
    const opengl_vtable_t *vt = nullptr;

    /* True to dump shaders, set by the caller */
    bool b_dump_shaders = true;

    /* Available gl extensions (from GL_EXTENSIONS) */
    std::string glexts;

    /* True if the current API is OpenGL ES, set by the caller */
    bool is_gles = false;
    /* GLSL version, set by the caller. 100 for GLSL ES, 120 for desktop GLSL */
    unsigned glsl_version = 0;
    /* Precision header, set by the caller. In OpenGLES, the fragment language
     * has no default precision qualifier for floating point types. */
    const char *glsl_precision_header = nullptr;

    /* Can only be changed from the module open function */
    video_format_t fmt = {0};

    /* Fragment shader, must be set from the module open function. It will be
     * deleted by the caller. */
    GLuint fshader = 0;

    /* Number of textures, cannot be 0 */
    unsigned tex_count = 0;

    /* Texture mapping (usually: GL_TEXTURE_2D), cannot be 0 */
    GLenum tex_target = 0;

    /* Set to true if textures are generated from pf_update() */
    bool handle_texs_gen = false;

    struct opengl_tex_cfg {
        /* Texture scale factor, cannot be 0 */
        vlc_rational_t w;
        vlc_rational_t h;

        /* The following is used and filled by the opengl_fragment_shader_init
         * function. */
        GLint internal;
        GLenum format;
        GLenum type;
    } texs[PICTURE_PLANE_MAX];

    /* The following is used and filled by the opengl_fragment_shader_init
     * function. */
    struct {
        GLint Texture[PICTURE_PLANE_MAX];
        GLint TexSize[PICTURE_PLANE_MAX]; /* for GL_TEXTURE_RECTANGLE */
        GLint Coefficients;
        GLint FillColor;
        GLint *pl_vars; /* for pl_sh_res */
    } uloc;
    bool yuv_color = false;
    GLfloat yuv_coefficients[16] = {0.0};

    struct pl_shader *pl_sh = nullptr;
    const struct pl_shader_res *pl_sh_res = nullptr;

    bool useable = false;
};