#include "VADxva.h"
#include "DirectXVA.h"
#include <initguid.h>
#include <iostream>

#define MS_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)

DEFINE_GUID(IID_IDirectXVideoDecoderService, 0xfc51a551, 0xd5e7, 0x11d9, 0xaf, 0x55, 0x00, 0x05, 0x4e, 0x43, 0xff, 0x02);
MS_GUID(IID_IDirectXVideoAccelerationService, 0xfc51a550, 0xd5e7, 0x11d9, 0xaf, 0x55, 0x00, 0x05, 0x4e, 0x43, 0xff, 0x02);

DEFINE_GUID(DXVA2_NoEncrypt, 0x1b81bed0, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);

DEFINE_GUID(DXVA_Intel_H264_NoFGT_ClearVideo, 0x604F8E68, 0x4951, 0x4c54, 0x88, 0xFE, 0xAB, 0xD2, 0x5C, 0x15, 0xB3, 0xD6);

struct d3d9_surface_t : public va_surface_t {
    d3d9_surface_t() : va_surface_t(), d3d(nullptr)
    {}
    ~d3d9_surface_t()
    {}
    void setSurface(IUnknown *s)
    {
        d3d = (IDirect3DSurface9 *) s;
    }
    IUnknown *getSurface() const
    {
        return d3d;
    }

private:
    IDirect3DSurface9 *d3d;
};

std::string VADxva::description()
{
    return DxDescribe();
}

std::string VADxva::DxDescribe()
{
    D3DADAPTER_IDENTIFIER9 d3dai;
    if (FAILED(IDirect3D9_GetAdapterIdentifier(hd3d.obj, d3d_dev.adapterId, 0, &d3dai))) {
        return NULL;
    }

    char *description;
    if (asprintf(&description, "DXVA2 (%.*s, vendor %s(%lx), device %lx, revision %lx)", (int) sizeof(d3dai.Description), d3dai.Description,
                 DxgiVendorStr(d3dai.VendorId), d3dai.VendorId, d3dai.DeviceId, d3dai.Revision) < 0)
        return NULL;
    std::string ret(description);
    free(description);
    return ret;
}

int VADxva::createDevice()
{
    if (d3d_dev.dev) {
        std::clog << "Reusing Direct3D9 device";
        return VLC_SUCCESS;
    }

    video_format_t fmt = {0};
    fmt.i_width = mCtx->coded_width;
    fmt.i_height = mCtx->coded_height;
    HRESULT hr = D3D9_CreateDevice(&hd3d, GetDesktopWindow(), &fmt, &d3d_dev);
    if (FAILED(hr)) {
        std::clog << "IDirect3D9_CreateDevice failed";
        return VLC_EGENERIC;
    }

    return VLC_SUCCESS;
}
void VADxva::destroyDevice()
{
    D3D9_ReleaseDevice(&d3d_dev);
    D3D9_Destroy(&hd3d);
}

int VADxva::createDeviceManager()
{
    typedef HRESULT(WINAPI * CreateDeviceManager9)(UINT * pResetToken, IDirect3DDeviceManager9 **);
    CreateDeviceManager9 createDeviceManager9 = (CreateDeviceManager9) GetProcAddress(dxva2_dll, "DXVA2CreateDirect3DDeviceManager9");

    if (!createDeviceManager9) {
        std::clog << "cannot load function";
        return VLC_EGENERIC;
    }
    std::clog << "OurDirect3DCreateDeviceManager9 Success!";

    UINT token;
    if (FAILED(createDeviceManager9(&token, &devmng))) {
        std::clog << "OurDirect3DCreateDeviceManager9 failed";
        return VLC_EGENERIC;
    }
    std::clog << "obtained IDirect3DDeviceManager9";

    HRESULT hr = devmng->ResetDevice(d3d_dev.dev, token);
    if (FAILED(hr)) {
        std::clog << "IDirect3DDeviceManager9_ResetDevice failed: %08x" << (unsigned) hr;
        return VLC_EGENERIC;
    }
    return VLC_SUCCESS;
}
void VADxva::destroyDeviceManager()
{
    if (devmng) devmng->Release();
}

