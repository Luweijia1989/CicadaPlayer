#pragma once

#if _WIN32_WINNT < 0x0600// _WIN32_WINNT_VISTA
/* d3d11 needs Vista support */
#undef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_VISTA
#endif

#include <vlc_common.h>

#include <libavcodec/avcodec.h>

#include <unknwn.h>
#include <vlc_atomic.h>

#include "VAD3D.h"
#include <d3d9.h>
#include <dxva2api.h>

typedef struct input_list_t {
    void (*pf_release)(struct input_list_t *);
    GUID *list;
    unsigned count;
} input_list_t;

typedef struct {
    const char *name;
    const GUID *guid;
    enum AVCodecID codec;
    const int *p_profiles;// NULL or ends with 0
} directx_va_mode_t;

class DirectXVA {
public:
    DirectXVA(VAD3D *d3d) : mD3d(d3d)
    {}
    virtual int getInputList(input_list_t *) = 0;
    virtual int setupOutput(const GUID *input) = 0;

    int directxVAOpen();
    void directxVAClose();
    int directxVASetup(const AVCodecContext *avctx, int flag_xbox);
    char *directxVAGetDecoderName(const GUID *guid);
    bool directxVAcanUseDecoder(UINT VendorId, UINT DeviceId, const GUID *pCodec, UINT driverBuild);

private:
    int FindVideoServiceConversion(const AVCodecContext *avctx);
    bool profileSupported(const directx_va_mode_t *mode, const AVCodecContext *avctx);

public:
    VAD3D *mD3d = nullptr;
    bool can_extern_pool = false;
    GUID input;
};

class VADxva;
class DirectXVADXVA2 : public DirectXVA {
public:
    DirectXVADXVA2(VAD3D *d3d);
    int getInputList(input_list_t *);
    int setupOutput(const GUID *input);

public:
    VADxva *mVAD3D = nullptr;
    /* for pre allocation */
    IDirect3DSurface9 *hw_surface[MAX_SURFACE_COUNT] = { 0 };

    IDirectXVideoDecoderService *d3ddec = nullptr;

    /* Video decoder */
    IDirectXVideoDecoder *decoder = nullptr;
};