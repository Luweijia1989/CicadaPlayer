#include "GLSoftwareTextureConverter.h"
#include "config.h"
#include <cassert>
#include <iostream>

extern "C" {
#include <libavutil/pixdesc.h>
#include <vlc_fixups.h>
}

#ifndef GL_UNPACK_ROW_LENGTH
#define GL_UNPACK_ROW_LENGTH 0x0CF2
#endif

#ifndef GL_PIXEL_UNPACK_BUFFER
#define GL_PIXEL_UNPACK_BUFFER 0x88EC
#endif
#ifndef GL_DYNAMIC_DRAW
#define GL_DYNAMIC_DRAW 0x88E8
#endif

#ifndef GL_MAP_READ_BIT
#define GL_MAP_READ_BIT 0x0001
#endif
#ifndef GL_MAP_WRITE_BIT
#define GL_MAP_WRITE_BIT 0x0002
#endif
#ifndef GL_MAP_FLUSH_EXPLICIT_BIT
#define GL_MAP_FLUSH_EXPLICIT_BIT 0x0010
#endif
#ifndef GL_MAP_PERSISTENT_BIT
#define GL_MAP_PERSISTENT_BIT 0x0040
#endif

#ifndef GL_CLIENT_STORAGE_BIT
#define GL_CLIENT_STORAGE_BIT 0x0200
#endif

#ifndef GL_ALREADY_SIGNALED
#define GL_ALREADY_SIGNALED 0x911A
#endif
#ifndef GL_CONDITION_SATISFIED
#define GL_CONDITION_SATISFIED 0x911C
#endif
#ifndef GL_SYNC_GPU_COMMANDS_COMPLETE
#define GL_SYNC_GPU_COMMANDS_COMPLETE 0x9117
#endif

GLSoftwareTextureConverter::GLSoftwareTextureConverter()
{}

GLSoftwareTextureConverter::~GLSoftwareTextureConverter()
{
    for (size_t i = 0; i < PBO_DISPLAY_COUNT && pbo_cache->pbo.display_pics[i]; ++i) pbo_picture_destroy(pbo_cache->pbo.display_pics[i]);

    free(pbo_cache->texture_temp_buf);
    free(pbo_cache);
}

int GLSoftwareTextureConverter::init()
{
    GLuint fragment_shader = 0;
    video_color_space_t space;
    const vlc_fourcc_t *list;

    if (vlc_fourcc_IsYUV(fmt.i_chroma)) {
        GLint max_texture_units = 0;
        vt->GetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_texture_units);
        if (max_texture_units < 3) return VLC_EGENERIC;

        list = vlc_fourcc_GetYUVFallback(fmt.i_chroma);
        space = fmt.space;
    } else if (fmt.i_chroma == VLC_CODEC_XYZ12) {
        static const vlc_fourcc_t xyz12_list[] = {VLC_CODEC_XYZ12, 0};
        list = xyz12_list;
        space = COLOR_SPACE_UNDEF;
    } else {
        list = vlc_fourcc_GetRGBFallback(fmt.i_chroma);
        space = COLOR_SPACE_UNDEF;
    }

    while (*list) {
        fragment_shader = pf_fragment_shader_init(GL_TEXTURE_2D, *list, space);
        if (fragment_shader != 0) {
            fmt.i_chroma = *list;
            break;
        }
        list++;
    }
    if (fragment_shader == 0) return VLC_EGENERIC;

    pbo_cache = (PBOCache *) calloc(1, sizeof(struct PBOCache));
    if (!pbo_cache) {
        vt->DeleteShader(fragment_shader);
        return VLC_ENOMEM;
    }

    /* OpenGL or OpenGL ES2 with GL_EXT_unpack_subimage ext */
    pbo_cache->has_unpack_subimage = !is_gles || HasExtension(glexts, "GL_EXT_unpack_subimage");

    bool allow_dr = true;
    if (allow_dr) {
        const char *glrenderer = (const char *) vt->GetString(GL_RENDERER);
        assert(glrenderer);
        if (strcmp(glrenderer, "Intel HD Graphics 3000 OpenGL Engine") == 0) {
            std::clog << "Disabling direct rendering because of buggy GPU/Driver";
            allow_dr = false;
        }
    }

    if (allow_dr && pbo_cache->has_unpack_subimage) {
        /* Ensure we do direct rendering / PBO with OpenGL 3.0 or higher.
         * Indeed, persistent mapped buffers or PBO seems to be slow with
         * OpenGL 2.1 drivers and bellow. This may be caused by OpenGL
         * compatibility layer. */
        const unsigned char *ogl_version = vt->GetString(GL_VERSION);
        const bool glver_ok = strverscmp((const char *) ogl_version, "3.0") >= 0;

        const bool has_pbo =
                glver_ok && (HasExtension(glexts, "GL_ARB_pixel_buffer_object") || HasExtension(glexts, "GL_EXT_pixel_buffer_object"));

        const bool supports_pbo = has_pbo && vt->BufferData && vt->BufferSubData;
        if (supports_pbo && pbo_pics_alloc() == 0) {
            updateType = PboUpdate;
            std::clog << "PBO support enabled";
        }
    }

    fshader = fragment_shader;

    return VLC_SUCCESS;
}

