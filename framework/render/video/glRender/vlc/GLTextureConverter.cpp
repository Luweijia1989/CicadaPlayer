#include "GLTextureConverter.h"
#include <assert.h>
#include <iostream>
#include <stdlib.h>

#ifdef HAVE_LIBPLACEBO
#include <libplacebo/shaders.h>
#include <libplacebo/shaders/colorspace.h>
#endif

#include "gl_common.h"

#ifndef GL_RED
#define GL_RED 0x1903
#endif
#ifndef GL_RG
#define GL_RG 0x8227
#endif
#ifndef GL_R16
#define GL_R16 0x822A
#endif
#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif
#ifndef GL_RG16
#define GL_RG16 0x822C
#endif
#ifndef GL_LUMINANCE16
#define GL_LUMINANCE16 0x8042
#endif
#ifndef GL_TEXTURE_RED_SIZE
#define GL_TEXTURE_RED_SIZE 0x805C
#endif
#ifndef GL_TEXTURE_LUMINANCE_SIZE
#define GL_TEXTURE_LUMINANCE_SIZE 0x8060
#endif

OpenGLTextureConverter::OpenGLTextureConverter()
{}

int OpenGLTextureConverter::GetTexFormatSize(int target, int tex_format, int tex_internal, int tex_type)
{
    if (!vt->GetTexLevelParameteriv) return -1;

    GLint tex_param_size;
    int mul = 1;
    switch (tex_format) {
        case GL_BGRA:
            mul = 4;
            /* fall through */
        case GL_RED:
        case GL_RG:
            tex_param_size = GL_TEXTURE_RED_SIZE;
            break;
        case GL_LUMINANCE:
            tex_param_size = GL_TEXTURE_LUMINANCE_SIZE;
            break;
        default:
            return -1;
    }
    GLuint texture;

    vt->GenTextures(1, &texture);
    vt->BindTexture(target, texture);
    vt->TexImage2D(target, 0, tex_internal, 64, 64, 0, tex_format, tex_type, NULL);
    GLint size = 0;
    vt->GetTexLevelParameteriv(target, 0, tex_param_size, &size);

    vt->DeleteTextures(1, &texture);
    return size > 0 ? size * mul : size;
}