int VADxva::createVideoService()
{
    HRESULT hr;

    hr = devmng->OpenDeviceHandle(&device);
    if (FAILED(hr)) {
        std::clog << "OpenDeviceHandle failed";
        return VLC_EGENERIC;
    }

    void *pv;
    hr = devmng->GetVideoService(device, IID_IDirectXVideoDecoderService, &pv);
    if (FAILED(hr)) {
        std::clog << "GetVideoService failed";
        return VLC_EGENERIC;
    }
    mDirectxVA->d3ddec = (IDirectXVideoDecoderService *) pv;

    return VLC_SUCCESS;
}
void VADxva::destroyVideoService()
{
    if (device) {
        HRESULT hr = devmng->CloseDeviceHandle(device);
        if (FAILED(hr)) std::clog << "Failed to release device handle 0x%p. (hr=0x%lX)" << device << hr;
    }
    if (mDirectxVA->d3ddec) mDirectxVA->d3ddec->Release();
}

int VADxva::createDecoderSurfaces(const AVCodecContext *ctx, unsigned surface_count)
{
    HRESULT hr;

    hr = mDirectxVA->d3ddec->CreateSurface(ctx->coded_width, ctx->coded_height, surface_count - 1, render, D3DPOOL_DEFAULT, 0,
                                           DXVA2_VideoDecoderRenderTarget, mDirectxVA->hw_surface, NULL);
    if (FAILED(hr)) {
        std::clog << "IDirectXVideoAccelerationService_CreateSurface %d failed (hr=0x%0lx)" << surface_count - 1 << hr;
        return VLC_EGENERIC;
    }
    std::clog << "IDirectXVideoAccelerationService_CreateSurface succeed with %d surfaces (%dx%d)" << surface_count << ctx->coded_width
              << ctx->coded_height;

    IDirect3DSurface9 *tstCrash;
    hr = mDirectxVA->d3ddec->CreateSurface(ctx->coded_width, ctx->coded_height, 0, render, D3DPOOL_DEFAULT, 0,
                                           DXVA2_VideoDecoderRenderTarget, &tstCrash, NULL);
    if (FAILED(hr)) {
        std::clog << "extra buffer impossible, avoid a crash (hr=0x%0lx)" << hr;
        goto error;
    }
    IDirect3DSurface9_Release(tstCrash);

    /* */
    DXVA2_VideoDesc dsc;
    ZeroMemory(&dsc, sizeof(dsc));
    dsc.SampleWidth = ctx->coded_width;
    dsc.SampleHeight = ctx->coded_height;
    dsc.Format = render;
    if (ctx->framerate.num > 0 && ctx->framerate.den > 0) {
        dsc.InputSampleFreq.Numerator = ctx->framerate.num;
        dsc.InputSampleFreq.Denominator = ctx->framerate.den;
    } else {
        dsc.InputSampleFreq.Numerator = 0;
        dsc.InputSampleFreq.Denominator = 0;
    }
    dsc.OutputFrameFreq = dsc.InputSampleFreq;
    dsc.UABProtectionLevel = FALSE;
    dsc.Reserved = 0;

    /* FIXME I am unsure we can let unknown everywhere */
    DXVA2_ExtendedFormat *ext = &dsc.SampleFormat;
    ext->SampleFormat = 0;          //DXVA2_SampleUnknown;
    ext->VideoChromaSubsampling = 0;//DXVA2_VideoChromaSubsampling_Unknown;
    ext->NominalRange = 0;          //DXVA2_NominalRange_Unknown;
    ext->VideoTransferMatrix = 0;   //DXVA2_VideoTransferMatrix_Unknown;
    ext->VideoLighting = 0;         //DXVA2_VideoLighting_Unknown;
    ext->VideoPrimaries = 0;        //DXVA2_VideoPrimaries_Unknown;
    ext->VideoTransferFunction = 0; //DXVA2_VideoTransFunc_Unknown;

    /* List all configurations available for the decoder */
    UINT cfg_count = 0;
    DXVA2_ConfigPictureDecode *cfg_list = NULL;
    hr = mDirectxVA->d3ddec->GetDecoderConfigurations(mDirectxVA->input, &dsc, NULL, &cfg_count, &cfg_list);
    if (FAILED(hr)) {
        std::clog << "IDirectXVideoDecoderService_GetDecoderConfigurations failed. (hr=0x%0lx)" << hr;
        goto error;
    }
    std::clog << "we got %d decoder configurations" << cfg_count;

    /* Select the best decoder configuration */
    int cfg_score = 0;
    for (unsigned i = 0; i < cfg_count; i++) {
        const DXVA2_ConfigPictureDecode *cfg = &cfg_list[i];

        /* */
        std::clog << "configuration[%d] ConfigBitstreamRaw %d" << i << cfg->ConfigBitstreamRaw;

        /* */
        int score;
        if (cfg->ConfigBitstreamRaw == 1)
            score = 1;
        else if (ctx->codec_id == AV_CODEC_ID_H264 && cfg->ConfigBitstreamRaw == 2)
            score = 2;
        else
            continue;
        if (IsEqualGUID(cfg->guidConfigBitstreamEncryption, DXVA2_NoEncrypt)) score += 16;

        if (cfg_score < score) {
            this->cfg = *cfg;
            cfg_score = score;
        }
    }
    CoTaskMemFree(cfg_list);
    if (cfg_score <= 0) {
        std::clog << "Failed to find a supported decoder configuration";
        goto error;
    }

    /* Create the decoder */
    IDirectXVideoDecoder *decoder;
    /* adds a reference on each decoder surface */
    if (FAILED(mDirectxVA->d3ddec->CreateVideoDecoder(mDirectxVA->input, &dsc, &cfg, mDirectxVA->hw_surface, surface_count, &decoder))) {
        std::clog << "IDirectXVideoDecoderService_CreateVideoDecoder failed";
        goto error;
    }
    mDirectxVA->decoder = decoder;

    std::clog << "IDirectXVideoDecoderService_CreateVideoDecoder succeed";
    return VLC_SUCCESS;
error:
    for (unsigned i = 0; i < surface_count; i++) mDirectxVA->hw_surface[i]->Release();
    return VLC_EGENERIC;
}
void VADxva::destroySurfaces()
{
    if (mDirectxVA->decoder) {
        /* releases a reference on each decoder surface */
        mDirectxVA->decoder->Release();
        mDirectxVA->decoder = NULL;
        for (unsigned i = 0; i < surface_count; i++) mDirectxVA->hw_surface[i]->Release();
    }
}
void VADxva::setupAvcodecCtx()
{
    hw.decoder = mDirectxVA->decoder;
    hw.cfg = &cfg;
    hw.surface_count = surface_count;
    hw.surface = mDirectxVA->hw_surface;

    if (IsEqualGUID(mDirectxVA->input, DXVA_Intel_H264_NoFGT_ClearVideo)) hw.workaround |= FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO;
}

