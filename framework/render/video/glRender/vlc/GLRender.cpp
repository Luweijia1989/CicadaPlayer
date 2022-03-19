#include "GLRender.h"
#include "GLSoftwareTextureConverter.h"
#include "wgl.h"
#include <iostream>

#ifdef WIN32
#define HAVE_GL_CORE_SYMBOLS
#endif

static inline GLsizei GetAlignedSize(unsigned size)
{
    /* Return the smallest larger or equal power of 2 */
    unsigned align = 1 << (8 * sizeof(unsigned) - clz(size));
    return ((align >> 1) == size) ? size : align;
}

static const GLfloat identity[] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};

static void getOrientationTransformMatrix(video_orientation_t orientation, GLfloat matrix[/*static */ 16])
{
    memcpy(matrix, identity, sizeof(identity));

    const int k_cos_pi = -1;
    const int k_cos_pi_2 = 0;
    const int k_cos_n_pi_2 = 0;

    const int k_sin_pi = 0;
    const int k_sin_pi_2 = 1;
    const int k_sin_n_pi_2 = -1;

    switch (orientation) {

        case ORIENT_ROTATED_90:
            matrix[0 * 4 + 0] = k_cos_pi_2;
            matrix[0 * 4 + 1] = -k_sin_pi_2;
            matrix[1 * 4 + 0] = k_sin_pi_2;
            matrix[1 * 4 + 1] = k_cos_pi_2;
            matrix[3 * 4 + 1] = 1;
            break;
        case ORIENT_ROTATED_180:
            matrix[0 * 4 + 0] = k_cos_pi;
            matrix[0 * 4 + 1] = -k_sin_pi;
            matrix[1 * 4 + 0] = k_sin_pi;
            matrix[1 * 4 + 1] = k_cos_pi;
            matrix[3 * 4 + 0] = 1;
            matrix[3 * 4 + 1] = 1;
            break;
        case ORIENT_ROTATED_270:
            matrix[0 * 4 + 0] = k_cos_n_pi_2;
            matrix[0 * 4 + 1] = -k_sin_n_pi_2;
            matrix[1 * 4 + 0] = k_sin_n_pi_2;
            matrix[1 * 4 + 1] = k_cos_n_pi_2;
            matrix[3 * 4 + 0] = 1;
            break;
        case ORIENT_HFLIPPED:
            matrix[0 * 4 + 0] = -1;
            matrix[3 * 4 + 0] = 1;
            break;
        case ORIENT_VFLIPPED:
            matrix[1 * 4 + 1] = -1;
            matrix[3 * 4 + 1] = 1;
            break;
        case ORIENT_TRANSPOSED:
            matrix[0 * 4 + 0] = 0;
            matrix[1 * 4 + 1] = 0;
            matrix[2 * 4 + 2] = -1;
            matrix[0 * 4 + 1] = 1;
            matrix[1 * 4 + 0] = 1;
            break;
        case ORIENT_ANTI_TRANSPOSED:
            matrix[0 * 4 + 0] = 0;
            matrix[1 * 4 + 1] = 0;
            matrix[2 * 4 + 2] = -1;
            matrix[0 * 4 + 1] = -1;
            matrix[1 * 4 + 0] = -1;
            matrix[3 * 4 + 0] = 1;
            matrix[3 * 4 + 1] = 1;
            break;
        default:
            break;
    }
}

GLRender::GLRender(video_format_t *format) : fmt(*format)
{
#ifdef WIN32
    glBase = new WGL();
#endif
}

GLRender::~GLRender()
{
    if (textureConvter) {
        const size_t main_tex_count = textureConvter->tex_count;
        const bool main_del_texs = !textureConvter->handle_texs_gen;

        vt.DeleteBuffers(main_tex_count, texture_buffer_object);

        if (main_del_texs) vt.DeleteTextures(main_tex_count, texture);
    }
    vt.DeleteBuffers(1, &vertex_buffer_object);
    vt.DeleteBuffers(1, &index_buffer_object);

    for (int i = 0; i < region_count; i++) {
        if (region[i].texture) vt.DeleteTextures(1, &region[i].texture);
    }
    free(region);

    destroyShaderProgram();

    delete glBase;
}

