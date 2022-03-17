#ifndef VLC_VIDEOCHROMA_DXGI_FMT_H_
#define VLC_VIDEOCHROMA_DXGI_FMT_H_

#include <dxgi.h>
#include <dxgiformat.h>

#include <vlc_common.h>
#include <vlc_fourcc.h>

#define GPU_MANUFACTURER_AMD 0x1002
#define GPU_MANUFACTURER_NVIDIA 0x10DE
#define GPU_MANUFACTURER_VIA 0x1106
#define GPU_MANUFACTURER_INTEL 0x8086
#define GPU_MANUFACTURER_S3 0x5333
#define GPU_MANUFACTURER_QUALCOMM 0x4D4F4351

#define D3D11_MAX_SHADER_VIEW 4

typedef struct {
    const char *name;
    DXGI_FORMAT formatTexture;
    vlc_fourcc_t fourcc;
    uint8_t bitsPerChannel;
    uint8_t widthDenominator;
    uint8_t heightDenominator;
    DXGI_FORMAT resourceFormat[D3D11_MAX_SHADER_VIEW];
} d3d_format_t;

const char *DxgiFormatToStr(DXGI_FORMAT format);
vlc_fourcc_t DxgiFormatFourcc(DXGI_FORMAT format);
const d3d_format_t *GetRenderFormatList(void);
const char *DxgiVendorStr(int gpu_vendor);
UINT DxgiResourceCount(const d3d_format_t *);

#endif /* include-guard */