int OpenGLTextureConverter::tc_yuv_base_init(GLenum tex_target, vlc_fourcc_t chroma, const vlc_chroma_description_t *desc,
                                             video_color_space_t yuv_space, bool *swap_uv, const char *swizzle_per_tex[])
{
    GLint oneplane_texfmt, oneplane16_texfmt, twoplanes_texfmt, twoplanes16_texfmt;

    if (HasExtension(glexts, "GL_ARB_texture_rg")) {
        oneplane_texfmt = GL_RED;
        oneplane16_texfmt = GL_R16;
        twoplanes_texfmt = GL_RG;
        twoplanes16_texfmt = GL_RG16;
    } else {
        oneplane_texfmt = GL_LUMINANCE;
        oneplane16_texfmt = GL_LUMINANCE16;
        twoplanes_texfmt = GL_LUMINANCE_ALPHA;
        twoplanes16_texfmt = 0;
    }

    float yuv_range_correction = 1.0;
    if (desc->pixel_size == 2) {
        if (GetTexFormatSize(tex_target, oneplane_texfmt, oneplane16_texfmt, GL_UNSIGNED_SHORT) != 16) return -1;

        /* Do a bit shift if samples are stored on LSB */
        if (chroma != VLC_CODEC_P010) yuv_range_correction = (float) ((1 << 16) - 1) / ((1 << desc->pixel_bits) - 1);
    }

    if (desc->plane_count == 3) {
        GLint internal = 0;
        GLenum type = 0;

        if (desc->pixel_size == 1) {
            internal = oneplane_texfmt;
            type = GL_UNSIGNED_BYTE;
        } else if (desc->pixel_size == 2) {
            internal = oneplane16_texfmt;
            type = GL_UNSIGNED_SHORT;
        } else
            return -1;

        assert(internal != 0 && type != 0);

        tex_count = 3;
        for (unsigned i = 0; i < tex_count; ++i) {
            texs[i] = {
                    {desc->p[i].w.num, desc->p[i].w.den}, {desc->p[i].h.num, desc->p[i].h.den}, internal, (GLenum) oneplane_texfmt, type};
        }

        if (oneplane_texfmt == GL_RED) swizzle_per_tex[0] = swizzle_per_tex[1] = swizzle_per_tex[2] = "r";
    } else if (desc->plane_count == 2) {
        tex_count = 2;

        if (desc->pixel_size == 1) {
            texs[0] = {{1, 1}, {1, 1}, oneplane_texfmt, (GLenum) oneplane_texfmt, GL_UNSIGNED_BYTE};
            texs[1] = {{1, 2}, {1, 2}, twoplanes_texfmt, (GLenum) twoplanes_texfmt, GL_UNSIGNED_BYTE};
        } else if (desc->pixel_size == 2) {
            if (twoplanes16_texfmt == 0 || GetTexFormatSize(tex_target, twoplanes_texfmt, twoplanes16_texfmt, GL_UNSIGNED_SHORT) != 16)
                return -1;
            texs[0] = {{1, 1}, {1, 1}, oneplane16_texfmt, (GLenum) oneplane_texfmt, GL_UNSIGNED_SHORT};
            texs[1] = {{1, 2}, {1, 2}, twoplanes16_texfmt, (GLenum) twoplanes_texfmt, GL_UNSIGNED_SHORT};
        } else
            return -1;

        if (oneplane_texfmt == GL_RED) {
            swizzle_per_tex[0] = "r";
            swizzle_per_tex[1] = "rg";
        } else {
            swizzle_per_tex[0] = NULL;
            swizzle_per_tex[1] = "xa";
        }
    } else if (desc->plane_count == 1) {
        /* Y1 U Y2 V fits in R G B A */
        tex_count = 1;
        texs[0] = {{1, 2}, {1, 1}, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE};

        /*
         * Set swizzling in Y1 U V order
         * R  G  B  A
         * U  Y1 V  Y2 => GRB
         * Y1 U  Y2 V  => RGA
         * V  Y1 U  Y2 => GBR
         * Y1 V  Y2 U  => RAG
         */
        switch (chroma) {
            case VLC_CODEC_UYVY:
                swizzle_per_tex[0] = "grb";
                break;
            case VLC_CODEC_YUYV:
                swizzle_per_tex[0] = "rga";
                break;
            case VLC_CODEC_VYUY:
                swizzle_per_tex[0] = "gbr";
                break;
            case VLC_CODEC_YVYU:
                swizzle_per_tex[0] = "rag";
                break;
            default:
                assert(!"missing chroma");
                return -1;
        }
    } else
        return -1;

    /* [R/G/B][Y U V O] from TV range to full range
     * XXX we could also do hue/brightness/constrast/gamma
     * by simply changing the coefficients
     */
    static const float matrix_bt601_tv2full[12] = {
            1.164383561643836,
            0.0000,
            1.596026785714286,
            -0.874202217873451,
            1.164383561643836,
            -0.391762290094914,
            -0.812967647237771,
            0.531667823499146,
            1.164383561643836,
            2.017232142857142,
            0.0000,
            -1.085630789302022,
    };
    static const float matrix_bt709_tv2full[12] = {
            1.164383561643836,
            0.0000,
            1.792741071428571,
            -0.972945075016308,
            1.164383561643836,
            -0.21324861427373,
            -0.532909328559444,
            0.301482665475862,
            1.164383561643836,
            2.112401785714286,
            0.0000,
            -1.133402217873451,
    };
    static const float matrix_bt2020_tv2full[12] = {
            1.164383530616760,
            0.0000,
            1.678674221038818,
            -0.915687978267670,
            1.164383530616760,
            -0.187326118350029,
            -0.650424420833588,
            0.347458571195602,
            1.164383530616760,
            2.141772270202637,
            0.0000,
            -1.148145079612732,
    };

    const float *matrix;
    switch (yuv_space) {
        case COLOR_SPACE_BT601:
            matrix = matrix_bt601_tv2full;
            break;
        case COLOR_SPACE_BT2020:
            matrix = matrix_bt2020_tv2full;
            break;
        default:
            matrix = matrix_bt709_tv2full;
    };

    for (int i = 0; i < 4; i++) {
        float correction = i < 3 ? yuv_range_correction : 1.f;
        /* We place coefficient values for coefficient[4] in one array from
         * matrix values. Notice that we fill values from top down instead
         * of left to right.*/
        for (int j = 0; j < 4; j++) yuv_coefficients[i * 4 + j] = j < 3 ? correction * matrix[j * 4 + i] : 0.f;
    }

    yuv_color = true;

    *swap_uv = chroma == VLC_CODEC_YV12 || chroma == VLC_CODEC_YV9 || chroma == VLC_CODEC_NV21;
    return 0;
}

