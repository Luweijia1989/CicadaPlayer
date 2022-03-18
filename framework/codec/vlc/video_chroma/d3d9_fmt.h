#pragma once

#include "vlc_es.h"

#define COBJMACROS
#include <d3d9.h>
#include <dxva2api.h>

#include "dxgi_fmt.h"

typedef struct
{
    HINSTANCE               hdll;       /* handle of the opened d3d9 dll */
    union {
        LPDIRECT3D9         obj;
        LPDIRECT3D9EX       objex;
    };
    bool                    use_ex;
} d3d9_handle_t;

typedef struct
{
    /* d3d9_handle_t           hd3d; TODO */
    union
    {
        LPDIRECT3DDEVICE9   dev;
        LPDIRECT3DDEVICE9EX devex;
    };
    bool                    owner;

    /* creation parameters */
    D3DPRESENT_PARAMETERS   pp;
    UINT                    adapterId;
    HWND                    hwnd;
    D3DCAPS9                caps;
} d3d9_device_t;

HRESULT D3D9_CreateDevice(d3d9_handle_t *, HWND,
                          const video_format_t *, d3d9_device_t *out);

void D3D9_ReleaseDevice(d3d9_device_t *);
int D3D9_Create(d3d9_handle_t *);

void D3D9_Destroy(d3d9_handle_t *);

int D3D9_FillPresentationParameters(d3d9_handle_t *, const video_format_t *, d3d9_device_t *);
