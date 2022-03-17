#pragma once

#include "VAD3D.h"
#include "video_chroma/d3d9_fmt.h"

extern "C" {
#include <libavcodec/dxva2.h>
}

class DirectXVADXVA2;

class VADxva : public VAD3D {
public:
    VADxva(AVCodecContext *ctx, AVPixelFormat fmt) : VAD3D(ctx, fmt)
    {
        open();
    }
    std::string description() override;

    int open();
    void close();

    virtual int createDevice() override;
    virtual void destroyDevice() override;

    virtual int createDeviceManager() override;
    virtual void destroyDeviceManager() override;

    virtual int createVideoService() override;
    virtual void destroyVideoService() override;

    virtual int createDecoderSurfaces(const AVCodecContext *, unsigned surface_count) override;
    virtual void destroySurfaces() override;

    virtual void setupAvcodecCtx() override;

    virtual va_surface_t *newSurfaceContext(int surface_index) override;

private:
    std::string DxDescribe();

public:
    DirectXVADXVA2 *mDirectxVA = nullptr;

    d3d9_handle_t hd3d = {0};
    d3d9_device_t d3d_dev = {0};

    /* DLL */
    HINSTANCE dxva2_dll = nullptr;

    /* Device manager */
    IDirect3DDeviceManager9 *devmng = nullptr;
    HANDLE device = INVALID_HANDLE_VALUE;

    /* Video service */
    D3DFORMAT render = D3DFMT_UNKNOWN;

    /* Video decoder */
    DXVA2_ConfigPictureDecode cfg = {0};

    /* avcodec internals */
    struct dxva_context hw = {0};
};