int OpenGLTextureConverter::tc_rgb_base_init(GLenum tex_target, vlc_fourcc_t chroma)
{
    (void) tex_target;

    switch (chroma) {
        case VLC_CODEC_RGB32:
        case VLC_CODEC_RGBA:
            texs[0] = {{1, 1}, {1, 1}, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE};
            break;
        case VLC_CODEC_BGRA: {
            if (GetTexFormatSize(tex_target, GL_BGRA, GL_RGBA, GL_UNSIGNED_BYTE) != 32) return -1;
            texs[0] = {{1, 1}, {1, 1}, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE};
            break;
        }
        default:
            return -1;
    }
    tex_count = 1;
    return 0;
}

int OpenGLTextureConverter::tc_base_fetch_locations(GLuint program)
{
    if (yuv_color) {
        uloc.Coefficients = vt->GetUniformLocation(program, "Coefficients");
        if (uloc.Coefficients == -1) return -1;
    }

    for (unsigned int i = 0; i < tex_count; ++i) {
        char name[sizeof("TextureX")];
        snprintf(name, sizeof(name), "Texture%1u", i);
        uloc.Texture[i] = vt->GetUniformLocation(program, name);
        if (uloc.Texture[i] == -1) return -1;
        if (tex_target == GL_TEXTURE_RECTANGLE) {
            snprintf(name, sizeof(name), "TexSize%1u", i);
            uloc.TexSize[i] = vt->GetUniformLocation(program, name);
            if (uloc.TexSize[i] == -1) return -1;
        }
    }

    uloc.FillColor = vt->GetUniformLocation(program, "FillColor");
    if (uloc.FillColor == -1) return -1;

#ifdef HAVE_LIBPLACEBO
    const struct pl_shader_res *res = pl_sh_res;
    for (int i = 0; res && i < res->num_variables; i++) {
        struct pl_shader_var sv = res->variables[i];
        uloc.pl_vars[i] = vt->GetUniformLocation(program, sv.var.name);
    }
#endif

    return 0;
}

void OpenGLTextureConverter::tc_base_prepare_shader(const GLsizei *tex_width, const GLsizei *tex_height, float alpha)
{
    (void) tex_width;
    (void) tex_height;

    if (yuv_color) vt->Uniform4fv(uloc.Coefficients, 4, yuv_coefficients);

    for (unsigned i = 0; i < tex_count; ++i) vt->Uniform1i(uloc.Texture[i], i);

    vt->Uniform4f(uloc.FillColor, 1.0f, 1.0f, 1.0f, alpha);

    if (tex_target == GL_TEXTURE_RECTANGLE) {
        for (unsigned i = 0; i < tex_count; ++i) vt->Uniform2f(uloc.TexSize[i], tex_width[i], tex_height[i]);
    }

#ifdef HAVE_LIBPLACEBO
    const struct pl_shader_res *res = pl_sh_res;
    for (int i = 0; res && i < res->num_variables; i++) {
        GLint loc = uloc.pl_vars[i];
        if (loc == -1)// uniform optimized out
            continue;

        struct pl_shader_var sv = res->variables[i];
#if PL_API_VER >= 4
        struct pl_var var = sv.var;
        // libplacebo doesn't need anything else anyway
        if (var.type != PL_VAR_FLOAT) continue;
#else
        struct ra_var var = sv.var;
        // libplacebo doesn't need anything else anyway
        if (var.type != RA_VAR_FLOAT) continue;
#endif

        if (var.dim_m > 1 && var.dim_m != var.dim_v) continue;

        const float *f = sv.data;
        switch (var.dim_m) {
            case 4:
                vt->UniformMatrix4fv(loc, 1, GL_FALSE, f);
                break;
            case 3:
                vt->UniformMatrix3fv(loc, 1, GL_FALSE, f);
                break;
            case 2:
                vt->UniformMatrix2fv(loc, 1, GL_FALSE, f);
                break;

            case 1:
                switch (var.dim_v) {
                    case 1:
                        vt->Uniform1f(loc, f[0]);
                        break;
                    case 2:
                        vt->Uniform2f(loc, f[0], f[1]);
                        break;
                    case 3:
                        vt->Uniform3f(loc, f[0], f[1], f[2]);
                        break;
                    case 4:
                        vt->Uniform4f(loc, f[0], f[1], f[2], f[3]);
                        break;
                }
                break;
        }
    }
#endif
}

