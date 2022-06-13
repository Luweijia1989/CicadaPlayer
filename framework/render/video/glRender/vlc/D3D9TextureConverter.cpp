#include "D3D9TextureConverter.h"
#include <cassert>
#include <iostream>

D3D9TextureConverter::D3D9TextureConverter(void *d3d9ex)
{
    converter.d3d_dev = (LPDIRECT3DDEVICE9EX) d3d9ex;
}
D3D9TextureConverter::~D3D9TextureConverter()
{
    if (converter.dx_render) IDirect3DSurface9_Release(converter.dx_render);

    if (converter.d3d_dev) converter.d3d_dev->Release();
}

int D3D9TextureConverter::init()
{
    if (!converter.d3d_dev) return VLC_EGENERIC;

    if (fmt.i_chroma != VLC_CODEC_D3D9_OPAQUE && fmt.i_chroma != VLC_CODEC_D3D9_OPAQUE_10B) return VLC_EGENERIC;

    if (gl->ext != GLBase::VLC_GL_EXT_WGL || !gl->wgl.getExtensionsString) return VLC_EGENERIC;

    const char *wglExt = gl->wgl.getExtensionsString(gl);

    if (wglExt == NULL || !HasExtension(wglExt, "WGL_NV_DX_interop")) return VLC_EGENERIC;

#define LOAD_EXT(name, type)                                                                                                               \
    do {                                                                                                                                   \
        converter.vt.name = (type) gl->getProcAddress("wgl" #name);                                                                        \
        if (!converter.vt.name) {                                                                                                           \
            std::clog << "'wgl " #name "' could not be loaded";                                                                            \
            return VLC_EGENERIC;                                                                                                           \
        }                                                                                                                                  \
    } while (0)

    LOAD_EXT(DXSetResourceShareHandleNV, PFNWGLDXSETRESOURCESHAREHANDLENVPROC);
    LOAD_EXT(DXOpenDeviceNV, PFNWGLDXOPENDEVICENVPROC);
    LOAD_EXT(DXCloseDeviceNV, PFNWGLDXCLOSEDEVICENVPROC);
    LOAD_EXT(DXRegisterObjectNV, PFNWGLDXREGISTEROBJECTNVPROC);
    LOAD_EXT(DXUnregisterObjectNV, PFNWGLDXUNREGISTEROBJECTNVPROC);
    LOAD_EXT(DXLockObjectsNV, PFNWGLDXLOCKOBJECTSNVPROC);
    LOAD_EXT(DXUnlockObjectsNV, PFNWGLDXUNLOCKOBJECTSNVPROC);

    converter.d3d_dev->AddRef();

    HRESULT hr;
    HANDLE shared_handle = NULL;
    hr = IDirect3DDevice9Ex_CreateRenderTarget(converter.d3d_dev, fmt.i_visible_width, fmt.i_visible_height, D3DFMT_X8R8G8B8,
                                               D3DMULTISAMPLE_NONE, 0, FALSE, &converter.dx_render, &shared_handle);
    if (FAILED(hr)) {
        std::clog << "IDirect3DDevice9_CreateOffscreenPlainSurface failed";
        goto error;
    }

    if (shared_handle) converter.vt.DXSetResourceShareHandleNV(converter.dx_render, shared_handle);

    converter.gl_handle_d3d = converter.vt.DXOpenDeviceNV(converter.d3d_dev);
    if (!converter.gl_handle_d3d) {
        std::clog << "DXOpenDeviceNV failed: %lu" << GetLastError();
        goto error;
    }

    fshader = pf_fragment_shader_init(GL_TEXTURE_2D, VLC_CODEC_RGB32, COLOR_SPACE_UNDEF);
    if (fshader == 0) goto error;

    return VLC_SUCCESS;

error:
    return VLC_EGENERIC;
}

int D3D9TextureConverter::pf_allocate_textures(GLuint *textures, const GLsizei *tex_width, const GLsizei *tex_height)
{
    (void) (tex_width);
    (void) (tex_height);
    converter.gl_render = converter.vt.DXRegisterObjectNV(converter.gl_handle_d3d, converter.dx_render, textures[0], GL_TEXTURE_2D,
                                                          WGL_ACCESS_WRITE_DISCARD_NV);
    if (!converter.gl_render) {
        std::clog << "DXRegisterObjectNV failed: %lu" << GetLastError();
        return VLC_EGENERIC;
    }

    if (!converter.vt.DXLockObjectsNV(converter.gl_handle_d3d, 1, &converter.gl_render)) {
        std::clog << "DXLockObjectsNV failed";
        converter.vt.DXUnregisterObjectNV(converter.gl_handle_d3d, converter.gl_render);
        converter.gl_render = NULL;
        return VLC_EGENERIC;
    }

    return VLC_SUCCESS;
}

int D3D9TextureConverter::pf_update(GLuint *textures, const GLsizei *tex_width, const GLsizei *tex_height, AVFrame *pic)
{
    (void) (textures);
    (void) (tex_width);
    (void) (tex_height);
    HRESULT hr;

    if (!converter.gl_render) return VLC_EGENERIC;

    if (!converter.vt.DXUnlockObjectsNV(converter.gl_handle_d3d, 1, &converter.gl_render)) {
        std::clog << "DXUnlockObjectsNV failed";
        return VLC_EGENERIC;
    }

    const RECT rect = {0, 0, fmt.i_visible_width, fmt.i_visible_height};
    hr = IDirect3DDevice9Ex_StretchRect(converter.d3d_dev, (IDirect3DSurface9 *) pic->data[3], &rect, converter.dx_render, NULL,
                                        D3DTEXF_NONE);
    if (FAILED(hr)) {
        std::clog << "IDirect3DDevice9Ex_StretchRect failed";
        return VLC_EGENERIC;
    }

    if (!converter.vt.DXLockObjectsNV(converter.gl_handle_d3d, 1, &converter.gl_render)) {
        std::clog << "DXLockObjectsNV failed";
        converter.vt.DXUnregisterObjectNV(converter.gl_handle_d3d, converter.gl_render);
        converter.gl_render = NULL;
        return VLC_EGENERIC;
    }

    return VLC_SUCCESS;
}

void D3D9TextureConverter::release()
{
    if (converter.gl_handle_d3d) {
        if (converter.gl_render) {
            converter.vt.DXUnlockObjectsNV(converter.gl_handle_d3d, 1, &converter.gl_render);
            converter.vt.DXUnregisterObjectNV(converter.gl_handle_d3d, converter.gl_render);
        }

        converter.vt.DXCloseDeviceNV(converter.gl_handle_d3d);
    }
}