bool GLRender::initGL()
{
#if defined(USE_OPENGL_ES2) || defined(HAVE_GL_CORE_SYMBOLS)
#define GET_PROC_ADDR_CORE(name) vt.name = gl##name
#else
#define GET_PROC_ADDR_CORE(name) GET_PROC_ADDR_EXT(name, true)
#endif
#define GET_PROC_ADDR_EXT(name, type, critical)                                                                                            \
    do {                                                                                                                                   \
        vt.name = (type) glBase->getProcAddress("gl" #name);                                                                               \
        if (vt.name == NULL && critical) {                                                                                                 \
            std::clog << "gl" #name " symbol not found, bailing out\n";                                                                    \
            return false;                                                                                                                  \
        }                                                                                                                                  \
    } while (0)
#if defined(USE_OPENGL_ES2)
#define GET_PROC_ADDR(name) GET_PROC_ADDR_CORE(name)
#define GET_PROC_ADDR_CORE_GL(name) GET_PROC_ADDR_EXT(name, false) /* optional for GLES */
#else
#define GET_PROC_ADDR(name, type) GET_PROC_ADDR_EXT(name, type, true)
#define GET_PROC_ADDR_CORE_GL(name) GET_PROC_ADDR_CORE(name)
#endif
#define GET_PROC_ADDR_OPTIONAL(name, type) GET_PROC_ADDR_EXT(name, type, false) /* GL 3 or more */

    GET_PROC_ADDR_CORE(BindTexture);
    GET_PROC_ADDR_CORE(BlendFunc);
    GET_PROC_ADDR_CORE(Clear);
    GET_PROC_ADDR_CORE(ClearColor);
    GET_PROC_ADDR_CORE(DeleteTextures);
    GET_PROC_ADDR_CORE(DepthMask);
    GET_PROC_ADDR_CORE(Disable);
    GET_PROC_ADDR_CORE(DrawArrays);
    GET_PROC_ADDR_CORE(DrawElements);
    GET_PROC_ADDR_CORE(Enable);
    GET_PROC_ADDR_CORE(Finish);
    GET_PROC_ADDR_CORE(Flush);
    GET_PROC_ADDR_CORE(GenTextures);
    GET_PROC_ADDR_CORE(GetError);
    GET_PROC_ADDR_CORE(GetIntegerv);
    GET_PROC_ADDR_CORE(GetString);
    GET_PROC_ADDR_CORE(PixelStorei);
    GET_PROC_ADDR_CORE(TexImage2D);
    GET_PROC_ADDR_CORE(TexParameterf);
    GET_PROC_ADDR_CORE(TexParameteri);
    GET_PROC_ADDR_CORE(TexSubImage2D);
    GET_PROC_ADDR_CORE(Viewport);

    GET_PROC_ADDR_CORE_GL(GetTexLevelParameteriv);
    GET_PROC_ADDR_CORE_GL(TexEnvf);

    GET_PROC_ADDR(CreateShader, PFNGLCREATESHADERPROC);
    GET_PROC_ADDR(ShaderSource, PFNGLSHADERSOURCEPROC);
    GET_PROC_ADDR(CompileShader, PFNGLCOMPILESHADERARBPROC);
    GET_PROC_ADDR(AttachShader, PFNGLATTACHSHADERPROC);
    GET_PROC_ADDR(DeleteShader, PFNGLDELETESHADERPROC);

    GET_PROC_ADDR(GetProgramiv, PFNGLGETPROGRAMIVPROC);
    GET_PROC_ADDR(GetShaderiv, PFNGLGETSHADERIVPROC);
    GET_PROC_ADDR(GetProgramInfoLog, PFNGLGETPROGRAMINFOLOGPROC);
    GET_PROC_ADDR(GetShaderInfoLog, PFNGLGETSHADERINFOLOGPROC);

    GET_PROC_ADDR(GetUniformLocation, PFNGLGETUNIFORMLOCATIONPROC);
    GET_PROC_ADDR(GetAttribLocation, PFNGLGETATTRIBLOCATIONPROC);
    GET_PROC_ADDR(VertexAttribPointer, PFNGLVERTEXATTRIBPOINTERPROC);
    GET_PROC_ADDR(EnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAYPROC);
    GET_PROC_ADDR(UniformMatrix4fv, PFNGLUNIFORMMATRIX4FVPROC);
    GET_PROC_ADDR(UniformMatrix3fv, PFNGLUNIFORMMATRIX3FVPROC);
    GET_PROC_ADDR(UniformMatrix2fv, PFNGLUNIFORMMATRIX2FVPROC);
    GET_PROC_ADDR(Uniform4fv, PFNGLUNIFORM4FVPROC);
    GET_PROC_ADDR(Uniform4f, PFNGLUNIFORM4FPROC);
    GET_PROC_ADDR(Uniform3f, PFNGLUNIFORM3FPROC);
    GET_PROC_ADDR(Uniform2f, PFNGLUNIFORM2FPROC);
    GET_PROC_ADDR(Uniform1f, PFNGLUNIFORM1FPROC);
    GET_PROC_ADDR(Uniform1i, PFNGLUNIFORM1IPROC);

    GET_PROC_ADDR(CreateProgram, PFNGLCREATEPROGRAMPROC);
    GET_PROC_ADDR(LinkProgram, PFNGLLINKPROGRAMPROC);
    GET_PROC_ADDR(UseProgram, PFNGLUSEPROGRAMPROC);
    GET_PROC_ADDR(DeleteProgram, PFNGLDELETEPROGRAMPROC);

    GET_PROC_ADDR(ActiveTexture, PFNGLACTIVETEXTUREPROC);

    GET_PROC_ADDR(GenBuffers, PFNGLGENBUFFERSPROC);
    GET_PROC_ADDR(BindBuffer, PFNGLBINDBUFFERPROC);
    GET_PROC_ADDR(BufferData, PFNGLBUFFERDATAPROC);
    GET_PROC_ADDR(DeleteBuffers, PFNGLDELETEBUFFERSPROC);

    GET_PROC_ADDR_OPTIONAL(GetFramebufferAttachmentParameteriv, PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC);

    GET_PROC_ADDR_OPTIONAL(BufferSubData, PFNGLBUFFERSUBDATAPROC);
    GET_PROC_ADDR_OPTIONAL(BufferStorage, PFNGLBUFFERSTORAGEPROC);
    GET_PROC_ADDR_OPTIONAL(MapBufferRange, PFNGLMAPBUFFERRANGEPROC);
    GET_PROC_ADDR_OPTIONAL(FlushMappedBufferRange, PFNGLFLUSHMAPPEDBUFFERRANGEPROC);
    GET_PROC_ADDR_OPTIONAL(UnmapBuffer, PFNGLUNMAPBUFFERPROC);
    GET_PROC_ADDR_OPTIONAL(FenceSync, PFNGLFENCESYNCPROC);
    GET_PROC_ADDR_OPTIONAL(DeleteSync, PFNGLDELETESYNCPROC);
    GET_PROC_ADDR_OPTIONAL(ClientWaitSync, PFNGLCLIENTWAITSYNCPROC);
#undef GET_PROC_ADDR

    GL_ASSERT_NOERROR();

    extensions = (const char *) vt.GetString(GL_EXTENSIONS);
    if (extensions.length() == 0) {
        std::clog << "glGetString returned NULL" << std::endl;
        return false;
    }
#if !defined(USE_OPENGL_ES2)
    // Check for OpenGL < 2.0
    const unsigned char *ogl_version = vt.GetString(GL_VERSION);
    if (strverscmp((const char *) ogl_version, "2.0") < 0) {
        // Even with OpenGL < 2.0 we might have GLSL support,
        // so check the GLSL version before finally giving up:
        const unsigned char *glsl_version = vt.GetString(GL_SHADING_LANGUAGE_VERSION);
        if (!glsl_version || strverscmp((const char *) glsl_version, "1.10") < 0) {
            std::clog << "shaders not supported, bailing out" << std::endl;
            return false;
        }
    }
#endif

    /* Resize the format if it is greater than the maximum texture size
     * supported by the hardware */
    GLint max_tex_size;
    vt.GetIntegerv(GL_MAX_TEXTURE_SIZE, &max_tex_size);

    if ((GLint) fmt.i_width > max_tex_size || (GLint) fmt.i_height > max_tex_size) ResizeFormatToGLMaxTexSize(max_tex_size);

#if defined(USE_OPENGL_ES2)
    /* OpenGL ES 2 includes support for non-power of 2 textures by specification
     * so checks for extensions are bound to fail. Check for OpenGL ES version instead. */
    supports_npot = true;
#else
    supports_npot = HasExtension(extensions.c_str(), "GL_ARB_texture_non_power_of_two") ||
                    HasExtension(extensions.c_str(), "GL_APPLE_texture_2D_limited_npot");
#endif

    GL_ASSERT_NOERROR();
    int ret = initShaderProgram();
    if (ret != VLC_SUCCESS) {
        std::clog << "could not init tex converter for %4.4s" << (const char *) &fmt.i_chroma << std::endl;
        return false;
    }

    GL_ASSERT_NOERROR();

    /* Update the fmt to main program one */
    fmt = textureConvter->fmt;

    /* Texture size */
    for (unsigned j = 0; j < textureConvter->tex_count; j++) {
        const GLsizei w = fmt.i_visible_width * textureConvter->texs[j].w.num / textureConvter->texs[j].w.den;
        const GLsizei h = fmt.i_visible_height * textureConvter->texs[j].h.num / textureConvter->texs[j].h.den;
        if (supports_npot) {
            tex_width[j] = w;
            tex_height[j] = h;
        } else {
            tex_width[j] = GetAlignedSize(w);
            tex_height[j] = GetAlignedSize(h);
        }
    }

    if (!textureConvter->handle_texs_gen) {
        ret = GenTextures(tex_width, tex_height, texture);
        if (ret != VLC_SUCCESS) {
            return false;
        }
    }

    /* */
    vt.Disable(GL_BLEND);
    vt.Disable(GL_DEPTH_TEST);
    vt.DepthMask(GL_FALSE);
    vt.Enable(GL_CULL_FACE);
    vt.ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    vt.Clear(GL_COLOR_BUFFER_BIT);

    vt.GenBuffers(1, &vertex_buffer_object);
    vt.GenBuffers(1, &index_buffer_object);
    vt.GenBuffers(textureConvter->tex_count, texture_buffer_object);

    /* */
    region_count = 0;
    region = NULL;

    return true;
}

int GLRender::GenTextures(const GLsizei *tex_width, const GLsizei *tex_height, GLuint *textures)
{
    vt.GenTextures(textureConvter->tex_count, textures);

    for (unsigned i = 0; i < textureConvter->tex_count; i++) {
        vt.BindTexture(textureConvter->tex_target, textures[i]);

#if !defined(USE_OPENGL_ES2)
        /* Set the texture parameters */
        vt.TexParameterf(textureConvter->tex_target, GL_TEXTURE_PRIORITY, 1.0);
        vt.TexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
#endif

        vt.TexParameteri(textureConvter->tex_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        vt.TexParameteri(textureConvter->tex_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        vt.TexParameteri(textureConvter->tex_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        vt.TexParameteri(textureConvter->tex_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    int ret = textureConvter->pf_allocate_textures(textures, tex_width, tex_height);
    if (ret != VLC_SUCCESS) {
        vt.DeleteTextures(textureConvter->tex_count, textures);
        memset(textures, 0, textureConvter->tex_count * sizeof(GLuint));
        return ret;
    }

    return VLC_SUCCESS;
}

void GLRender::ResizeFormatToGLMaxTexSize(unsigned int max_tex_size)
{
    if (fmt.i_width > fmt.i_height) {
        unsigned int const vis_w = fmt.i_visible_width;
        unsigned int const vis_h = fmt.i_visible_height;
        unsigned int const nw_w = max_tex_size;
        unsigned int const nw_vis_w = nw_w * vis_w / fmt.i_width;

        fmt.i_height = nw_w * fmt.i_height / fmt.i_width;
        fmt.i_width = nw_w;
        fmt.i_visible_height = nw_vis_w * vis_h / vis_w;
        fmt.i_visible_width = nw_vis_w;
    } else {
        unsigned int const vis_w = fmt.i_visible_width;
        unsigned int const vis_h = fmt.i_visible_height;
        unsigned int const nw_h = max_tex_size;
        unsigned int const nw_vis_h = nw_h * vis_h / fmt.i_height;

        fmt.i_width = nw_h * fmt.i_width / fmt.i_height;
        fmt.i_height = nw_h;
        fmt.i_visible_width = nw_vis_h * vis_w / vis_h;
        fmt.i_visible_height = nw_vis_h;
    }
}

void GLRender::getViewpointMatrixes()
{
    memcpy(prgm.var.ProjectionMatrix, identity, sizeof(identity));
    memcpy(prgm.var.ZRotMatrix, identity, sizeof(identity));
    memcpy(prgm.var.YRotMatrix, identity, sizeof(identity));
    memcpy(prgm.var.XRotMatrix, identity, sizeof(identity));
    memcpy(prgm.var.ZoomMatrix, identity, sizeof(identity));
}

int GLRender::initShaderProgram()
{
    int ret;

    const vlc_chroma_description_t *desc = vlc_fourcc_GetChromaDescription(fmt.i_chroma);

    if (!desc) {
        return VLC_EGENERIC;
    }
    if (desc->plane_count == 0) {

    } else
        textureConvter = new GLSoftwareTextureConverter();

    if (!textureConvter) return VLC_ENOMEM;

    ret = textureConvter->init();
    if (ret != VLC_SUCCESS) return VLC_EGENERIC;

    textureConvter->gl = glBase;
    textureConvter->vt = &vt;
    textureConvter->b_dump_shaders = true;
    textureConvter->glexts = extensions;
#if defined(USE_OPENGL_ES2)
    textureConvter->is_gles = true;
    textureConvter->glsl_version = 100;
    textureConvter->glsl_precision_header = "precision highp float;\n";
#else
    textureConvter->is_gles = false;
    textureConvter->glsl_version = 120;
    textureConvter->glsl_precision_header = "";
#endif
    textureConvter->fmt = fmt;

#if 0
    // create the main libplacebo context
    if (!subpics)
    {
        tc->pl_ctx = pl_context_create(PL_API_VER, &(struct pl_context_params) {
            .log_cb    = log_cb,
            .log_priv  = tc,
            .log_level = PL_LOG_INFO,
        });
        if (tc->pl_ctx) {
#if PL_API_VER >= 20
            tc->pl_sh = pl_shader_alloc(tc->pl_ctx, NULL);
#elif PL_API_VER >= 6
            tc->pl_sh = pl_shader_alloc(tc->pl_ctx, NULL, 0);
#else
            tc->pl_sh = pl_shader_alloc(tc->pl_ctx, NULL, 0, 0);
#endif
        }
    }
#endif

    ret = linkShaderProgram();
    if (ret != VLC_SUCCESS) {
        return VLC_EGENERIC;
    }

    getOrientationTransformMatrix(fmt.orientation, prgm.var.OrientationMatrix);
    getViewpointMatrixes();

    return VLC_SUCCESS;
}

void GLRender::destroyShaderProgram()
{
    if (textureConvter) delete textureConvter;

    if (prgm.id != 0) vt.DeleteProgram(prgm.id);

#if 0
    FREENULL(tc->uloc.pl_vars);
    if (tc->pl_ctx)
        pl_context_destroy(&tc->pl_ctx);
#endif
}

int GLRender::linkShaderProgram()
{
    GLuint vertex_shader = BuildVertexShader(textureConvter->tex_count);
    GLuint shaders[] = {textureConvter->fshader, vertex_shader};

    /* Check shaders messages */
    for (unsigned i = 0; i < 2; i++) {
        int infoLength;
        vt.GetShaderiv(shaders[i], GL_INFO_LOG_LENGTH, &infoLength);
        if (infoLength <= 1) continue;

        char *infolog = (char *) malloc(infoLength);
        if (infolog != NULL) {
            int charsWritten;
            vt.GetShaderInfoLog(shaders[i], infoLength, &charsWritten, infolog);
            std::clog << "shader %d: %s" << i << infolog;
            free(infolog);
        }
    }

    prgm.id = vt.CreateProgram();
    vt.AttachShader(prgm.id, textureConvter->fshader);
    vt.AttachShader(prgm.id, vertex_shader);
    vt.LinkProgram(prgm.id);

    vt.DeleteShader(vertex_shader);
    vt.DeleteShader(textureConvter->fshader);

    /* Check program messages */
    int infoLength = 0;
    vt.GetProgramiv(prgm.id, GL_INFO_LOG_LENGTH, &infoLength);
    if (infoLength > 1) {
        char *infolog = (char *) malloc(infoLength);
        if (infolog != NULL) {
            int charsWritten;
            vt.GetProgramInfoLog(prgm.id, infoLength, &charsWritten, infolog);
            std::clog << "shader program: %s" << infolog;
            free(infolog);
        }

        /* If there is some message, better to check linking is ok */
        GLint link_status = GL_TRUE;
        vt.GetProgramiv(prgm.id, GL_LINK_STATUS, &link_status);
        if (link_status == GL_FALSE) {
            std::clog << "Unable to use program";
            goto error;
        }
    }

    /* Fetch UniformLocations and AttribLocations */
#define GET_LOC(type, x, str)                                                                                                              \
    do {                                                                                                                                   \
        x = vt.Get##type##Location(prgm.id, str);                                                                                          \
        assert(x != -1);                                                                                                                   \
        if (x == -1) {                                                                                                                     \
            std::clog << "Unable to Get" #type "Location(%s)\n" << str << std::endl;                                                       \
            goto error;                                                                                                                    \
        }                                                                                                                                  \
    } while (0)
#define GET_ULOC(x, str) GET_LOC(Uniform, prgm.uloc.x, str)
#define GET_ALOC(x, str) GET_LOC(Attrib, prgm.aloc.x, str)
    GET_ULOC(OrientationMatrix, "OrientationMatrix");
    GET_ULOC(ProjectionMatrix, "ProjectionMatrix");
    GET_ULOC(ZRotMatrix, "ZRotMatrix");
    GET_ULOC(YRotMatrix, "YRotMatrix");
    GET_ULOC(XRotMatrix, "XRotMatrix");
    GET_ULOC(ZoomMatrix, "ZoomMatrix");

    GET_ALOC(VertexPosition, "VertexPosition");
    GET_ALOC(MultiTexCoord[0], "MultiTexCoord0");
    /* MultiTexCoord 1 and 2 can be optimized out if not used */
    if (textureConvter->tex_count > 1)
        GET_ALOC(MultiTexCoord[1], "MultiTexCoord1");
    else
        prgm.aloc.MultiTexCoord[1] = -1;
    if (textureConvter->tex_count > 2)
        GET_ALOC(MultiTexCoord[2], "MultiTexCoord2");
    else
        prgm.aloc.MultiTexCoord[2] = -1;
#undef GET_LOC
#undef GET_ULOC
#undef GET_ALOC
    int ret = textureConvter->pf_fetch_locations(prgm.id);
    assert(ret == VLC_SUCCESS);
    if (ret != VLC_SUCCESS) {
        std::clog << "Unable to get locations from tex_conv";
        goto error;
    }

    return VLC_SUCCESS;

error:
    vt.DeleteProgram(prgm.id);
    prgm.id = 0;
    return VLC_EGENERIC;
}

GLuint GLRender::BuildVertexShader(unsigned plane_count)
{
    /* Basic vertex shader */
    static const char *template_str =
            "#version %u\n"
            "varying vec2 TexCoord0;\n"
            "attribute vec4 MultiTexCoord0;\n"
            "%s%s"
            "attribute vec3 VertexPosition;\n"
            "uniform mat4 OrientationMatrix;\n"
            "uniform mat4 ProjectionMatrix;\n"
            "uniform mat4 XRotMatrix;\n"
            "uniform mat4 YRotMatrix;\n"
            "uniform mat4 ZRotMatrix;\n"
            "uniform mat4 ZoomMatrix;\n"
            "void main() {\n"
            " TexCoord0 = vec4(OrientationMatrix * MultiTexCoord0).st;\n"
            "%s%s"
            " gl_Position = ProjectionMatrix * ZoomMatrix * ZRotMatrix * XRotMatrix * YRotMatrix * vec4(VertexPosition, 1.0);\n"
            "}";

    const char *coord1_header = plane_count > 1 ? "varying vec2 TexCoord1;\nattribute vec4 MultiTexCoord1;\n" : "";
    const char *coord1_code = plane_count > 1 ? " TexCoord1 = vec4(OrientationMatrix * MultiTexCoord1).st;\n" : "";
    const char *coord2_header = plane_count > 2 ? "varying vec2 TexCoord2;\nattribute vec4 MultiTexCoord2;\n" : "";
    const char *coord2_code = plane_count > 2 ? " TexCoord2 = vec4(OrientationMatrix * MultiTexCoord2).st;\n" : "";

    char *code;
    if (asprintf(&code, template_str, textureConvter->glsl_version, coord1_header, coord2_header, coord1_code, coord2_code) < 0) return 0;

    GLuint shader = textureConvter->vt->CreateShader(GL_VERTEX_SHADER);
    textureConvter->vt->ShaderSource(shader, 1, (const char **) &code, NULL);
    if (textureConvter->b_dump_shaders)
        std::clog << "\n=== Vertex shader for fourcc: %4.4s ===\n%s\n" << (const char *) &textureConvter->fmt.i_chroma << code << std::endl;
    textureConvter->vt->CompileShader(shader);
    free(code);
    return shader;
}