int GLSoftwareTextureConverter::pf_allocate_textures(GLuint *textures, const GLsizei *tex_width, const GLsizei *tex_height)
{
    for (unsigned i = 0; i < tex_count; i++) {
        vt->BindTexture(tex_target, textures[i]);
        vt->TexImage2D(tex_target, 0, texs[i].internal, tex_width[i], tex_height[i], 0, texs[i].format, texs[i].type, NULL);
    }
    return 0;
}

int GLSoftwareTextureConverter::pf_update(GLuint *textures, const GLsizei *tex_width, const GLsizei *tex_height, AVFrame *pic)
{
    if (updateType == PboUpdate)
        return pbo_update(textures, tex_width, tex_height, pic);
    else if (updateType == Common)
        return common_update(textures, tex_width, tex_height, pic);
    else
        return -1;
}

int GLSoftwareTextureConverter::common_update(GLuint *textures, const GLsizei *tex_width, const GLsizei *tex_height, AVFrame *pic)
{
    int ret = 0;
    for (unsigned i = 0; i < tex_count && ret == 0; i++) {
        assert(textures[i] != 0);
        vt->ActiveTexture(GL_TEXTURE0 + i);
        vt->BindTexture(tex_target, textures[i]);
        const void *pixels = pic->data[i];

        auto pixdesc = av_pix_fmt_desc_get((AVPixelFormat) pic->format);
        ret = upload_plane(i, tex_width[i], tex_height[i], pic->linesize[i],
                           i == 0 ? pic->width : -((-pic->width) >> pixdesc->log2_chroma_w), pixels);
    }
    return ret;
}

int GLSoftwareTextureConverter::pbo_update(GLuint *textures, const GLsizei *tex_width, const GLsizei *tex_height, AVFrame *pic)
{
    PBOPicture *display_pic = pbo_cache->pbo.display_pics[pbo_cache->pbo.display_idx];
    pbo_cache->pbo.display_idx = (pbo_cache->pbo.display_idx + 1) % PBO_DISPLAY_COUNT;

    for (int i = 0; i < fmt.i_planes; i++) {
        const video_format_t::plane_t &p = fmt.plane[i];
        GLsizeiptr size = p.i_lines * p.i_pitch;
        const GLvoid *data = pic->data[i];
        vt->BindBuffer(GL_PIXEL_UNPACK_BUFFER, display_pic->buffers[i]);
        vt->BufferSubData(GL_PIXEL_UNPACK_BUFFER, 0, size, data);

        vt->ActiveTexture(GL_TEXTURE0 + i);
        vt->BindTexture(tex_target, textures[i]);

        vt->PixelStorei(GL_UNPACK_ROW_LENGTH, p.i_pitch * tex_width[i] / (p.i_visible_pitch ? p.i_visible_pitch : 1));

        vt->TexSubImage2D(tex_target, 0, 0, 0, tex_width[i], tex_height[i], texs[i].format, texs[i].type, NULL);

        vt->PixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    }

    /* turn off pbo */
    vt->BindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    return 0;
}