int OpenGLTextureConverter::tc_xyz12_fetch_locations(GLuint program)
{
    uloc.Texture[0] = vt->GetUniformLocation(program, "Texture0");
    return uloc.Texture[0] != -1 ? 0 : -1;
}

void OpenGLTextureConverter::tc_xyz12_prepare_shader(const GLsizei *tex_width, const GLsizei *tex_height, float alpha)
{
    (void) tex_width;
    (void) tex_height;
    (void) alpha;
    vt->Uniform1i(uloc.Texture[0], 0);
}

GLuint OpenGLTextureConverter::xyz12_shader_init()
{
    base_shader = false;

    tex_count = 1;
    tex_target = GL_TEXTURE_2D;
    texs[0] = {{1, 1}, {1, 1}, GL_RGB, GL_RGB, GL_UNSIGNED_SHORT};

    /* Shader for XYZ to RGB correction
     * 3 steps :
     *  - XYZ gamma correction
     *  - XYZ to RGB matrix conversion
     *  - reverse RGB gamma correction
     */
    static const char *shader_template = "#version %u\n"
                                         "%s"
                                         "uniform sampler2D Texture0;"
                                         "uniform vec4 xyz_gamma = vec4(2.6);"
                                         "uniform vec4 rgb_gamma = vec4(1.0/2.2);"
                                         /* WARN: matrix Is filled column by column (not row !) */
                                         "uniform mat4 matrix_xyz_rgb = mat4("
                                         "    3.240454 , -0.9692660, 0.0556434, 0.0,"
                                         "   -1.5371385,  1.8760108, -0.2040259, 0.0,"
                                         "    -0.4985314, 0.0415560, 1.0572252,  0.0,"
                                         "    0.0,      0.0,         0.0,        1.0 "
                                         " );"

                                         "varying vec2 TexCoord0;"
                                         "void main()"
                                         "{ "
                                         " vec4 v_in, v_out;"
                                         " v_in  = texture2D(Texture0, TexCoord0);"
                                         " v_in = pow(v_in, xyz_gamma);"
                                         " v_out = matrix_xyz_rgb * v_in ;"
                                         " v_out = pow(v_out, rgb_gamma) ;"
                                         " v_out = clamp(v_out, 0.0, 1.0) ;"
                                         " gl_FragColor = v_out;"
                                         "}";

    char *code;
    if (asprintf(&code, shader_template, glsl_version, glsl_precision_header) < 0) return 0;

    GLuint fragment_shader = vt->CreateShader(GL_FRAGMENT_SHADER);
    vt->ShaderSource(fragment_shader, 1, (const char **) &code, NULL);
    vt->CompileShader(fragment_shader);
    free(code);
    return fragment_shader;
}

