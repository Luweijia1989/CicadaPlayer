#pragma once

#include "GLTextureConverter.h"

struct PBOPicture {
    GLBase *gl;
    PFNGLDELETEBUFFERSPROC DeleteBuffers;
    GLuint buffers[PICTURE_PLANE_MAX];
    uint8_t *data[PICTURE_PLANE_MAX];
    size_t bytes[PICTURE_PLANE_MAX];
    unsigned index;
    int i_planes;
};

#define PBO_DISPLAY_COUNT 2 /* Double buffering */

#define PICTURE_SW_SIZE_MAX (1 << 28) /* 256MB: 8K * 8K * 4*/

struct PBOCache {
    bool has_unpack_subimage;
    void *texture_temp_buf;
    size_t texture_temp_buf_size;
    struct {
        PBOPicture *display_pics[PBO_DISPLAY_COUNT];
        size_t display_idx;
    } pbo;
};

class GLSoftwareTextureConverter : public OpenGLTextureConverter {
public:
    enum TextureUpdateType {
        Common,
        PboUpdate,
    };
    GLSoftwareTextureConverter();
    ~GLSoftwareTextureConverter();

	int init() override;

    int pf_allocate_textures(GLuint *textures, const GLsizei *tex_width, const GLsizei *tex_height);

    int pf_update(GLuint *textures, const GLsizei *tex_width, const GLsizei *tex_height, AVFrame *pic) override;

private:
    PBOPicture *pbo_picture_create(GLSoftwareTextureConverter *converter, bool direct_rendering);
    void pbo_picture_destroy(PBOPicture *pic);
    int pbo_data_alloc(PBOPicture *pic);
    int pbo_pics_alloc();
    int upload_plane(unsigned tex_idx, GLsizei width, GLsizei height, unsigned pitch, unsigned visible_pitch, const void *pixels);

    int common_update(GLuint *textures, const GLsizei *tex_width, const GLsizei *tex_height, AVFrame *pic);
    int pbo_update(GLuint *textures, const GLsizei *tex_width, const GLsizei *tex_height, AVFrame *pic);

private:
    TextureUpdateType updateType = Common;
    PBOCache *pbo_cache = nullptr;
};