va_surface_t *VADxva::newSurfaceContext(int surface_index)
{
    auto sf = new d3d9_surface_t;
    sf->setSurface(mDirectxVA->hw_surface[surface_index]);
    return sf;
}

int VADxva::open()
{
    int err = VLC_EGENERIC;

    if (mFmt != AV_PIX_FMT_DXVA2_VLD) return VLC_EGENERIC;

    mCtx->hwaccel_context = NULL;

    mDirectxVA = new DirectXVADXVA2(this);
    if (!mDirectxVA) return VLC_ENOMEM;

    /* Load dll*/
    if (D3D9_Create(&hd3d) != VLC_SUCCESS) {
        std::clog << "cannot load d3d9.dll";
        delete mDirectxVA;
        goto error;
    }

    /* Load dll*/
    dxva2_dll = LoadLibrary(TEXT("DXVA2.DLL"));
    if (!dxva2_dll) {
        std::clog << "cannot load DXVA2 decoder DLL";
        D3D9_Destroy(&hd3d);
        delete mDirectxVA;
        goto error;
    }

    err = mDirectxVA->directxVAOpen();
    if (err != VLC_SUCCESS) goto error;

    err = mDirectxVA->directxVASetup(mCtx, 0);
    if (err != VLC_SUCCESS) goto error;

    mCtx->hwaccel_context = &hw;

    return VLC_SUCCESS;

error:
    close();
    return VLC_EGENERIC;
}

void VADxva::close()
{
    if (mDirectxVA) mDirectxVA->directxVAClose();

    if (dxva2_dll) FreeLibrary(dxva2_dll);
}

int VADxva::get(void **opaque, uint8_t **data)
{
    /* Check the device */
    HRESULT hr = devmng->TestDevice(device);
    if (hr == DXVA2_E_NEW_VIDEO_DEVICE) {
        std::clog << "New video device detected.";
        if (resetVideoDecoder()) return VLC_EGENERIC;
    } else if (FAILED(hr)) {
        std::clog << "IDirect3DDeviceManager9_TestDevice %u" << (unsigned) hr;
        return VLC_EGENERIC;
    }

    va_surface_t *vs = nullptr;
    int res = getVASurface(&vs);
    if (res == VLC_SUCCESS) {
        *data = (uint8_t *) vs->getSurface();
        *opaque = vs;
    }
    return res;
}

void VADxva::release(void *opaque, uint8_t *data)
{
    surfaceRelease((va_surface_t *) opaque);
}

void *VADxva::getExtraInfoForRender()
{
    return d3d_dev.devex;
}