#ifdef HAVE_LIBPLACEBO
static struct pl_color_space pl_color_space_from_video_format(const video_format_t *fmt)
{
    static enum pl_color_primaries primaries[COLOR_PRIMARIES_MAX + 1] = {
            [COLOR_PRIMARIES_UNDEF] = PL_COLOR_PRIM_UNKNOWN,        [COLOR_PRIMARIES_BT601_525] = PL_COLOR_PRIM_BT_601_525,
            [COLOR_PRIMARIES_BT601_625] = PL_COLOR_PRIM_BT_601_625, [COLOR_PRIMARIES_BT709] = PL_COLOR_PRIM_BT_709,
            [COLOR_PRIMARIES_BT2020] = PL_COLOR_PRIM_BT_2020,       [COLOR_PRIMARIES_DCI_P3] = PL_COLOR_PRIM_DCI_P3,
            [COLOR_PRIMARIES_BT470_M] = PL_COLOR_PRIM_BT_470M,
    };

    static enum pl_color_transfer transfers[TRANSFER_FUNC_MAX + 1] = {
            [TRANSFER_FUNC_UNDEF] = PL_COLOR_TRC_UNKNOWN,
            [TRANSFER_FUNC_LINEAR] = PL_COLOR_TRC_LINEAR,
            [TRANSFER_FUNC_SRGB] = PL_COLOR_TRC_SRGB,
            [TRANSFER_FUNC_SMPTE_ST2084] = PL_COLOR_TRC_PQ,
            [TRANSFER_FUNC_HLG] = PL_COLOR_TRC_HLG,
            // these are all designed to be displayed on BT.1886 displays, so this
            // is the correct way to handle them in libplacebo
            [TRANSFER_FUNC_BT470_BG] = PL_COLOR_TRC_BT_1886,
            [TRANSFER_FUNC_BT470_M] = PL_COLOR_TRC_BT_1886,
            [TRANSFER_FUNC_BT709] = PL_COLOR_TRC_BT_1886,
            [TRANSFER_FUNC_SMPTE_240] = PL_COLOR_TRC_BT_1886,
    };

    // Derive the signal peak/avg from the color light level metadata
    float sig_peak = fmt->lighting.MaxCLL / PL_COLOR_REF_WHITE;
    float sig_avg = fmt->lighting.MaxFALL / PL_COLOR_REF_WHITE;

    // As a fallback value for the signal peak, we can also use the mastering
    // metadata's luminance information
    if (!sig_peak) sig_peak = fmt->mastering.max_luminance / (10000.0 * PL_COLOR_REF_WHITE);

    // Sanitize the sig_peak/sig_avg, because of buggy or low quality tagging
    // that's sadly common in lots of typical sources
    sig_peak = (sig_peak > 1.0 && sig_peak <= 100.0) ? sig_peak : 0.0;
    sig_avg = (sig_avg >= 0.0 && sig_avg <= 1.0) ? sig_avg : 0.0;

    return (struct pl_color_space){
            .primaries = primaries[fmt->primaries],
            .transfer = transfers[fmt->transfer],
            .light = PL_COLOR_LIGHT_UNKNOWN,
            .sig_peak = sig_peak,
            .sig_avg = sig_avg,
    };
}
#endif

