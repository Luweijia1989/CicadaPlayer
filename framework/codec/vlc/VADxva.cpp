#include "VADxva.h"
#include "DirectXVA.h"
#include <iostream>

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
    return 0;
}
void VADxva::destroyDevice()
{}

int VADxva::createDeviceManager()
{
    return 0;
}
void VADxva::destroyDeviceManager()
{}

int VADxva::createVideoService()
{
    return 0;
}
void VADxva::destroyVideoService()
{}

int VADxva::createDecoderSurfaces(const AVCodecContext *, unsigned surface_count)
{
    return 0;
}
void VADxva::destroySurfaces()
{}
void VADxva::setupAvcodecCtx()
{}

va_surface_t *VADxva::newSurfaceContext(int surface_index)
{
    return nullptr;
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