int GLSoftwareTextureConverter::upload_plane(unsigned tex_idx, GLsizei width, GLsizei height, unsigned pitch, unsigned visible_pitch,
                                             const void *pixels)
{
    GLenum tex_format = texs[tex_idx].format;
    GLenum tex_type = texs[tex_idx].type;

    /* This unpack alignment is the default, but setting it just in case. */
    vt->PixelStorei(GL_UNPACK_ALIGNMENT, 4);

    if (!pbo_cache->has_unpack_subimage) {
        if (pitch != visible_pitch) {
#define ALIGN(x, y) (((x) + ((y) -1)) & ~((y) -1))
            visible_pitch = ALIGN(visible_pitch, 4);
#undef ALIGN
            size_t buf_size = visible_pitch * height;
            const uint8_t *source = (const uint8_t *) pixels;
            uint8_t *destination;
            if (pbo_cache->texture_temp_buf_size < buf_size) {
                pbo_cache->texture_temp_buf = realloc_or_free(pbo_cache->texture_temp_buf, buf_size);
                if (pbo_cache->texture_temp_buf == NULL) {
                    pbo_cache->texture_temp_buf_size = 0;
                    return -1;
                }
                pbo_cache->texture_temp_buf_size = buf_size;
            }
            destination = (uint8_t *) pbo_cache->texture_temp_buf;

            for (GLsizei h = 0; h < height; h++) {
                memcpy(destination, source, visible_pitch);
                source += pitch;
                destination += visible_pitch;
            }
            vt->TexSubImage2D(tex_target, 0, 0, 0, width, height, tex_format, tex_type, pbo_cache->texture_temp_buf);
        } else {
            vt->TexSubImage2D(tex_target, 0, 0, 0, width, height, tex_format, tex_type, pixels);
        }
    } else {
        vt->PixelStorei(GL_UNPACK_ROW_LENGTH, pitch * width / (visible_pitch ? visible_pitch : 1));
        vt->TexSubImage2D(tex_target, 0, 0, 0, width, height, tex_format, tex_type, pixels);
        vt->PixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    }
    return 0;
}

void GLSoftwareTextureConverter::pbo_picture_destroy(PBOPicture *pic)
{
    if (pic->i_planes && pic->buffers) pic->DeleteBuffers(pic->i_planes, pic->buffers);
    if (pic->data[0]) aligned_free(pic->data[0]);
    free(pic);
}

PBOPicture *GLSoftwareTextureConverter::pbo_picture_create(GLSoftwareTextureConverter *converter, bool direct_rendering)
{
    PBOPicture *pic = (PBOPicture *) calloc(1, sizeof(*pic));
    if (!pic) return NULL;

    pic->i_planes = fmt.i_planes;
    size_t i_bytes = 0;
    for (int i = 0; i < pic->i_planes; i++) {
        const video_format_t::plane_t &p = fmt.plane[i];

        if (p.i_pitch < 0 || p.i_lines <= 0 || (size_t) p.i_pitch > (SIZE_MAX - i_bytes) / p.i_lines) {
            free(pic);
            return NULL;
        }
        i_bytes += p.i_pitch * p.i_lines;
    }

    if (i_bytes >= PICTURE_SW_SIZE_MAX) {
        free(pic);
        return NULL;
    }

    i_bytes = (i_bytes + 63) & ~63; /* must be a multiple of 64 */
    uint8_t *p_data = (uint8_t *) aligned_alloc(64, i_bytes);
    if (i_bytes > 0 && p_data == NULL) {
        free(pic);
        return NULL;
    }

    /* Fill the p_pixels field for each plane */
    pic->data[0] = p_data;
    for (int i = 1; i < pic->i_planes; i++) {
        const video_format_t::plane_t &p = fmt.plane[i - 1];
        pic->data[i] = pic->data[i - 1] + p.i_lines * p.i_pitch;
    }

    vt->GenBuffers(pic->i_planes, pic->buffers);
    pic->DeleteBuffers = vt->DeleteBuffers;


    assert(pic->i_planes > 0 && (unsigned) pic->i_planes == tex_count);

    for (int i = 0; i < pic->i_planes; ++i) {
        const video_format_t::plane_t &p = fmt.plane[i];
        pic->bytes[i] = (p.i_pitch * p.i_lines) + 15 / 16 * 16;
    }
    return pic;
}
int GLSoftwareTextureConverter::pbo_data_alloc(PBOPicture *pic)
{
    vt->GetError();

    for (int i = 0; i < pic->i_planes; ++i) {
        vt->BindBuffer(GL_PIXEL_UNPACK_BUFFER, pic->buffers[i]);
        vt->BufferData(GL_PIXEL_UNPACK_BUFFER, pic->bytes[i], NULL, GL_DYNAMIC_DRAW);

        if (vt->GetError() != GL_NO_ERROR) {
            std::clog << "could not alloc PBO buffers";
            vt->DeleteBuffers(i, pic->buffers);
            return -1;
        }
    }
    return 0;
}

int GLSoftwareTextureConverter::pbo_pics_alloc()
{
    for (size_t i = 0; i < PBO_DISPLAY_COUNT; ++i) {
        auto *pic = pbo_cache->pbo.display_pics[i] = pbo_picture_create(this, false);
        if (pic == NULL) goto error;

        if (pbo_data_alloc(pic) != 0) goto error;
    }

    /* turn off pbo */
    vt->BindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    return 0;
error:
    for (size_t i = 0; i < PBO_DISPLAY_COUNT; ++i) {
        if (pbo_cache->pbo.display_pics[i]) pbo_picture_destroy(pbo_cache->pbo.display_pics[i]);
    }
    return -1;
}