GLuint OpenGLTextureConverter::pf_fragment_shader_init(GLenum tex_target, vlc_fourcc_t chroma, video_color_space_t yuv_space)
{
    const char *swizzle_per_tex[PICTURE_PLANE_MAX] = {
            NULL,
    };
    const bool is_yuv = vlc_fourcc_IsYUV(chroma);
    bool yuv_swap_uv = false;
    int ret;

    const vlc_chroma_description_t *desc = vlc_fourcc_GetChromaDescription(chroma);
    if (desc == NULL) return -1;

    if (chroma == VLC_CODEC_XYZ12) return xyz12_shader_init();

    base_shader = true;
    if (is_yuv)
        ret = tc_yuv_base_init(tex_target, chroma, desc, yuv_space, &yuv_swap_uv, swizzle_per_tex);
    else
        ret = tc_rgb_base_init(tex_target, chroma);

    if (ret != 0) return 0;

    const char *sampler = NULL;
    const char *lookup = NULL;
    const char *coord_name = NULL;
    switch (tex_target) {
        case GL_TEXTURE_2D:
            sampler = "sampler2D";
            lookup = "texture2D";
            coord_name = "TexCoord";
            break;
        case GL_TEXTURE_RECTANGLE:
            sampler = "sampler2DRect";
            lookup = "texture2DRect";
            coord_name = "TexCoordRect";
            break;
        default:
            break;
    }

    struct vlc_memstream ms;
    if (vlc_memstream_open(&ms) != 0) return 0;

#define ADD(x) vlc_memstream_puts(&ms, x)
#define ADDF(x, ...) vlc_memstream_printf(&ms, x, ##__VA_ARGS__)

    ADDF("#version %u\n%s", glsl_version, glsl_precision_header);

    for (unsigned i = 0; i < tex_count; ++i)
        ADDF("uniform %s Texture%u;\n"
             "varying vec2 TexCoord%u;\n",
             sampler, i, i);

#ifdef HAVE_LIBPLACEBO
    if (pl_sh) {
        struct pl_shader *sh = pl_sh;
        struct pl_color_map_params color_params = pl_color_map_default_params;
        color_params.intent = var_InheritInteger(gl, "rendering-intent");
        color_params.tone_mapping_algo = var_InheritInteger(gl, "tone-mapping");
        color_params.tone_mapping_param = var_InheritFloat(gl, "tone-mapping-param");
#if PL_API_VER >= 10
        color_params.desaturation_strength = var_InheritFloat(gl, "desat-strength");
        color_params.desaturation_exponent = var_InheritFloat(gl, "desat-exponent");
        color_params.desaturation_base = var_InheritFloat(gl, "desat-base");
        color_params.max_boost = var_InheritFloat(gl, "max-boost");
#else
        color_params.tone_mapping_desaturate = var_InheritFloat(gl, "tone-mapping-desat");
#endif
        color_params.gamut_warning = var_InheritBool(gl, "tone-mapping-warn");

        struct pl_color_space dst_space = pl_color_space_unknown;
        dst_space.primaries = var_InheritInteger(gl, "target-prim");
        dst_space.transfer = var_InheritInteger(gl, "target-trc");

        pl_shader_color_map(sh, &color_params, pl_color_space_from_video_format(&fmt), dst_space, NULL, false);

        struct pl_shader_obj *dither_state = NULL;
        int method = var_InheritInteger(gl, "dither-algo");
        if (method >= 0) {

            unsigned out_bits = 0;
            int override = var_InheritInteger(gl, "dither-depth");
            if (override > 0)
                out_bits = override;
            else {
                GLint fb_depth = 0;
#if !defined(USE_OPENGL_ES2)
                /* fetch framebuffer depth (we are already bound to the default one). */
                if (vt->GetFramebufferAttachmentParameteriv != NULL)
                    vt->GetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_BACK_LEFT, GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE, &fb_depth);
#endif
                if (fb_depth <= 0) fb_depth = 8;
                out_bits = fb_depth;
            }

            pl_shader_dither(sh, out_bits, &dither_state,
                             &(struct pl_dither_params){
                                     .method = method,
                                     .lut_size = 4,// avoid too large values, since this gets embedded
                             });
        }

        const struct pl_shader_res *res = pl_sh_res = pl_shader_finalize(sh);
        pl_shader_obj_destroy(&dither_state);

        FREENULL(uloc.pl_vars);
        uloc.pl_vars = calloc(res->num_variables, sizeof(GLint));
        for (int i = 0; i < res->num_variables; i++) {
            struct pl_shader_var sv = res->variables[i];
#if PL_API_VER >= 4
            const char *glsl_type_name = pl_var_glsl_type_name(sv.var);
#else
            const char *glsl_type_name = ra_var_glsl_type_name(sv.var);
#endif
            ADDF("uniform %s %s;\n", glsl_type_name, sv.var.name);
        }

        // We can't handle these yet, but nothing we use requires them, either
        assert(res->num_vertex_attribs == 0);
        assert(res->num_descriptors == 0);

        ADD(res->glsl);
    }
#else
    if (fmt.transfer == TRANSFER_FUNC_SMPTE_ST2084 || fmt.primaries == COLOR_PRIMARIES_BT2020) {
        // no warning for HLG because it's more or less backwards-compatible
        std::clog << "VLC needs to be built with support for libplacebo "
                     "in order to display wide gamut or HDR signals correctly.";
    }
