#pragma once

#include "GLTextureConverter.h"
#define COBJMACROS
#include <d3d9.h>

struct wgl_vt {
    PFNWGLDXSETRESOURCESHAREHANDLENVPROC DXSetResourceShareHandleNV;
    PFNWGLDXOPENDEVICENVPROC DXOpenDeviceNV;
    PFNWGLDXCLOSEDEVICENVPROC DXCloseDeviceNV;
    PFNWGLDXREGISTEROBJECTNVPROC DXRegisterObjectNV;
    PFNWGLDXUNREGISTEROBJECTNVPROC DXUnregisterObjectNV;
    PFNWGLDXLOCKOBJECTSNVPROC DXLockObjectsNV;
    PFNWGLDXUNLOCKOBJECTSNVPROC DXUnlockObjectsNV;
};

struct d3d_convert {
    struct wgl_vt vt;
    LPDIRECT3DDEVICE9EX d3d_dev;
    HANDLE gl_handle_d3d;
    HANDLE gl_render;
    IDirect3DSurface9 *dx_render;
};

class D3D9TextureConverter : public OpenGLTextureConverter {
public:
    D3D9TextureConverter(void *d3d9ex);
    ~D3D9TextureConverter();

    int init();

    int pf_allocate_textures(GLuint *textures, const GLsizei *tex_width, const GLsizei *tex_height);

    int pf_update(GLuint *textures, const GLsizei *tex_width, const GLsizei *tex_height, AVFrame *pic);

private:
    d3d_convert converter = {0};
};