#endif

    if (tex_target == GL_TEXTURE_RECTANGLE) {
        for (unsigned i = 0; i < tex_count; ++i) ADDF("uniform vec2 TexSize%u;\n", i);
    }

    if (is_yuv) ADD("uniform vec4 Coefficients[4];\n");

    ADD("uniform vec4 FillColor;\n"
        "void main(void) {\n"
        " float val;vec4 colors;\n");

    if (tex_target == GL_TEXTURE_RECTANGLE) {
        for (unsigned i = 0; i < tex_count; ++i)
            ADDF(" vec2 TexCoordRect%u = vec2(TexCoord%u.x * TexSize%u.x, "
                 "TexCoord%u.y * TexSize%u.y);\n",
                 i, i, i, i, i);
    }

    unsigned color_idx = 0;
    for (unsigned i = 0; i < tex_count; ++i) {
        const char *swizzle = swizzle_per_tex[i];
        if (swizzle) {
            size_t swizzle_count = strlen(swizzle);
            ADDF(" colors = %s(Texture%u, %s%u);\n", lookup, i, coord_name, i);
            for (unsigned j = 0; j < swizzle_count; ++j) {
                ADDF(" val = colors.%c;\n"
                     " vec4 color%u = vec4(val, val, val, 1);\n",
                     swizzle[j], color_idx);
                color_idx++;
                assert(color_idx <= PICTURE_PLANE_MAX);
            }
        } else {
            ADDF(" vec4 color%u = %s(Texture%u, %s%u);\n", color_idx, lookup, i, coord_name, i);
            color_idx++;
            assert(color_idx <= PICTURE_PLANE_MAX);
        }
    }
    unsigned color_count = color_idx;
    assert(yuv_space == COLOR_SPACE_UNDEF || color_count == 3);

    if (is_yuv)
        ADD(" vec4 result = (color0 * Coefficients[0]) + Coefficients[3];\n");
    else
        ADD(" vec4 result = color0;\n");

    for (unsigned i = 1; i < color_count; ++i) {
        if (yuv_swap_uv) {
            assert(color_count == 3);
            color_idx = (i % 2) + 1;
        } else
            color_idx = i;

        if (is_yuv)
            ADDF(" result = (color%u * Coefficients[%u]) + result;\n", color_idx, i);
        else
            ADDF(" result = color%u + result;\n", color_idx);
    }

#ifdef HAVE_LIBPLACEBO
    if (pl_sh_res) {
        const struct pl_shader_res *res = pl_sh_res;
        assert(res->input == PL_SHADER_SIG_COLOR);
        assert(res->output == PL_SHADER_SIG_COLOR);
        ADDF(" result = %s(result);\n", res->name);
    }
#endif

    ADD(" gl_FragColor = vec4((result * FillColor).xyz, 1.0);\n"
        "}");

#undef ADD
#undef ADDF

    if (vlc_memstream_close(&ms) != 0) return 0;

    GLuint fragment_shader = vt->CreateShader(GL_FRAGMENT_SHADER);
    if (fragment_shader == 0) {
        free(ms.ptr);
        return 0;
    }
    GLint length = ms.length;
    vt->ShaderSource(fragment_shader, 1, (const char **) &ms.ptr, &length);
    vt->CompileShader(fragment_shader);
    if (b_dump_shaders)
        std::clog << ("\n=== Fragment shader for fourcc: %4.4s, colorspace: %d ===\n%s\n", (const char *) &chroma, yuv_space, ms.ptr);
    free(ms.ptr);

    this->tex_target = tex_target;

    return fragment_shader;
}
int OpenGLTextureConverter::pf_fetch_locations(GLuint program)
{
    if (base_shader)
        return tc_base_fetch_locations(program);
    else
        return tc_xyz12_fetch_locations(program);
}

void OpenGLTextureConverter::pf_prepare_shader(const GLsizei *tex_width, const GLsizei *tex_height, float alpha)
{
    if (base_shader)
        return tc_base_prepare_shader(tex_width, tex_height, alpha);
    else
        return tc_xyz12_prepare_shader(tex_width, tex_height, alpha);
}